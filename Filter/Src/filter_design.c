/**
  ******************************************************************************
  * @file           : filter_design.c
  * @brief          : Filter design implementation for audio DSP
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Implementation of digital filter design algorithms for audio processing
  * Includes methods for creating various filter types common in audio crossovers
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "filter_design.h"
#include "butterworth.h"
#include "linkwitz_riley.h"
#include "bessel.h"
#include "biquad.h"
#include "math_utils.h"
#include <math.h>
#include <string.h>
#include <stdint.h>

/* Private define ------------------------------------------------------------*/
#define MAX_FILTER_ORDER    8
#define MAX_FILTER_SECTIONS 4  /* For up to 8th order filters (4 biquads) */

/* Private typedef -----------------------------------------------------------*/
typedef struct {
    double zeros[2]; /* z^2 + zeros[0]*z + zeros[1] */
    double poles[2]; /* z^2 + poles[0]*z + poles[1] */
    double gain;     /* Overall gain factor */
} FilterSection_t;

/* Private function prototypes -----------------------------------------------*/
static void BilinearTransform(FilterCoefficients_t* coeffs, float sampleFreq);
static void NormalizeCoefficients(FilterCoefficients_t* coeffs);
static void DesignSecondOrderLowPass(FilterCoefficients_t* coeffs, float freq, float Q);
static void DesignSecondOrderHighPass(FilterCoefficients_t* coeffs, float freq, float Q);
static void DesignSecondOrderBandPass(FilterCoefficients_t* coeffs, float freq, float bandwidth);
static void DesignSecondOrderBandStop(FilterCoefficients_t* coeffs, float freq, float bandwidth);
static void DesignLowShelf(FilterCoefficients_t* coeffs, float freq, float Q, float gainDb);
static void DesignHighShelf(FilterCoefficients_t* coeffs, float freq, float Q, float gainDb);
static void DesignPeakingEQ(FilterCoefficients_t* coeffs, float freq, float Q, float gainDb);
static float ComputeQForButterworthOrder(uint8_t order, uint8_t section);
static void ComputeShelfParameters(float gainDb, float freq, float Q, 
                                  float* A, float* beta, float* omega);

/* Filter design functions ---------------------------------------------------*/

/**
  * @brief  Create IIR filter coefficients based on filter type and parameters
  * @param  coeffs: Pointer to filter coefficients structure
  * @param  filterType: Type of filter (LP, HP, BP, etc)
  * @param  freq: Cutoff/center frequency in Hz
  * @param  Q: Q-factor for the filter
  * @param  gainDb: Gain in dB (for shelving and peaking filters)
  * @param  sampleFreq: Sampling frequency in Hz
  * @retval Filter design status (0 = success)
  */
int FilterDesign_Create(FilterCoefficients_t* coeffs, 
                        FilterType_t filterType,
                        float freq, 
                        float Q, 
                        float gainDb,
                        float sampleFreq)
{
    /* Validate parameters */
    if (coeffs == NULL || freq <= 0 || sampleFreq <= 0) {
        return FILTER_DESIGN_ERROR_PARAMS;
    }
    
    if (freq >= sampleFreq / 2.0f) {
        freq = sampleFreq / 2.0f - 1.0f; /* Limit to just below Nyquist */
    }
    
    /* Initialize coefficients to neutral values */
    memset(coeffs, 0, sizeof(FilterCoefficients_t));
    coeffs->a[0] = 1.0f;
    
    /* Design the filter based on type */
    switch (filterType) {
        case FILTER_TYPE_LOWPASS:
            DesignSecondOrderLowPass(coeffs, freq, Q);
            break;
            
        case FILTER_TYPE_HIGHPASS:
            DesignSecondOrderHighPass(coeffs, freq, Q);
            break;
            
        case FILTER_TYPE_BANDPASS:
            DesignSecondOrderBandPass(coeffs, freq, 1.0f/Q); /* Bandwidth = 1/Q */
            break;
            
        case FILTER_TYPE_BANDSTOP:
            DesignSecondOrderBandStop(coeffs, freq, 1.0f/Q); /* Bandwidth = 1/Q */
            break;
            
        case FILTER_TYPE_LOW_SHELF:
            DesignLowShelf(coeffs, freq, Q, gainDb);
            break;
            
        case FILTER_TYPE_HIGH_SHELF:
            DesignHighShelf(coeffs, freq, Q, gainDb);
            break;
            
        case FILTER_TYPE_PEAKING_EQ:
            DesignPeakingEQ(coeffs, freq, Q, gainDb);
            break;
            
        case FILTER_TYPE_ALL_PASS:
            /* All-pass filter design */
            coeffs->b[0] = 1.0f;
            coeffs->b[1] = -2.0f * cosf(2.0f * M_PI * freq / sampleFreq);
            coeffs->b[2] = 1.0f;
            coeffs->a[0] = 1.0f;
            coeffs->a[1] = coeffs->b[1];
            coeffs->a[2] = 1.0f;
            break;
            
        default:
            return FILTER_DESIGN_ERROR_TYPE;
    }
    
    /* Apply bilinear transform to convert to digital domain */
    BilinearTransform(coeffs, sampleFreq);
    
    /* Normalize a0 to 1.0 */
    NormalizeCoefficients(coeffs);
    
    return FILTER_DESIGN_SUCCESS;
}

