/**
  ******************************************************************************
  * @file           : audio_routing.c
  * @brief          : Implementation of audio routing functions
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Panel Kontrol DSP STM32F411 untuk Sistem Audio Crossover Aktif
  * Routing matrix for 2-input, 4-output DSP processing system
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "audio_routing.h"
#include "debug.h"
#include "math_utils.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define INPUT_GAIN_MAX         4.0f   /* Maximum input gain (linear) */
#define INPUT_GAIN_MIN         0.0f   /* Minimum input gain (linear) */
#define MIX_LEVEL_MAX          1.0f   /* Maximum mix level */
#define MIX_LEVEL_MIN          0.0f   /* Minimum mix level */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static AudioRouting_TypeDef audioRoutingConfig;

/* Source name strings for UI display */
static const char* sourceNames[] = {
  "NONE",
  "IN1",
  "IN2",
  "IN1+IN2",
  "IN1 L",
  "IN1 R",
  "IN2 L",
  "IN2 R"
};

/* Private function prototypes -----------------------------------------------*/
static void ApplyInputGain(AudioBuffer_TypeDef *buffer);
static void ProcessChannelRouting(uint8_t outputChannel, AudioBuffer_TypeDef *inputBuffer, AudioBuffer_TypeDef *outputBuffer);
static void ValidateRoutingConfig(void);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize the audio routing matrix
  * @param  None
  * @retval None
  */
void AudioRouting_Init(void)
{
  /* Set default sources */
  for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
    /* Default routing: OUT1=IN1, OUT2=IN2, OUT3=IN1, OUT4=IN2 */
    audioRoutingConfig.source[i] = (i % 2 == 0) ? AUDIO_SOURCE_IN1 : AUDIO_SOURCE_IN2;
    
    /* Default mix level: 0.5 (50/50 mix when using IN1+IN2) */
    audioRoutingConfig.mixLevel[i] = 0.5f;
    
    /* Default: unmuted */
    audioRoutingConfig.outputMute[i] = 0;
  }
  
  /* Default input gain: 1.0 (0dB) */
  for (uint8_t i = 0; i < AUDIO_INPUT_CHANNELS; i++) {
    audioRoutingConfig.inputGain[i] = 1.0f;
  }
  
  /* Default: stereo linked */
  for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS/2; i++) {
    audioRoutingConfig.stereoLink[i] = 1;
  }
  
  /* Default: stereo (no mono sum) */
  audioRoutingConfig.monoSumInputs = 0;
  
  DEBUG_PRINT("Audio routing initialized with default config\r\n");
}

/**
  * @brief  Configure routing for a specific output channel
  * @param  outputChannel: Output channel number (0 to AUDIO_OUTPUT_CHANNELS-1)
  * @param  source: Audio source to connect
  * @retval None
  */
void AudioRouting_ConfigOutput(uint8_t outputChannel, AudioSource_TypeDef source)
{
  /* Check bounds */
  if (outputChannel >= AUDIO_OUTPUT_CHANNELS || source >= AUDIO_SOURCE_MAX) {
    DEBUG_PRINT("Error: Invalid output channel or source\r\n");
    return;
  }
  
  /* Check if channel is stereo linked */
  uint8_t pairIndex = outputChannel / 2;
  
  /* Set routing for this channel */
  audioRoutingConfig.source[outputChannel] = source;
  
  /* If stereo linked, set matching source for the paired channel */
  if (audioRoutingConfig.stereoLink[pairIndex]) {
    uint8_t pairedChannel = (outputChannel % 2 == 0) ? outputChannel + 1 : outputChannel - 1;
    
    /* For stereo links, we need to map sources appropriately */
    AudioSource_TypeDef pairedSource;
    
    switch (source) {
      case AUDIO_SOURCE_IN1:
        pairedSource = AUDIO_SOURCE_IN2;
        break;
      case AUDIO_SOURCE_IN2:
        pairedSource = AUDIO_SOURCE_IN1;
        break;
      case AUDIO_SOURCE_IN1_ONLY_LEFT:
        pairedSource = AUDIO_SOURCE_IN1_ONLY_RIGHT;
        break;
      case AUDIO_SOURCE_IN1_ONLY_RIGHT:
        pairedSource = AUDIO_SOURCE_IN1_ONLY_LEFT;
        break;
      case AUDIO_SOURCE_IN2_ONLY_LEFT:
        pairedSource = AUDIO_SOURCE_IN2_ONLY_RIGHT;
        break;
      case AUDIO_SOURCE_IN2_ONLY_RIGHT:
        pairedSource = AUDIO_SOURCE_IN2_ONLY_LEFT;
        break;
      default:
        pairedSource = source;
        break;
    }
    
    audioRoutingConfig.source[pairedChannel] = pairedSource;
  }
  
  DEBUG_PRINT("Output %d configured with source %s\r\n", 
              outputChannel, sourceNames[source]);
}

