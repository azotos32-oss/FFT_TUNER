// blink_any.c
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include <stdio.h>
#include "fft.h"
#include "_kiss_fft_guts.h"
#include "kiss_fft.h"
#include "kiss_fftr.h"
#include <stdint.h>

#define buffer_size FSAMP
#define BIN_COUNT 500


uint8_t buffer[buffer_size]; 
int start_index = 0;

frequency_bin_t bins[BIN_COUNT];

void make_bins(){
    for(int q=0;q<(BIN_COUNT-1);q++){
        bins[q].name = "bin";
        bins[q].freq_min = q;
        bins[q].freq_max = q+1;   // 1 Hz width
        bins[q].amplitude = 0;
    }
}


int highest_bin_amplitude_index(){
    int index = 0;
    float highest_amplitude = 0;
    for(int q=0;q<(BIN_COUNT-1);q++){

        if(bins[q].amplitude > highest_amplitude){
            highest_amplitude = bins[q].amplitude;
            index = q;
        }
    }
    return index;
}

int second_highest_bin_amplitude_index()
{
    int max1 = -1;        // index of highest
    int max2 = -1;        // index of second highest
    float amp1 = -1.0f;   // highest amplitude
    float amp2 = -1.0f;   // second highest amplitude
    for (int q = 0; q < BIN_COUNT - 1; q++)
    {
        float A = bins[q].amplitude;

        if (A > amp1)
        {
            // Shift previous highest down to second highest
            amp2 = amp1;
            max2 = max1;

            amp1 = A;
            max1 = q;
        }
        else if (A > amp2)
        {
            // Only update second highest
            amp2 = A;
            max2 = q;
        }
    }
    return max2;  // Returns -1 if fewer than 2 unique bins
}


void print_all_bins(){
    for(int q=0;q<(BIN_COUNT-1);q++){
        printf("Bin %d: Freq %d-%d Hz, Amplitude: %f\n", q, bins[q].freq_min, bins[q].freq_max, bins[q].amplitude);
    }
}



int main() {
    
    fft_setup();

    sleep_ms(3000);
    printf("FFT Setup Complete\n");
    while (true) {
        fft_sample(buffer);

        make_bins();
        fft_process(buffer, bins, BIN_COUNT);
        int index = highest_bin_amplitude_index();
        int index2 = second_highest_bin_amplitude_index();
        float freq = 0; // 1 Hz per bin
        if (index < 109){
            if (index < index2){
                freq = index * 4;
            } else{
                freq = index * (4.0f / 3.0f);
            }
        } else {
            freq = index * 4;
        }
        printf("Second Dominant Frequency: %d Hz with Amplitude: \n", index2);
        printf("Dominant Frequency: %d Hz with Amplitude: %f\n", index, bins[index].amplitude);
        printf("Estimated Frequency: %f Hz\n", freq);
        printf("-----------------------------------------------------------------------\n");
    }
    return 0;
}