/**
  * @brief  Design a cascade of filters for higher order responses
  * @param  filterBank: Array of filter coefficient structures
  * @param  numFilters: Number of filters to create in cascade
  * @param  filterType: Type of filter (LP, HP)
  * @param  freq: Cutoff frequency in Hz
  * @param  filterResponse: Filter response type (Butterworth, L-R, etc)
  * @param  order: Filter order (determines steepness)
  * @param  sampleFreq: Sampling frequency in Hz
  * @retval Filter design status (0 = success)
  */
int FilterDesign_CreateCascade(FilterCoefficients_t* filterBank,
                               uint8_t numFilters,
                               FilterType_t filterType,
                               float freq,
                               FilterResponse_t filterResponse,
                               uint8_t order,
                               float sampleFreq)
{
    uint8_t i;
    float Q;
    int status = FILTER_DESIGN_SUCCESS;
    
    /* Validate parameters */
    if (filterBank == NULL || numFilters < 1 || freq <= 0 || sampleFreq <= 0) {
        return FILTER_DESIGN_ERROR_PARAMS;
    }
    
    /* Check if we have enough filters in the bank for the requested order */
    uint8_t requiredFilters = (order + 1) / 2; /* Ceiling division of order/2 */
    if (numFilters < requiredFilters) {
        return FILTER_DESIGN_ERROR_ORDER;
    }
    
    /* Handle special case for Linkwitz-Riley which requires Butterworth stages */
    if (filterResponse == FILTER_RESPONSE_LINKWITZ_RILEY) {
        /* L-R filters are implemented as cascaded Butterworth filters */
        /* Make sure order is even for L-R */
        if (order % 2 != 0) {
            order = order + 1;
        }
        
        /* For L-R filters, implement as two Butterworth filters in cascade */
        uint8_t butterworthOrder = order / 2;
        uint8_t butterworthSections = (butterworthOrder + 1) / 2;
        
        /* First half of the filter bank gets the first Butterworth */
        for (i = 0; i < butterworthSections; i++) {
            Q = ComputeQForButterworthOrder(butterworthOrder, i);
            status = FilterDesign_Create(&filterBank[i], 
                                         filterType, 
                                         freq, 
                                         Q, 
                                         0.0f, 
                                         sampleFreq);
            if (status != FILTER_DESIGN_SUCCESS) {
                return status;
            }
        }
        
        /* Second half of the filter bank gets the second Butterworth */
        for (i = 0; i < butterworthSections; i++) {
            Q = ComputeQForButterworthOrder(butterworthOrder, i);
            status = FilterDesign_Create(&filterBank[butterworthSections + i], 
                                         filterType, 
                                         freq, 
                                         Q, 
                                         0.0f, 
                                         sampleFreq);
            if (status != FILTER_DESIGN_SUCCESS) {
                return status;
            }
        }
        
        return FILTER_DESIGN_SUCCESS;
    }
    
    /* For other filter types (Butterworth, Bessel) */
    switch (filterResponse) {
        case FILTER_RESPONSE_BUTTERWORTH:
            for (i = 0; i < requiredFilters; i++) {
                Q = ComputeQForButterworthOrder(order, i);
                status = FilterDesign_Create(&filterBank[i], 
                                             filterType, 
                                             freq, 
                                             Q, 
                                             0.0f, 
                                             sampleFreq);
                if (status != FILTER_DESIGN_SUCCESS) {
                    return status;
                }
            }
            break;
            
        case FILTER_RESPONSE_BESSEL:
            /* For Bessel, we use pre-computed poles */
            if (order > 8) {
                return FILTER_DESIGN_ERROR_ORDER;
            }
            
            /* Create biquad sections using Bessel pole positions */
            status = Bessel_CreateFilter(filterBank, 
                                        numFilters, 
                                        filterType, 
                                        freq, 
                                        order, 
                                        sampleFreq);
            break;
            
        default:
            return FILTER_DESIGN_ERROR_RESPONSE;
    }
    
    return status;
}

