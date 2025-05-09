# Panel Kontrol DSP STM32F411 untuk Sistem Audio Crossover Aktif

## Deskripsi Proyek

Proyek ini mengimplementasikan sistem Digital Signal Processing (DSP) untuk audio crossover aktif berbasis mikrokontroler STM32F411. Panel kontrol DSP ini dirancang untuk memberikan tingkat kontrol yang presisi terhadap parameter audio dengan antarmuka yang terinspirasi dari produk DMLS di pasaran. Sistem ini menawarkan 2 input dan 4 output dengan kemampuan pemrosesan audio yang komprehensif untuk berbagai kebutuhan audio, mulai dari instalasi mobil, home audio, hingga aplikasi pro audio skala kecil.

## Fitur Utama

### 📊 Konfigurasi Input/Output
- 2 input audio (stereo atau dual mono)
- 4 output audio yang dapat dikonfigurasi
- Matriks routing input-output yang fleksibel
- Summing/mixing input untuk setiap output

### 🎚️ Pengaturan Crossover
- Konfigurasi fleksibel: 2-way, 3-way, atau 4-way
- Filter: Butterworth, Linkwitz-Riley, Bessel
- Slope: 6dB/oct, 12dB/oct, 18dB/oct, 24dB/oct, 36dB/oct, 48dB/oct
- Frekuensi crossover: 20Hz - 20kHz

### 🔊 PEQ (Parametric Equalizer)
- 5-band PEQ per output
- Frekuensi: 20Hz - 20kHz
- Gain: -12dB sampai +12dB
- Q-factor: 0.1 - 10
- Filter types: Low Shelf, High Shelf, Bell, Low Pass, High Pass

### 📈 Kompresor & Limiter
- Threshold: -60dB sampai 0dB
- Ratio: 1:1 sampai 20:1
- Attack: 0.1ms - 100ms
- Release: 10ms - 1000ms
- Knee: soft/hard
- Limiter khusus untuk perlindungan sistem

### ⏱️ Delay & Fase
- Delay hingga 20ms per output
- Koreksi fase: 0°, 180° (phase invert)
- Pengaturan presisi per-milisecond untuk time alignment

### 🔊 Output Gain
- Gain: -80dB sampai +12dB
- Mute/unmute per output
- Link channel untuk pengaturan stereo

### 🎵 Preset & Memory
- 10 preset user yang dapat disimpan
- 5 preset pabrik untuk berbagai konfigurasi:
  - Standard 2-way
  - Standard 3-way
  - Subwoofer + Full range
  - Bi-amp monitor
  - Tri-amp sistem
- Penyimpanan parameter dalam memori non-volatile

### 📱 Antarmuka Pengguna
- OLED 0.91" 128x32 I2C untuk menampilkan informasi sistem
- Kontrol navigasi dengan rotary encoder
- 4 tombol cepat untuk akses fungsi utama
- Indikator level signal dengan LED

## Persyaratan Hardware

- Mikrokontroler STM32F411 (STM32F411CEU6 "Black Pill")
- OLED Display 0.91" 128x32 I2C (SSD1306)
- 1 buah PCM1808: ADC Audio (2-channel)
- 2 buah PCM5102A: DAC Audio (total 4-channel output)
- Rotary Encoder dengan push button
- 4 tombol fungsi untuk kontrol cepat
- 8 LED untuk indikator level dan status
- EEPROM I2C 24LC256 untuk penyimpanan preset
- Regulator tegangan untuk power supply stabil
- Komponen audio pasif (resistor, kapasitor, op-amp)
- Jack input/output audio (TRS 3.5mm atau XLR mini)

## Layouting Panel

```
┌───────────────────────────────────────────────┐
│                                               │
│  ┌─────────────────────────────────────────┐  │
│  │            OLED 128x32 DISPLAY          │  │
│  └─────────────────────────────────────────┘  │
│                                               │
│  [MENU]  [BACK]  [MUTE]  [PRESET]            │
│                                               │
│  ┌─┐                                          │
│  │▓│ ROTARY                        IN1  IN2   │
│  └─┘ ENCODER                       ○○   ○○    │
│                                               │
│                                 OUT1 OUT2 OUT3 OUT4
│                                 ○○   ○○   ○○   ○○  
│                                               │
│  VU: ●●●●●●●●                                 │
│                                               │
└───────────────────────────────────────────────┘
```

