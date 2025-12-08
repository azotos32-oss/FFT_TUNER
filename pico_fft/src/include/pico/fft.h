#ifndef FFT_H
#define FFT_H

#include "pico/stdlib.h"
#include "pico/kiss_fftr.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

#include <stdio.h>
#include <math.h>


#define FSAMP 8000
#define CLOCK_DIV   (48000000.0f / FSAMP)

#define CAPTURE_CHANNEL 2
#define NSAMP 2000

typedef struct {
    const char *name;
    int freq_min;
    int freq_max;
    float amplitude;
} frequency_bin_t;

void fft_setup();
void fft_sample(uint8_t *capture_buf);
void fft_process(uint8_t *capture_buf, frequency_bin_t *bins, int bin_count);

#endif /* FFT_H */