/**
  * @brief  Creates a crossover filter set (LP + HP) at specified frequency
  * @param  lpFilter: Pointer to lowpass filter coefficients
  * @param  hpFilter: Pointer to highpass filter coefficients
  * @param  freq: Crossover frequency in Hz
  * @param  filterResponse: Filter response type (Butterworth, L-R, etc)
  * @param  order: Filter order (determines steepness)
  * @param  sampleFreq: Sampling frequency in Hz
  * @retval Filter design status (0 = success)
  */
int FilterDesign_CreateCrossover(FilterCoefficients_t* lpFilter,
                                FilterCoefficients_t* hpFilter,
                                float freq,
                                FilterResponse_t filterResponse,
                                uint8_t order,
                                float sampleFreq)
{
    int status;
    
    /* Validate parameters */
    if (lpFilter == NULL || hpFilter == NULL || freq <= 0 || sampleFreq <= 0) {
        return FILTER_DESIGN_ERROR_PARAMS;
    }
    
    /* Design lowpass filter */
    status = FilterDesign_Create(lpFilter, 
                                 FILTER_TYPE_LOWPASS, 
                                 freq, 
                                 GetDefaultQ(filterResponse, order), 
                                 0.0f, 
                                 sampleFreq);
    if (status != FILTER_DESIGN_SUCCESS) {
        return status;
    }
    
    /* Design highpass filter */
    status = FilterDesign_Create(hpFilter, 
                                 FILTER_TYPE_HIGHPASS, 
                                 freq, 
                                 GetDefaultQ(filterResponse, order), 
                                 0.0f, 
                                 sampleFreq);
    
    return status;
}

/**
  * @brief  Get the default Q factor for a given filter response and order
  * @param  filterResponse: Filter response type
  * @param  order: Filter order
  * @retval Default Q factor value
  */
float GetDefaultQ(FilterResponse_t filterResponse, uint8_t order)
{
    /* Default Q values for different filter types */
    switch (filterResponse) {
        case FILTER_RESPONSE_BUTTERWORTH:
            return 0.7071f; /* Q for 2nd order Butterworth */
            
        case FILTER_RESPONSE_LINKWITZ_RILEY:
            if (order == 2) return 0.5f;      /* LR2 */
            else if (order == 4) return 0.7071f;  /* LR4 */
            else if (order == 8) return 0.871f;    /* LR8 */
            else return 0.7071f;              /* Default */
            
        case FILTER_RESPONSE_BESSEL:
            return 0.5f; /* Bessel has more gradual roll-off */
            
        default:
            return 0.7071f;
    }
}

/**
  * @brief  Convert filter coefficients to fixed point representation
  * @param  src: Source floating point coefficients
  * @param  dst: Destination fixed point coefficients
  * @param  qFormat: Q format for fixed point representation
  * @retval None
  */