## Tampilan OLED & Navigasi

### Layar Utama
```
┌──────────────────┐
│IN1→OUT1  -3.5dB ▐█
│70Hz|LR24|COMP ON │
└──────────────────┘
```

### Menu Crossover
```
┌──────────────────┐
│XOVER:OUT1   70Hz │
│TYPE:LR  SLOPE:24 │
└──────────────────┘
```

### Menu EQ
```
┌──────────────────┐
│EQ1:1kHz  Q:0.7   │
│GAIN: +3.5dB      │
└──────────────────┘
```

### Menu Kompresor
```
┌──────────────────┐
│COMP:OUT2  -12dB  │
│RAT:4:1 A:15 R:150│
└──────────────────┘
```

## Instalasi dan Penggunaan

### Persiapan Development Environment
1. Install STM32CubeIDE
2. Clone repository ini
3. Buka proyek dengan STM32CubeIDE

### Membangun Proyek
1. Pastikan semua dependensi terinstal
2. Gunakan STM32CubeIDE untuk membangun proyek:
   - Klik kanan pada proyek
   - Pilih "Build Project"

### Mengupload Firmware ke STM32F411
1. Hubungkan board STM32F411 ke komputer dengan programmer ST-Link
2. Di STM32CubeIDE:
   - Klik kanan pada proyek
   - Pilih "Run As" → "STM32 C/C++ Application"
   - Atau untuk mode debugging, pilih "Debug As" → "STM32 C/C++ Application"

### Metode Upload Alternatif
1. **Menggunakan ST-Link Utility**:
   - Buka ST-Link Utility
   - Pilih file .bin dari folder Debug
   - Atur alamat awal ke 0x08000000
   - Klik "Program & Verify"

2. **Menggunakan Mode DFU** (jika board mendukung):
   - Set pin BOOT0 ke high untuk boot dari system memory
   - Reset board
   - Gunakan DfuSe atau dfu-util untuk upload firmware

## Penggunaan Panel Kontrol

### Menu Utama
Saat dinyalakan, sistem menampilkan layar utama dengan informasi input, output aktif, dan status.
- Putar encoder untuk berpindah antar output
- Tekan tombol MENU untuk masuk ke menu utama

### Navigasi Menu
1. Putar encoder untuk navigasi menu
2. Tekan encoder untuk memilih
3. Tekan BACK untuk kembali ke menu sebelumnya

### Pengaturan Input-Output Routing
1. Pilih "ROUTING" dari menu utama
2. Pilih output yang ingin dikonfigurasi 
3. Pilih sumber input (IN1, IN2, IN1+IN2)
4. Tekan encoder untuk konfirmasi

### Pengaturan Crossover
1. Pilih "XOVER" dari menu utama
2. Pilih output yang ingin diatur
3. Pilih parameter (Frequency, Type, Slope)
4. Atur nilai dengan memutar encoder
5. Tekan encoder untuk konfirmasi

### Pengaturan EQ
1. Pilih "PEQ" dari menu utama
2. Pilih output dan band EQ (1-5)
3. Atur Freq, Gain, dan Q-factor
4. Tekan encoder untuk konfirmasi

### Pengaturan Kompressor
1. Pilih "COMP" dari menu utama
2. Pilih output yang ingin diatur
3. Atur Threshold, Ratio, Attack, Release
4. Tekan tombol MUTE untuk bypass/enable

### Pengaturan Delay
1. Pilih "DELAY" dari menu utama
2. Pilih output yang ingin diatur
3. Atur delay dalam ms atau cm
4. Tekan encoder untuk konfirmasi

### Menyimpan/Memuat Preset
1. Tekan tombol PRESET untuk akses cepat menu preset
2. Pilih "SAVE" atau "LOAD"
3. Pilih slot preset (1-10)
4. Konfirmasi dengan menekan encoder

## Struktur Proyek

