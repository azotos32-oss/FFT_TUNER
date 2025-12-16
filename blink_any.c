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
#include "hardware/i2c.h"

#define OLED_ADDR 0x3C
#define buffer_size FSAMP
#define BIN_COUNT 500
#define OLED_WIDTH 128
#define OLED_HEIGHT 32
#define OLED_PAGES (OLED_HEIGHT / 8)


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


int highest_bin_amplitude_index()
{
    int index = -1;
    float highest_amplitude = 0.0f;

    for (int q = 15; q < BIN_COUNT - 1; q++)
    {
        // Exclude bins 38–44 and 96–102
        if ((q >= 38 && q <= 44) || (q >= 96 && q <= 102))
            continue;

        if (bins[q].amplitude > highest_amplitude)
        {
            highest_amplitude = bins[q].amplitude;
            index = q;
        }
    }

    return index; // returns -1 if no valid bin found
}


int second_highest_bin_amplitude_index(int highest_index)
{
    int max2 = -1;        // index of second highest
    float amp2 = -1.0f;   // second highest amplitude

    for (int q = 16; q < BIN_COUNT - 1; q++)
    {
        // Skip the highest bin itself
        if (q == highest_index)
            continue;

        // Skip bins within ±5 of the highest bin
        if (q >= highest_index - 5 && q <= highest_index + 5)
            continue;

        float A = bins[q].amplitude;

        if (A > amp2)
        {
            amp2 = A;
            max2 = q;
        }
    }

    return max2; // Returns -1 if no valid second highest exists
}


void print_all_bins(){
    for(int q=0;q<(BIN_COUNT-1);q++){
        printf("Bin %d: Freq %d-%d Hz, Amplitude: %f\n", q, bins[q].freq_min, bins[q].freq_max, bins[q].amplitude);
    }
}

// ------------------- Basic OLED commands -------------------
static void oled_cmd(i2c_inst_t *i2c, uint8_t cmd) {
    uint8_t data[2] = {0x00, cmd};
    i2c_write_blocking(i2c, OLED_ADDR, data, 2, false);
}



int main() {
    
    fft_setup();

    i2c_init(i2c0, 100 * 1000);
    gpio_set_function(16, GPIO_FUNC_I2C);
    gpio_set_function(17, GPIO_FUNC_I2C);
    gpio_pull_up(16);
    gpio_pull_up(17);

    sleep_ms(100);

    // Minimal init

    // OLED init
    oled_cmd(i2c0, 0xAE); // display off
    oled_cmd(i2c0, 0x8D); oled_cmd(i2c0, 0x14); // charge pump
    oled_cmd(i2c0, 0xA0); // SEG0 → column 0
    oled_cmd(i2c0, 0xC8); // COM scan reversed
    oled_cmd(i2c0, 0xA4); // ALL ON mode
    oled_cmd(i2c0, 0xA6); // normal display

    // Clear framebuffer and draw letter A centered

    oled_cmd(i2c0, 0xAF); // display on


    sleep_ms(3000);
    printf("FFT Setup Complete\n");


    while (true) {
        fft_sample(buffer);

        make_bins();
        fft_process(buffer, bins, BIN_COUNT);
        int index = highest_bin_amplitude_index();
        int index2 = second_highest_bin_amplitude_index(index);
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
        if (index < 50.0f){
            freq = index * 4.0f;
        }
        printf("-----------------------------------------------------------------------\n");
        printf("Second Dominant Frequency: %d Hz with Amplitude: \n", index2);
        printf("Dominant Frequency: %d Hz with Amplitude: %f\n", index, bins[index].amplitude);
        printf("Estimated Frequency: %f Hz\n", freq);
        printf("-----------------------------------------------------------------------\n");
        
    }
    return 0;
}