void FilterDesign_ConvertToFixed(const FilterCoefficients_t* src,
                                FilterCoefficientsFixed_t* dst,
                                uint8_t qFormat)
{
    int i;
    float scaleFactor = (float)(1 << qFormat);
    
    /* Convert floating point coefficients to fixed point */
    for (i = 0; i < FILTER_MAX_COEFS; i++) {
        dst->b[i] = (int32_t)(src->b[i] * scaleFactor);
        dst->a[i] = (int32_t)(src->a[i] * scaleFactor);
    }
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Apply bilinear transform to convert from analog to digital domain
  * @param  coeffs: Pointer to filter coefficients structure
  * @param  sampleFreq: Sampling frequency in Hz
  * @retval None
  */
static void BilinearTransform(FilterCoefficients_t* coeffs, float sampleFreq)
{
    float w0, alpha, cosw0;
    float a0, a1, a2, b0, b1, b2;
    
    /* Compute intermediate values */
    w0 = 2.0f * M_PI * coeffs->w0 / sampleFreq;
    cosw0 = cosf(w0);
    alpha = sinf(w0) / (2.0f * coeffs->Q);
    
    /* Bilinear transform depends on filter type */
    /* However, we've already designed coefficients in analog domain */
    /* So we just need to copy them and normalize */
    a0 = coeffs->a[0];
    a1 = coeffs->a[1];
    a2 = coeffs->a[2];
    b0 = coeffs->b[0];
    b1 = coeffs->b[1];
    b2 = coeffs->b[2];
    
    /* Normalize by a0 */
    float a0_inv = 1.0f / a0;
    coeffs->b[0] = b0 * a0_inv;
    coeffs->b[1] = b1 * a0_inv;
    coeffs->b[2] = b2 * a0_inv;
    coeffs->a[0] = 1.0f;
    coeffs->a[1] = a1 * a0_inv;
    coeffs->a[2] = a2 * a0_inv;
}

/**
  * @brief  Normalize filter coefficients so a0 = 1.0
  * @param  coeffs: Pointer to filter coefficients structure
  * @retval None
  */
static void NormalizeCoefficients(FilterCoefficients_t* coeffs)
{
    float a0;
    
    /* If a0 is already 1.0, no need to normalize */
    if (fabsf(coeffs->a[0] - 1.0f) < 0.000001f) {
        return;
    }
    
    /* Normalize all coefficients by a0 */
    a0 = coeffs->a[0];
    coeffs->b[0] /= a0;
    coeffs->b[1] /= a0;
    coeffs->b[2] /= a0;
    coeffs->a[1] /= a0;
    coeffs->a[2] /= a0;
    coeffs->a[0] = 1.0f;
}

/**
  * @brief  Design second order low pass filter
  * @param  coeffs: Pointer to filter coefficients structure
  * @param  freq: Cutoff frequency in Hz
  * @param  Q: Q-factor for the filter
  * @retval None
  */
static void DesignSecondOrderLowPass(FilterCoefficients_t* coeffs, float freq, float Q)
{
    /* Set internal parameters */
    coeffs->w0 = freq;
    coeffs->Q = Q;
    
    /* Calculate second-order low-pass filter coefficients */
    float w0 = 2.0f * M_PI * freq;
    float alpha = w0 / (2.0f * Q);
    
    /* Analog prototype: H(s) = 1 / (s^2 + s/Q + 1) */
    coeffs->b[0] = w0 * w0;             /* b0 = w0^2 */
    coeffs->b[1] = 0.0f;                /* b1 = 0 */
    coeffs->b[2] = 0.0f;                /* b2 = 0 */
    coeffs->a[0] = w0 * w0;             /* a0 = w0^2 */
    coeffs->a[1] = alpha;               /* a1 = s/Q */
    coeffs->a[2] = 1.0f;                /* a2 = s^2 */
}

/**
  * @brief  Design second order high pass filter
  * @param  coeffs: Pointer to filter coefficients structure
  * @param  freq: Cutoff frequency in Hz
  * @param  Q: Q-factor for the filter
  * @retval None
  */
static void DesignSecondOrderHighPass(FilterCoefficients_t* coeffs, float freq, float Q)
{
    /* Set internal parameters */
    coeffs->w0 = freq;
    coeffs->Q = Q;
    
    /* Calculate second-order high-pass filter coefficients */
    float w0 = 2.0f * M_PI * freq;
    float alpha = w0 / (2.0f * Q);
    
    /* Analog prototype: H(s) = s^2 / (s^2 + s/Q + 1) */
    coeffs->b[0] = 1.0f;                /* b0 = s^2 */
    coeffs->b[1] = 0.0f;                /* b1 = 0 */
    coeffs->b[2] = 0.0f;                /* b2 = 0 */
    coeffs->a[0] = w0 * w0;             /* a0 = w0^2 */
    coeffs->a[1] = alpha;               /* a1 = s/Q */
    coeffs->a[2] = 1.0f;                /* a2 = s^2 */
}

/**
  * @brief  Design second order band pass filter
  * @param  coeffs: Pointer to filter coefficients structure
  * @param  freq: Center frequency in Hz
  * @param  bandwidth: Filter bandwidth in octaves
  * @retval None
  */
static void DesignSecondOrderBandPass(FilterCoefficients_t* coeffs, float freq, float bandwidth)
{
    /* Set internal parameters */
    coeffs->w0 = freq;
    coeffs->Q = 1.0f / bandwidth;
    
    /* Calculate second-order band-pass filter coefficients */
    float w0 = 2.0f * M_PI * freq;
    float alpha = sinf(w0) * sinhf(logf(2.0f) / 2.0f * bandwidth * w0 / sinf(w0));
    
    /* Digital domain coefficients */
    float a0 = 1.0f + alpha;
    coeffs->b[0] = alpha;                     /* b0 = alpha */
    coeffs->b[1] = 0.0f;                      /* b1 = 0 */
    coeffs->b[2] = -alpha;                    /* b2 = -alpha */
    coeffs->a[0] = a0;                        /* a0 = 1 + alpha */
    coeffs->a[1] = -2.0f * cosf(w0);          /* a1 = -2*cos(w0) */
    coeffs->a[2] = 1.0f - alpha;              /* a2 = 1 - alpha */
}

/**
  * @brief  Design second order band stop (notch) filter
  * @param  coeffs: Pointer to filter coefficients structure
  * @param  freq: Center frequency in Hz
  * @param  bandwidth: Filter bandwidth in octaves
  * @retval None
  */
static void DesignSecondOrderBandStop(FilterCoefficients_t* coeffs, float freq, float bandwidth)
{
    /* Set internal parameters */
    coeffs->w0 = freq;
    coeffs->Q = 1.0f / bandwidth;
    
    /* Calculate second-order band-stop filter coefficients */
    float w0 = 2.0f * M_PI * freq;
    float alpha = sinf(w0) * sinhf(logf(2.0f) / 2.0f * bandwidth * w0 / sinf(w0));
    
    /* Digital domain coefficients */
    float a0 = 1.0f + alpha;
    coeffs->b[0] = 1.0f;                      /* b0 = 1 */
    coeffs->b[1] = -2.0f * cosf(w0);          /* b1 = -2*cos(w0) */
    coeffs->b[2] = 1.0f;                      /* b2 = 1 */
    coeffs->a[0] = a0;                        /* a0 = 1 + alpha */
    coeffs->a[1] = -2.0f * cosf(w0);          /* a1 = -2*cos(w0) */
    coeffs->a[2] = 1.0f - alpha;              /* a2 = 1 - alpha */
}

/**
  * @brief  Design low shelf filter
  * @param  coeffs: Pointer to filter coefficients structure
  * @param  freq: Corner frequency in Hz
  * @param  Q: Q-factor (determines transition slope)
  * @param  gainDb: Gain in dB
  * @retval None
  */
static void DesignLowShelf(FilterCoefficients_t* coeffs, float freq, float Q, float gainDb)
{
    float A, beta, omega;
    
    /* Set internal parameters */
    coeffs->w0 = freq;
    coeffs->Q = Q;
    
    /* Compute intermediate parameters */
    ComputeShelfParameters(gainDb, freq, Q, &A, &beta, &omega);
    
    /* Calculate coefficients */
    float cosw = cosf(omega);
    float sinw = sinf(omega);
    float alpha = sinw * sqrtf((A + 1.0f/A) * (1.0f/Q - 1.0f) + 2.0f);
    
    float a0 = (A + 1.0f) + (A - 1.0f) * cosw + beta * sinw;
    float a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosw);
    float a2 = (A + 1.0f) + (A - 1.0f) * cosw - beta * sinw;
    float b0 = A * ((A + 1.0f) - (A - 1.0f) * cosw + beta * sinw);
    float b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw);
    float b2 = A * ((A + 1.0f) - (A - 1.0f) * cosw - beta * sinw);
    
    /* Store coefficients */
    coeffs->b[0] = b0;
    coeffs->b[1] = b1;
    coeffs->b[2] = b2;
    coeffs->a[0] = a0;
    coeffs->a[1] = a1;
    coeffs->a[2] = a2;
}