```
STM32F411_DMLS/
├── STM32F411_DMLS.ioc         # File konfigurasi STM32CubeMX
├── README.md                  # Dokumentasi proyek
│
├── Core/
│   ├── Inc/
│   │   ├── main.h            # Main header file
│   │   ├── gpio.h            # GPIO configuration
│   │   ├── dma.h             # DMA untuk audio streams
│   │   ├── i2c.h             # I2C untuk OLED dan EEPROM
│   │   ├── i2s.h             # I2S untuk audio codec
│   │   ├── spi.h             # SPI untuk storage
│   │   ├── tim.h             # Timers
│   │   ├── usart.h           # UART untuk debug
│   │   └── stm32f4xx_it.h    # Interrupt handlers
│   │
│   └── Src/
│       ├── main.c            # Entry point (250-350 baris)
│       ├── system_init.c     # Inisialisasi sistem (200-250 baris)
│       ├── gpio.c            # Implementasi GPIO (100-150 baris)
│       ├── i2c.c             # Implementasi I2C (100-150 baris)
│       ├── i2s.c             # Implementasi I2S (100-150 baris)
│       ├── spi.c             # Implementasi SPI (100-150 baris)
│       ├── tim.c             # Implementasi timer (100-150 baris)
│       ├── usart.c           # Implementasi UART (100-150 baris)
│       ├── dma.c             # Implementasi DMA (150-200 baris)
│       └── stm32f4xx_it.c    # Interrupt handlers (200-300 baris)
│
├── Drivers/                   # HAL dan CMSIS drivers (tidak dimodifikasi)
│
├── Audio/
│   ├── Inc/
│   │   ├── audio_config.h     # Konfigurasi audio umum
│   │   ├── audio_driver.h     # Interface driver audio
│   │   ├── audio_routing.h    # Definisi routing
│   │   ├── audio_processing.h # Rantai DSP
│   │   ├── audio_utils.h      # Fungsi utilitas
│   │   ├── codec_pcm1808.h    # Driver untuk ADC PCM1808
│   │   └── codec_pcm5102a.h   # Driver untuk DAC PCM5102A
│   │
│   └── Src/
│       ├── audio_config.c     # Implementasi konfigurasi (150-200 baris)
│       ├── audio_driver.c     # Implementasi driver (300-350 baris)
│       ├── audio_routing.c    # Implementasi routing (250-300 baris)
│       ├── audio_processing.c # Orchestrator proses DSP (300-400 baris)
│       ├── audio_utils.c      # Implementasi utilitas (200-250 baris)
│       ├── codec_pcm1808.c    # Implementasi ADC (200-250 baris)
│       └── codec_pcm5102a.c   # Implementasi DAC (200-250 baris)
│
├── DSP/
│   ├── Inc/
│   │   ├── dsp_common.h       # Definisi umum DSP
│   │   ├── crossover_types.h  # Tipe data crossover
│   │   ├── crossover.h        # Interface crossover
│   │   ├── peq_types.h        # Tipe data equalizer
│   │   ├── peq.h              # Interface parametric EQ
│   │   ├── compressor_types.h # Tipe data compressor
│   │   ├── compressor.h       # Interface compressor
│   │   ├── limiter_types.h    # Tipe data limiter
│   │   ├── limiter.h          # Interface limiter
│   │   ├── delay_types.h      # Tipe data delay
│   │   └── delay.h            # Interface delay
│   │
│   └── Src/
│       ├── dsp_common.c       # Implementasi fungsi DSP umum (200-250 baris)
│       ├── crossover_init.c   # Inisialisasi crossover (150-200 baris)
│       ├── crossover_filter.c # Filter crossover (300-350 baris)
│       ├── crossover_config.c # Konfigurasi crossover (200-250 baris)
│       ├── peq_init.c         # Inisialisasi EQ (150-200 baris)
│       ├── peq_filter.c       # Filter EQ (300-350 baris)
│       ├── peq_config.c       # Konfigurasi EQ (200-250 baris)
│       ├── compressor_init.c  # Inisialisasi kompresor (150-200 baris)
│       ├── compressor_proc.c  # Proses kompresor (300-350 baris)
│       ├── compressor_config.c # Konfigurasi kompresor (200-250 baris)
│       ├── limiter_init.c     # Inisialisasi limiter (100-150 baris)
│       ├── limiter_proc.c     # Proses limiter (250-300 baris)
│       ├── delay_init.c       # Inisialisasi delay (100-150 baris)
│       └── delay_proc.c       # Proses delay (200-250 baris)
│
├── Filter/
│   ├── Inc/
│   │   ├── filter_types.h     # Tipe data filter
│   │   ├── filter_design.h    # Interface desain filter
│   │   ├── iir_filter.h       # Interface IIR filter
│   │   ├── biquad.h           # Interface biquad filter
│   │   ├── butterworth.h      # Filter Butterworth
│   │   ├── linkwitz_riley.h   # Filter Linkwitz-Riley
│   │   └── bessel.h           # Filter Bessel
│   │
│   └── Src/
│       ├── filter_design.c    # Implementasi desain filter (300-400 baris)
│       ├── iir_filter.c       # Implementasi IIR filter (200-300 baris)
│       ├── biquad.c           # Implementasi biquad (200-300 baris)
│       ├── butterworth.c      # Implementasi Butterworth (250-350 baris)
│       ├── linkwitz_riley.c   # Implementasi L-R (250-350 baris)
│       └── bessel.c           # Implementasi Bessel (250-350 baris)
│
├── UI/
│   ├── Inc/
│   │   ├── ui_config.h        # Konfigurasi UI
│   │   ├── oled_driver.h      # Interface OLED SSD1306
│   │   ├── oled_graphics.h    # Library grafis
│   │   ├── rotary_encoder.h   # Interface encoder
│   │   ├── button_handler.h   # Interface button
│   │   ├── led_handler.h      # Interface LED
│   │   ├── menu_system.h      # Sistem menu
│   │   └── ui_pages.h         # Definisi halaman UI
│   │
│   └── Src/
│       ├── ui_config.c        # Implementasi konfigurasi UI (100-150 baris)
│       ├── oled_driver.c      # Driver OLED (200-250 baris)
│       ├── oled_graphics.c    # Fungsi grafis dasar (300-350 baris)
│       ├── oled_text.c        # Rendering teks (200-250 baris)
│       ├── oled_icons.c       # Rendering ikon (150-200 baris)
│       ├── rotary_encoder.c   # Implementasi encoder (150-200 baris)
│       ├── button_handler.c   # Implementasi button (150-200 baris)
│       ├── led_handler.c      # Implementasi LED (100-150 baris)
│       ├── menu_system.c      # Implementasi sistem menu (300-400 baris)
│       ├── menu_navigation.c  # Navigasi menu (200-250 baris)
│       ├── ui_pages_main.c    # Halaman utama UI (150-200 baris)
│       ├── ui_pages_xover.c   # Halaman crossover (200-250 baris)
│       ├── ui_pages_eq.c      # Halaman EQ (200-250 baris)
│       ├── ui_pages_comp.c    # Halaman kompressor (200-250 baris)
│       └── ui_pages_delay.c   # Halaman delay (150-200 baris)
│
├── Storage/
│   ├── Inc/
│   │   ├── storage_types.h    # Tipe data storage
│   │   ├── eeprom_driver.h    # Interface EEPROM
│   │   ├── preset_manager.h   # Interface preset manager
│   │   └── factory_presets.h  # Preset bawaan
│   │
│   └── Src/
│       ├── eeprom_driver.c    # Implementasi EEPROM (200-250 baris)
│       ├── preset_manager.c   # Manajemen preset (300-350 baris)
│       ├── preset_io.c        # Input/output preset (200-250 baris)
│       └── factory_presets.c  # Implementasi preset bawaan (250-300 baris)
│
├── Utils/
│   ├── Inc/
│   │   ├── debug.h            # Fungsi debugging
│   │   ├── math_utils.h       # Utilitas matematika
│   │   ├── buffer_manager.h   # Manajemen buffer
│   │   └── system_monitor.h   # Monitor sistem
│   │
│   └── Src/
│       ├── debug.c            # Implementasi debug (150-200 baris)
│       ├── math_utils.c       # Implementasi utilitas matematika (250-300 baris)
│       ├── buffer_manager.c   # Implementasi manajemen buffer (200-250 baris)
│       └── system_monitor.c   # Implementasi monitor sistem (150-200 baris)
│
└── build/                     # Output build folder
    └── AudioCrossover.bin     # Binary file untuk upload
```