/**
  * @brief  Configure input gain for a specific input channel
  * @param  inputChannel: Input channel number (0 to AUDIO_INPUT_CHANNELS-1)
  * @param  gain: Gain value (0.0 to 4.0)
  * @retval None
  */
void AudioRouting_SetInputGain(uint8_t inputChannel, float gain)
{
  /* Check bounds */
  if (inputChannel >= AUDIO_INPUT_CHANNELS) {
    DEBUG_PRINT("Error: Invalid input channel\r\n");
    return;
  }
  
  /* Limit to valid range */
  gain = LIMIT_FLOAT(gain, INPUT_GAIN_MIN, INPUT_GAIN_MAX);
  
  /* Set gain */
  audioRoutingConfig.inputGain[inputChannel] = gain;
  
  DEBUG_PRINT("Input %d gain set to %.2f\r\n", inputChannel, gain);
}

/**
  * @brief  Set mix level for an output when mixing inputs
  * @param  outputChannel: Output channel number (0 to AUDIO_OUTPUT_CHANNELS-1)
  * @param  level: Mix level (0.0 to 1.0)
  * @retval None
  */
void AudioRouting_SetMixLevel(uint8_t outputChannel, float level)
{
  /* Check bounds */
  if (outputChannel >= AUDIO_OUTPUT_CHANNELS) {
    DEBUG_PRINT("Error: Invalid output channel\r\n");
    return;
  }
  
  /* Limit to valid range */
  level = LIMIT_FLOAT(level, MIX_LEVEL_MIN, MIX_LEVEL_MAX);
  
  /* Set mix level */
  audioRoutingConfig.mixLevel[outputChannel] = level;
  
  /* Apply to stereo linked channel if applicable */
  uint8_t pairIndex = outputChannel / 2;
  if (audioRoutingConfig.stereoLink[pairIndex]) {
    uint8_t pairedChannel = (outputChannel % 2 == 0) ? outputChannel + 1 : outputChannel - 1;
    audioRoutingConfig.mixLevel[pairedChannel] = level;
  }
  
  DEBUG_PRINT("Output %d mix level set to %.2f\r\n", outputChannel, level);
}

/**
  * @brief  Set mute state for an output channel
  * @param  outputChannel: Output channel number (0 to AUDIO_OUTPUT_CHANNELS-1)
  * @param  state: Mute state (0=unmuted, 1=muted)
  * @retval None
  */
void AudioRouting_SetMute(uint8_t outputChannel, uint8_t state)
{
  /* Check bounds */
  if (outputChannel >= AUDIO_OUTPUT_CHANNELS) {
    DEBUG_PRINT("Error: Invalid output channel\r\n");
    return;
  }
  
  /* Set mute state */
  audioRoutingConfig.outputMute[outputChannel] = state ? 1 : 0;
  
  /* Apply to stereo linked channel if applicable */
  uint8_t pairIndex = outputChannel / 2;
  if (audioRoutingConfig.stereoLink[pairIndex]) {
    uint8_t pairedChannel = (outputChannel % 2 == 0) ? outputChannel + 1 : outputChannel - 1;
    audioRoutingConfig.outputMute[pairedChannel] = state ? 1 : 0;
  }
  
  DEBUG_PRINT("Output %d mute set to %d\r\n", outputChannel, state);
}