/**
  * @brief  Design high shelf filter
  * @param  coeffs: Pointer to filter coefficients structure
  * @param  freq: Corner frequency in Hz
  * @param  Q: Q-factor (determines transition slope)
  * @param  gainDb: Gain in dB
  * @retval None
  */
static void DesignHighShelf(FilterCoefficients_t* coeffs, float freq, float Q, float gainDb)
{
    float A, beta, omega;
    
    /* Set internal parameters */
    coeffs->w0 = freq;
    coeffs->Q = Q;
    
    /* Compute intermediate parameters */
    ComputeShelfParameters(gainDb, freq, Q, &A, &beta, &omega);
    
    /* Calculate coefficients */
    float cosw = cosf(omega);
    float sinw = sinf(omega);
    float alpha = sinw * sqrtf((A + 1.0f/A) * (1.0f/Q - 1.0f) + 2.0f);
    
    float a0 = (A + 1.0f) - (A - 1.0f) * cosw + beta * sinw;
    float a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosw);
    float a2 = (A + 1.0f) - (A - 1.0f) * cosw - beta * sinw;
    float b0 = A * ((A + 1.0f) + (A - 1.0f) * cosw + beta * sinw);
    float b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw);
    float b2 = A * ((A + 1.0f) + (A - 1.0f) * cosw - beta * sinw);
    
    /* Store coefficients */
    coeffs->b[0] = b0;
    coeffs->b[1] = b1;
    coeffs->b[2] = b2;
    coeffs->a[0] = a0;
    coeffs->a[1] = a1;
    coeffs->a[2] = a2;
}