## Diagram Blok DSP

```
┌─────────┐   ┌─────────┐   ┌─────────────────────────────────────┐
│ INPUT 1 │──→│         │   │ OUTPUT 1                            │
└─────────┘   │         │   │ ┌─────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ │
              │ ROUTING │──→│ │XOVER│→│ PEQ│→│COMP│→│DLIY│→│GAIN│ │
┌─────────┐   │ MATRIX  │   │ └─────┘ └────┘ └────┘ └────┘ └────┘ │
│ INPUT 2 │──→│         │   └─────────────────────────────────────┘
└─────────┘   │         │   
              │         │   ┌─────────────────────────────────────┐
              │         │   │ OUTPUT 2                            │
              │         │   │ ┌─────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ │
              │         │──→│ │XOVER│→│ PEQ│→│COMP│→│DLIY│→│GAIN│ │
              │         │   │ └─────┘ └────┘ └────┘ └────┘ └────┘ │
              │         │   └─────────────────────────────────────┘
              │         │   
              │         │   ┌─────────────────────────────────────┐
              │         │   │ OUTPUT 3                            │
              │         │   │ ┌─────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ │
              │         │──→│ │XOVER│→│ PEQ│→│COMP│→│DLIY│→│GAIN│ │
              │         │   │ └─────┘ └────┘ └────┘ └────┘ └────┘ │
              │         │   └─────────────────────────────────────┘
              │         │   
              │         │   ┌─────────────────────────────────────┐
              │         │   │ OUTPUT 4                            │
              │         │   │ ┌─────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ │
              │         │──→│ │XOVER│→│ PEQ│→│COMP│→│DLIY│→│GAIN│ │
              └─────────┘   │ └─────┘ └────┘ └────┘ └────┘ └────┘ │
                            └─────────────────────────────────────┘
```