/**
  * @brief  Set stereo link for channel pair
  * @param  pairIndex: Pair index (0 to AUDIO_OUTPUT_CHANNELS/2-1)
  * @param  state: Link state (0=independent, 1=linked)
  * @retval None
  */
void AudioRouting_SetStereoLink(uint8_t pairIndex, uint8_t state)
{
  /* Check bounds */
  if (pairIndex >= AUDIO_OUTPUT_CHANNELS/2) {
    DEBUG_PRINT("Error: Invalid pair index\r\n");
    return;
  }
  
  /* Set stereo link state */
  audioRoutingConfig.stereoLink[pairIndex] = state ? 1 : 0;
  
  DEBUG_PRINT("Channel pair %d stereo link set to %d\r\n", pairIndex, state);
}

/**
  * @brief  Set mono sum mode for inputs
  * @param  state: Mono sum state (0=stereo, 1=mono)
  * @retval None
  */
void AudioRouting_SetMonoSum(uint8_t state)
{
  /* Set mono sum state */
  audioRoutingConfig.monoSumInputs = state ? 1 : 0;
  
  DEBUG_PRINT("Mono sum mode set to %d\r\n", state);
}

/**
  * @brief  Process audio through the routing matrix
  * @param  inputBuffer: Pointer to input buffer
  * @param  outputBuffer: Pointer to output buffer
  * @retval None
  */
void AudioRouting_Process(AudioBuffer_TypeDef *inputBuffer, AudioBuffer_TypeDef *outputBuffer)
{
  /* Apply input gains */
  ApplyInputGain(inputBuffer);
  
  /* Apply mono summing if enabled */
  if (audioRoutingConfig.monoSumInputs) {
    /* For each sample in the buffer */
    for (uint16_t i = 0; i < inputBuffer->bufferSize; i++) {
      float monoSum = 0.0f;
      
      /* Sum all input channels */
      for (uint8_t ch = 0; ch < AUDIO_INPUT_CHANNELS; ch++) {
        monoSum += inputBuffer->samples[ch][i];
      }
      
      /* Divide by number of channels and apply to all inputs */
      monoSum /= AUDIO_INPUT_CHANNELS;
      for (uint8_t ch = 0; ch < AUDIO_INPUT_CHANNELS; ch++) {
        inputBuffer->samples[ch][i] = monoSum;
      }
    }
  }
  
  /* Process each output channel according to routing configuration */
  for (uint8_t ch = 0; ch < AUDIO_OUTPUT_CHANNELS; ch++) {
    ProcessChannelRouting(ch, inputBuffer, outputBuffer);
  }
}

/**
  * @brief  Get current routing configuration
  * @param  config: Pointer to configuration structure
  * @retval None
  */
void AudioRouting_GetConfig(AudioRouting_TypeDef *config)
{
  if (config == NULL) {
    DEBUG_PRINT("Error: NULL config pointer\r\n");
    return;
  }
  
  /* Copy current configuration */
  *config = audioRoutingConfig;
}

/**
  * @brief  Set routing configuration
  * @param  config: Pointer to configuration structure
  * @retval None
  */
void AudioRouting_SetConfig(AudioRouting_TypeDef *config)
{
  if (config == NULL) {
    DEBUG_PRINT("Error: NULL config pointer\r\n");
    return;
  }
  
  /* Copy new configuration */
  audioRoutingConfig = *config;
  
  /* Validate the configuration */
  ValidateRoutingConfig();
  
  DEBUG_PRINT("Audio routing configuration updated\r\n");
}