/**
  * @brief  Design peaking EQ filter
  * @param  coeffs: Pointer to filter coefficients structure
  * @param  freq: Center frequency in Hz
  * @param  Q: Q-factor (determines bandwidth)
  * @param  gainDb: Gain in dB
  * @retval None
  */
static void DesignPeakingEQ(FilterCoefficients_t* coeffs, float freq, float Q, float gainDb)
{
    /* Set internal parameters */
    coeffs->w0 = freq;
    coeffs->Q = Q;
    
    /* Convert gain from dB to linear scale */
    float A = powf(10.0f, gainDb / 40.0f);
    
    /* Calculate second-order peaking EQ coefficients */
    float w0 = 2.0f * M_PI * freq;
    float alpha = sinf(w0) / (2.0f * Q);
    
    /* Digital domain coefficients */
    float a0 = 1.0f + alpha / A;
    coeffs->b[0] = 1.0f + alpha * A;          /* b0 = 1 + alpha*A */
    coeffs->b[1] = -2.0f * cosf(w0);          /* b1 = -2*cos(w0) */
    coeffs->b[2] = 1.0f - alpha * A;          /* b2 = 1 - alpha*A */
    coeffs->a[0] = a0;                        /* a0 = 1 + alpha/A */
    coeffs->a[1] = -2.0f * cosf(w0);          /* a1 = -2*cos(w0) */
    coeffs->a[2] = 1.0f - alpha / A;          /* a2 = 1 - alpha/A */
}

/**
  * @brief  Compute Q factor for Butterworth filter of given order and section
  * @param  order: Filter order
  * @param  section: Section index (for filters requiring multiple biquads)
  * @retval Q factor value
  */
static float ComputeQForButterworthOrder(uint8_t order, uint8_t section)
{
    /* For odd-order filters, the first section is first-order */
    if (order % 2 == 1 && section == 0) {
        return 0.5f; /* Q value not used for first-order section */
    }
    
    /* Compute the actual section index based on whether order is odd */
    uint8_t actualSection = (order % 2 == 1) ? section - 1 : section;
    
    /* For even-order filters, all sections are second-order */
    /* Q values for Butterworth filters based on order */
    float Q = 0.0f;
    switch (order) {
        case 2:
            Q = 0.7071f; /* 2nd order - 1 section */
            break;
            
        case 3:
            Q = 1.0f; /* 3rd order - 1st order + 1 biquad */
            break;
            
        case 4:
            if (actualSection == 0) Q = 0.54f; /* 4th order - 2 biquads */
            else if (actualSection == 1) Q = 1.31f;
            break;
            
        case 5:
            if (actualSection == 0) Q = 0.62f; /* 5th order - 1st order + 2 biquads */
            else if (actualSection == 1) Q = 1.62f;
            break;
            
        case 6:
            if (actualSection == 0) Q = 0.52f; /* 6th order - 3 biquads */
            else if (actualSection == 1) Q = 0.71f;
            else if (actualSection == 2) Q = 1.93f;
            break;
            
        case 7:
            if (actualSection == 0) Q = 0.55f; /* 7th order - 1st order + 3 biquads */
            else if (actualSection == 1) Q = 0.80f;
            else if (actualSection == 2) Q = 2.25f;
            break;
            
        case 8:
            if (actualSection == 0) Q = 0.51f; /* 8th order - 4 biquads */
            else if (actualSection == 1) Q = 0.60f;
            else if (actualSection == 2) Q = 0.90f;
            else if (actualSection == 3) Q = 2.56f;
            break;
            
        default:
            Q = 0.7071f; /* Default to 0.7071 for unknown orders */
            break;
    }
    
    return Q;
}