## Konfigurasi Sistem Umum

### 2-way Stereo + Subwoofer
- OUT1: Left High (>80Hz)
- OUT2: Right High (>80Hz)
- OUT3: Left Low (80-500Hz)
- OUT4: Right Low + Subwoofer (<500Hz)

### 3-way Mono
- OUT1: High (>2.5kHz)
- OUT2: Mid (500Hz-2.5kHz)
- OUT3: Low (80-500Hz)
- OUT4: Subwoofer (<80Hz)

### Full Range + Sub
- OUT1: Left Full Range
- OUT2: Right Full Range
- OUT3: Mono Subwoofer
- OUT4: Mono Subwoofer (duplicate)

## Troubleshooting

### Masalah Umum

1. **Tidak Ada Suara**:
   - Periksa koneksi input dan output
   - Pastikan output tidak dalam keadaan mute
   - Periksa routing input ke output
   - Pastikan gain tidak terlalu rendah

2. **Suara Terdistorsi**:
   - Periksa level input (jangan sampai clipping)
   - Turunkan threshold limiter
   - Periksa pengaturan kompresor
   - Periksa apakah gain terlalu tinggi

3. **OLED Tidak Menampilkan Informasi**:
   - Periksa koneksi I2C ke OLED
   - Pastikan alamat I2C OLED benar (biasanya 0x3C atau 0x3D)
   - Reset system

4. **Pengaturan Tidak Tersimpan**:
   - Periksa koneksi EEPROM
   - Pastikan proses save preset selesai sebelum power-off
   - Reset sistem dan coba simpan kembali

### Batas Kemampuan

STM32F411 memiliki keterbatasan sumber daya:
- Memori RAM terbatas: 128KB
- Flash terbatas: 512KB
- Beberapa DSP kompleks mungkin perlu dioptimasi

## Pengembangan Lebih Lanjut

Beberapa area yang dapat dikembangkan:

1. **Konektivitas**: Tambahan Bluetooth untuk kontrol via smartphone
2. **USB Audio**: Implementasi audio USB untuk koneksi ke komputer
3. **Display Upgrade**: Dukungan untuk OLED yang lebih besar (128x64)
4. **RTA**: Real-time analyzer dengan microphone input
5. **Remote Control**: Aplikasi mobile untuk pengaturan jarak jauh

## Kontribusi

Kami sangat menghargai kontribusi untuk pengembangan proyek ini. Silakan mengikuti langkah-langkah berikut:

1. Fork repositori ini
2. Buat branch fitur baru (`git checkout -b fitur-baru`)
3. Commit perubahan Anda (`git commit -m 'Menambahkan fitur baru'`)
4. Push ke branch (`git push origin fitur-baru`)
5. Buat Pull Request

## Lisensi

Proyek ini dilisensikan di bawah lisensi MIT. Lihat file LICENSE untuk informasi lebih lanjut.

## Kontak

Untuk pertanyaan atau dukungan, silakan hubungi:
- Email: [dsp.audio.project@gmail.com]
- Instagram: [@dsp_audio_project]

---

Dikembangkan dengan ❤️ untuk komunitas audio Indonesia