/**
  * @brief  Reset routing to default configuration
  * @param  None
  * @retval None
  */
void AudioRouting_Reset(void)
{
  /* Re-initialize with defaults */
  AudioRouting_Init();
  
  DEBUG_PRINT("Audio routing reset to defaults\r\n");
}

/**
  * @brief  Get source name string for UI display
  * @param  source: Source enum value
  * @retval Pointer to source name string
  */
const char* AudioRouting_GetSourceName(AudioSource_TypeDef source)
{
  /* Validate source index */
  if (source >= AUDIO_SOURCE_MAX) {
    return "INVALID";
  }
  
  return sourceNames[source];
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Apply configured input gain to input buffer
  * @param  buffer: Pointer to input buffer
  * @retval None
  */
static void ApplyInputGain(AudioBuffer_TypeDef *buffer)
{
  /* Apply gain to each input channel */
  for (uint8_t ch = 0; ch < AUDIO_INPUT_CHANNELS; ch++) {
    float gain = audioRoutingConfig.inputGain[ch];
    
    /* Apply gain to all samples */
    for (uint16_t i = 0; i < buffer->bufferSize; i++) {
      buffer->samples[ch][i] *= gain;
    }
  }
}

/**
  * @brief  Process routing for a specific output channel
  * @param  outputChannel: Output channel number
  * @param  inputBuffer: Pointer to input buffer
  * @param  outputBuffer: Pointer to output buffer
  * @retval None
  */
static void ProcessChannelRouting(uint8_t outputChannel, AudioBuffer_TypeDef *inputBuffer, AudioBuffer_TypeDef *outputBuffer)
{
  /* Check if channel is muted */
  if (audioRoutingConfig.outputMute[outputChannel]) {
    /* If muted, output zeros */
    for (uint16_t i = 0; i < outputBuffer->bufferSize; i++) {
      outputBuffer->samples[outputChannel][i] = 0.0f;
    }
    return;
  }
  
  /* Get source for this output channel */
  AudioSource_TypeDef source = audioRoutingConfig.source[outputChannel];
  
  /* Process based on source type */
  switch (source) {
    case AUDIO_SOURCE_NONE:
      /* Output silence */
      for (uint16_t i = 0; i < outputBuffer->bufferSize; i++) {
        outputBuffer->samples[outputChannel][i] = 0.0f;
      }
      break;
    
    case AUDIO_SOURCE_IN1:
      /* Copy IN1 to output */
      for (uint16_t i = 0; i < outputBuffer->bufferSize; i++) {
        outputBuffer->samples[outputChannel][i] = inputBuffer->samples[0][i];
      }
      break;
    
    case AUDIO_SOURCE_IN2:
      /* Copy IN2 to output */
      for (uint16_t i = 0; i < outputBuffer->bufferSize; i++) {
        outputBuffer->samples[outputChannel][i] = inputBuffer->samples[1][i];
      }
      break;
    
    case AUDIO_SOURCE_IN1_IN2_MIX:
      /* Mix IN1 and IN2 based on mix level */
      {
        float mixLevel = audioRoutingConfig.mixLevel[outputChannel];
        float in1Weight = mixLevel;
        float in2Weight = 1.0f - mixLevel;
        
        for (uint16_t i = 0; i < outputBuffer->bufferSize; i++) {
          outputBuffer->samples[outputChannel][i] = 
            (inputBuffer->samples[0][i] * in1Weight) + 
            (inputBuffer->samples[1][i] * in2Weight);
        }
      }
      break;
    
    case AUDIO_SOURCE_IN1_ONLY_LEFT:
      /* For 2-channel input, assume first channel is left */
      if (AUDIO_INPUT_CHANNELS >= 2) {
        for (uint16_t i = 0; i < outputBuffer->bufferSize; i++) {
          outputBuffer->samples[outputChannel][i] = inputBuffer->samples[0][i];
        }
      } else {
        /* Fallback to IN1 if not multi-channel */
        for (uint16_t i = 0; i < outputBuffer->bufferSize; i++) {
          outputBuffer->samples[outputChannel][i] = inputBuffer->samples[0][i];
        }
      }
      break;
    
    case AUDIO_SOURCE_IN1_ONLY_RIGHT:
      /* For 2-channel input, assume second channel is right */
      if (AUDIO_INPUT_CHANNELS >= 2) {
        for (uint16_t i = 0; i < outputBuffer->bufferSize; i++) {
          outputBuffer->samples[outputChannel][i] = inputBuffer->samples[0][i];
        }
      } else {
        /* Fallback to IN1 if not multi-channel */
        for (uint16_t i = 0; i < outputBuffer->bufferSize; i++) {
          outputBuffer->samples[outputChannel][i] = inputBuffer->samples[0][i];
        }
      }
      break;
    
    case AUDIO_SOURCE_IN2_ONLY_LEFT:
      /* For 2-channel input, assume first channel is left */
      if (AUDIO_INPUT_CHANNELS >= 2) {
        for (uint16_t i = 0; i < outputBuffer->bufferSize; i++) {
          outputBuffer->samples[outputChannel][i] = inputBuffer->samples[1][i];
        }
      } else {
        /* Fallback to IN2 if not multi-channel */
        for (uint16_t i = 0; i < outputBuffer->bufferSize; i++) {
          outputBuffer->samples[outputChannel][i] = inputBuffer->samples[1][i];
        }
      }
      break;
    
    case AUDIO_SOURCE_IN2_ONLY_RIGHT:
      /* For 2-channel input, assume second channel is right */
      if (AUDIO_INPUT_CHANNELS >= 2) {
        for (uint16_t i = 0; i < outputBuffer->bufferSize; i++) {
          outputBuffer->samples[outputChannel][i] = inputBuffer->samples[1][i];
        }
      } else {
        /* Fallback to IN2 if not multi-channel */
        for (uint16_t i = 0; i < outputBuffer->bufferSize; i++) {
          outputBuffer->samples[outputChannel][i] = inputBuffer->samples[1][i];
        }
      }
      break;
    
    default:
      /* Invalid source, output silence */
      for (uint16_t i = 0; i < outputBuffer->bufferSize; i++) {
        outputBuffer->samples[outputChannel][i] = 0.0f;
      }
      break;
  }
}

/**
  * @brief  Validate routing configuration and correct invalid settings
  * @param  None
  * @retval None
  */
static void ValidateRoutingConfig(void)
{
  /* Check sources */
  for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
    if (audioRoutingConfig.source[i] >= AUDIO_SOURCE_MAX) {
      audioRoutingConfig.source[i] = AUDIO_SOURCE_NONE;
    }
  }
  
  /* Check input gains */
  for (uint8_t i = 0; i < AUDIO_INPUT_CHANNELS; i++) {
    audioRoutingConfig.inputGain[i] = LIMIT_FLOAT(audioRoutingConfig.inputGain[i], 
                                                INPUT_GAIN_MIN, 
                                                INPUT_GAIN_MAX);
  }
  
  /* Check mix levels */
  for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
    audioRoutingConfig.mixLevel[i] = LIMIT_FLOAT(audioRoutingConfig.mixLevel[i], 
                                               MIX_LEVEL_MIN, 
                                               MIX_LEVEL_MAX);
  }
  
  /* Ensure mute flags are binary */
  for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
    audioRoutingConfig.outputMute[i] = audioRoutingConfig.outputMute[i] ? 1 : 0;
  }
  
  /* Ensure stereo link flags are binary */
  for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS/2; i++) {
    audioRoutingConfig.stereoLink[i] = audioRoutingConfig.stereoLink[i] ? 1 : 0;
  }
  
  /* Ensure mono sum flag is binary */
  audioRoutingConfig.monoSumInputs = audioRoutingConfig.monoSumInputs ? 1 : 0;
}