/**
  * @brief  Compute shelf filter intermediate parameters
  * @param  gainDb: Gain in dB
  * @param  freq: Corner frequency in Hz
  * @param  Q: Q-factor (determines transition slope)
  * @param  A: Pointer to amplitude factor
  * @param  beta: Pointer to beta factor
  * @param  omega: Pointer to angular frequency
  * @retval None
  */
static void ComputeShelfParameters(float gainDb, float freq, float Q, 
                                  float* A, float* beta, float* omega)
{
    /* Convert gain from dB to linear scale */
    *A = powf(10.0f, gainDb / 40.0f);
    
    /* Compute angular frequency */
    *omega = 2.0f * M_PI * freq;
    
    /* Compute beta (transition slope) */
    *beta = sqrtf((*A * *A + 1.0f) / Q - (*A - 1.0f) * (*A - 1.0f));
}

/**
  * @brief  Estimate magnitude response at a given frequency for filter
  * @param  coeffs: Pointer to filter coefficients structure
  * @param  freq: Frequency to evaluate at (Hz)
  * @param  sampleFreq: Sampling frequency in Hz
  * @retval Magnitude response in dB
  */
float FilterDesign_GetMagnitudeResponse(const FilterCoefficients_t* coeffs, 
                                       float freq, 
                                       float sampleFreq)
{
    float omega, real_num, imag_num, real_den, imag_den;
    float magnitude;
    
    /* Validate parameters */
    if (coeffs == NULL || freq < 0 || sampleFreq <= 0) {
        return -INFINITY;
    }
    
    /* Compute omega (normalized digital frequency) */
    omega = 2.0f * M_PI * freq / sampleFreq;
    
    /* Compute response at this frequency */
    real_num = coeffs->b[0] + coeffs->b[1] * cosf(omega) + coeffs->b[2] * cosf(2 * omega);
    imag_num = -coeffs->b[1] * sinf(omega) - coeffs->b[2] * sinf(2 * omega);
    
    real_den = 1.0f + coeffs->a[1] * cosf(omega) + coeffs->a[2] * cosf(2 * omega);
    imag_den = -coeffs->a[1] * sinf(omega) - coeffs->a[2] * sinf(2 * omega);
    
    /* Calculate magnitude response */
    magnitude = sqrtf((real_num * real_num + imag_num * imag_num) / 
                      (real_den * real_den + imag_den * imag_den));
    
    /* Convert to dB */
    return 20.0f * log10f(magnitude);
}

/**
  * @brief  Estimate group delay at a given frequency for filter
  * @param  coeffs: Pointer to filter coefficients structure
  * @param  freq: Frequency to evaluate at (Hz)
  * @param  sampleFreq: Sampling frequency in Hz
  * @retval Group delay in samples
  */
float FilterDesign_GetGroupDelay(const FilterCoefficients_t* coeffs, 
                                float freq, 
                                float sampleFreq)
{
    float omega, real_num, imag_num, real_den, imag_den;
    float num_phase_deriv, den_phase_deriv;
    
    /* Validate parameters */
    if (coeffs == NULL || freq < 0 || sampleFreq <= 0) {
        return 0.0f;
    }
    
    /* Compute omega (normalized digital frequency) */
    omega = 2.0f * M_PI * freq / sampleFreq;
    
    /* For a biquad, the group delay can be estimated using the derivative
       of the phase response with respect to omega */
    
    /* Numerator phase derivative */
    num_phase_deriv = coeffs->b[1] + 2 * coeffs->b[2] * cosf(omega);
    
    /* Denominator phase derivative */
    den_phase_deriv = coeffs->a[1] + 2 * coeffs->a[2] * cosf(omega);
    
    /* Calculate group delay (negative derivative of phase) */
    return -num_phase_deriv + den_phase_deriv;
}

/**
  * @brief  Design an all-pass filter to match a specific phase delay
  * @param  coeffs: Pointer to filter coefficients structure
  * @param  freq: Frequency to design for (Hz)
  * @param  delay: Desired delay in milliseconds
  * @param  sampleFreq: Sampling frequency in Hz
  * @retval Filter design status (0 = success)
  */
int FilterDesign_CreateAllPassDelay(FilterCoefficients_t* coeffs,
                                   float freq,
                                   float delay,
                                   float sampleFreq)
{
    /* Validate parameters */
    if (coeffs == NULL || freq <= 0 || sampleFreq <= 0) {
        return FILTER_DESIGN_ERROR_PARAMS;
    }
    
    /* Calculate phase shift needed for the desired delay */
    float phaseShift = 2.0f * M_PI * freq * delay / 1000.0f;
    phaseShift = fmodf(phaseShift, 2.0f * M_PI); /* Normalize to 0-2π */
    
    /* Design first order all-pass filter */
    float alpha = (1.0f - tanf(phaseShift / 2.0f)) / (1.0f + tanf(phaseShift / 2.0f));
    
    /* Set filter coefficients */
    coeffs->b[0] = alpha;
    coeffs->b[1] = 1.0f;
    coeffs->b[2] = 0.0f;
    coeffs->a[0] = 1.0f;
    coeffs->a[1] = alpha;
    coeffs->a[2] = 0.0f;
    
    /* Store parameters */
    coeffs->w0 = freq;
    coeffs->Q = 0.0f; /* Not applicable for all-pass */
    
    return FILTER_DESIGN_SUCCESS;
}

/**
  * @brief  Apply filter coefficients to an audio sample
  * @param  state: Pointer to filter state structure
  * @param  coeffs: Pointer to filter coefficients structure
  * @param  input: Input audio sample
  * @retval Output audio sample
  */
float FilterDesign_ProcessSample(FilterState_t* state,
                               const FilterCoefficients_t* coeffs,
                               float input)
{
    float output;
    
    /* Apply difference equation: y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2] */
    output = coeffs->b[0] * input + coeffs->b[1] * state->x[0] + coeffs->b[2] * state->x[1]
           - coeffs->a[1] * state->y[0] - coeffs->a[2] * state->y[1];
    
    /* Update filter state */
    state->x[1] = state->x[0];
    state->x[0] = input;
    state->y[1] = state->y[0];
    state->y[0] = output;
    
    return output;
}

/**
  * @brief  Reset filter state to zeros
  * @param  state: Pointer to filter state structure
  * @retval None
  */
void FilterDesign_ResetState(FilterState_t* state)
{
    /* Clear filter state variables */
    state->x[0] = 0.0f;
    state->x[1] = 0.0f;
    state->y[0] = 0.0f;
    state->y[1] = 0.0f;
}

/**
  * @brief  Design a first-order filter (LP or HP only)
  * @param  coeffs: Pointer to filter coefficients structure
  * @param  filterType: Type of filter (LP or HP)
  * @param  freq: Cutoff frequency in Hz
  * @param  sampleFreq: Sampling frequency in Hz
  * @retval Filter design status (0 = success)
  */
int FilterDesign_CreateFirstOrder(FilterCoefficients_t* coeffs, 
                                 FilterType_t filterType,
                                 float freq, 
                                 float sampleFreq)
{
    /* Validate parameters */
    if (coeffs == NULL || freq <= 0 || sampleFreq <= 0) {
        return FILTER_DESIGN_ERROR_PARAMS;
    }
    
    /* Only LP and HP supported for first-order */
    if (filterType != FILTER_TYPE_LOWPASS && filterType != FILTER_TYPE_HIGHPASS) {
        return FILTER_DESIGN_ERROR_TYPE;
    }
    
    /* Calculate normalized frequency */
    float omega = 2.0f * M_PI * freq / sampleFreq;
    float theta = tanf(omega / 2.0f);
    
    /* Calculate coefficients based on filter type */
    if (filterType == FILTER_TYPE_LOWPASS) {
        float gamma = theta / (1.0f + theta);
        
        coeffs->b[0] = gamma;
        coeffs->b[1] = gamma;
        coeffs->b[2] = 0.0f;
        coeffs->a[0] = 1.0f;
        coeffs->a[1] = gamma - 1.0f;
        coeffs->a[2] = 0.0f;
    } else { /* FILTER_TYPE_HIGHPASS */
        float gamma = 1.0f / (1.0f + theta);
        
        coeffs->b[0] = gamma;
        coeffs->b[1] = -gamma;
        coeffs->b[2] = 0.0f;
        coeffs->a[0] = 1.0f;
        coeffs->a[1] = gamma * (1.0f - theta);
        coeffs->a[2] = 0.0f;
    }
    
    /* Store parameters */
    coeffs->w0 = freq;
    coeffs->Q = 0.5f; /* Standard first-order Q value */
    
    return FILTER_DESIGN_SUCCESS;
}

/* End of file */