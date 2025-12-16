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

void oled_init() {
    const uint8_t init_sequence[] = {
        0xAE,             // display off
        0x20, 0x00,       // memory addressing mode 0=horizontal
        0xB0,             // set page start address
        0xC8,             // COM scan direction
        0x00, 0x10,       // column address
        0x40,             // start line
        0x81, 0x7F,       // contrast
        0xA1,             // segment remap
        0xA6,             // normal display
        0xA8, 0x1F,       // multiplex 32
        0xA4,             // display all on resume
        0xD3, 0x00,       // display offset
        0xD5, 0xF0,       // display clock
        0xD9, 0x22,       // pre-charge
        0xDA, 0x02,       // COM pins
        0xDB, 0x20,       // vcom detect
        0x8D, 0x14,       // charge pump
        0xAF              // display on
    };

    uint8_t buf[2];
    for (int i = 0; i < sizeof(init_sequence); i++) {
        buf[0] = 0x00;  // command
        buf[1] = init_sequence[i];
        i2c_write_blocking(i2c0, OLED_ADDR, buf, 2, false);
    }
}

uint8_t display_buffer[128 * 4]; // 128x32 / 8 = 4 pages

void oled_draw_pixel(int x, int y, bool on) {
    if (x < 0 || x >= 128 || y < 0 || y >= 32) return;
    int page = y / 8;
    int bit = y % 8;
    if (on)
        display_buffer[x + page * 128] |= (1 << bit);
    else
        display_buffer[x + page * 128] &= ~(1 << bit);
}

void oled_show() {
    uint8_t buf[129];
    buf[0] = 0x40; // data prefix
    for (int page = 0; page < 4; page++) {
        // set page address
        uint8_t cmd[3] = {0x00, 0xB0 | page, 0x00};
        i2c_write_blocking(i2c0, OLED_ADDR, cmd, 3, false);

        for (int col = 0; col < 128; col += 128) {
            memcpy(buf + 1, &display_buffer[page * 128], 128);
            i2c_write_blocking(i2c0, OLED_ADDR, buf, 129, false);
        }
    }
}

const uint32_t A_bitmap[32] = {
    0b00000000001111111100000000000000,
    0b00000000011111111110000000000000,
    0b00000000111111111111000000000000,
    0b00000001111100011111100000000000,
    0b00000011111000001111110000000000,
    0b00000111110000000111111000000000,
    0b00001111100000000011111100000000,
    0b00011111000000000001111110000000,
    0b00111110000000000000111111000000,
    0b01111100000000000000011111100000,
    0b11111000000000000000001111110000,
    0b11111111111111111111111111110000,
    0b11111111111111111111111111110000,
    0b11111000000000000000000011110000,
    0b11111000000000000000000011110000,
    0b11111000000000000000000011110000,
    0b11111000000000000000000011110000,
    0b11111000000000000000000011110000,
    0b11111000000000000000000011110000,
    0b11111000000000000000000011110000,
    0b11111000000000000000000011110000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000
};

const uint32_t E_bitmap[32] = {
    0b11111111111111111111111111110000,
    0b11111111111111111111111111110000,
    0b11110000000000000000000000000000,
    0b11110000000000000000000000000000,
    0b11110000000000000000000000000000,
    0b11110000000000000000000000000000,
    0b11110000000000000000000000000000,
    0b11110000000000000000000000000000,
    0b11110000000000000000000000000000,
    0b11110000000000000000000000000000,
    0b11111111111111111111111111110000,
    0b11111111111111111111111111110000,
    0b11110000000000000000000000000000,
    0b11110000000000000000000000000000,
    0b11110000000000000000000000000000,
    0b11110000000000000000000000000000,
    0b11110000000000000000000000000000,
    0b11110000000000000000000000000000,
    0b11110000000000000000000000000000,
    0b11111111111111111111111111110000,
    0b11111111111111111111111111110000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000
};

const uint32_t D_bitmap[32] = {
    0b11111111111111111111111100000000,
    0b11111111111111111111111100000000,
    0b11110000000000000000111111110000,
    0b11110000000000000000111111110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000111111110000,
    0b11110000000000000000111111110000,
    0b11111111111111111111111100000000,
    0b11111111111111111111111100000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000
};

const uint32_t B_bitmap[32] = {
    0b11111111111111111111111111100000,
    0b11111111111111111111111111100000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11111111111111111111111111100000,
    0b11111111111111111111111111100000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11110000000000000000000011110000,
    0b11111111111111111111111111100000,
    0b11111111111111111111111111100000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000
};

const uint32_t G_bitmap[32] = {
    0b00000111111111111111111000000000,
    0b00011111111111111111111110000000,
    0b00111100000000000000011111000000,
    0b01111000000000000000001111100000,
    0b11110000000000000000000111110000,
    0b11100000000000000000000011110000,
    0b11100000000000000000000000000000,
    0b11100000000000000000000000000000,
    0b11100000000000000000000000000000,
    0b11100000000000011111111111100000,
    0b11100000000000111111111111100000,
    0b11100000000000111111111111100000,
    0b11100000000000000000000111110000,
    0b11100000000000000000000011110000,
    0b11100000000000000000000011110000,
    0b11100000000000000000000011110000,
    0b11100000000000000000000011110000,
    0b11110000000000000000000111110000,
    0b01111000000000000000001111100000,
    0b00111100000000000000011111000000,
    0b00011111111111111111111110000000,
    0b00000111111111111111111000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000
};

const uint32_t Z_bitmap[32] = {
    0b11111111111111111111111111110000,
    0b11111111111111111111111111110000,
    0b00000000000000000000000011110000,
    0b00000000000000000000001111000000,
    0b00000000000000000000111100000000,
    0b00000000000000000011110000000000,
    0b00000000000000000111100000000000,
    0b00000000000000001111000000000000,
    0b00000000000000011110000000000000,
    0b00000000000000111100000000000000,
    0b00000000000001111000000000000000,
    0b00000000000011110000000000000000,
    0b00000000000111100000000000000000,
    0b00000000001111000000000000000000,
    0b00000000011110000000000000000000,
    0b00000000111100000000000000000000,
    0b00000001111000000000000000000000,
    0b00000011110000000000000000000000,
    0b00000111100000000000000000000000,
    0b00001111000000000000000000000000,
    0b00111100000000000000000000000000,
    0b11111111111111111111111111110000,
    0b11111111111111111111111111110000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000
};

const uint32_t Rect_bitmap[32] = {
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,

    0b00000000000011111111000000000000,
    0b00000000000011111111000000000000,
    0b00000000000011111111000000000000,
    0b00000000000011111111000000000000,
    0b00000000000011111111000000000000,
    0b00000000000011111111000000000000,
    0b00000000000011111111000000000000,
    0b00000000000011111111000000000000,
    0b00000000000011111111000000000000,
    0b00000000000011111111000000000000,
    0b00000000000011111111000000000000,

    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000,
    0b00000000000000000000000000000000
};


void oled_clear() {
    memset(display_buffer, 0, sizeof(display_buffer)); // clear buffer
    oled_show();  // send buffer to OLED
}

void draw_char(int x0, int y0, const uint32_t bitmap[32]) {
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            bool pixel_on = (bitmap[y] >> (31 - x)) & 1;
            oled_draw_pixel(x0 + x, y0 + y, pixel_on);
        }
    }
    oled_show();
}

typedef struct {
    const char name;
    float freq;
} GuitarString;

// Standard guitar tuning (low E to high E)
const GuitarString strings[6] = {
    {'E', 82.41},  // Low E
    {'A', 110.00},
    {'D', 146.83},
    {'G', 196.00},
    {'B', 246.94},
    {'e', 329.63}  // High E
};

const char closestGuitarString(float freq) {
    int closestIndex = 0;
    float minDiff = fabs(freq - strings[0].freq);

    for (int i = 1; i < 6; i++) {
        float diff = fabs(freq - strings[i].freq);
        if (diff < minDiff) {
            minDiff = diff;
            closestIndex = i;
        }
    }
    return strings[closestIndex].name;
}

#include <math.h>

int distance_from_closest_note(float frequency, char closestString) {
    float closestFreq = 0.0f;

    switch (closestString) {
        case 'E': closestFreq = strings[0].freq; break;
        case 'A': closestFreq = strings[1].freq; break;
        case 'D': closestFreq = strings[2].freq; break;
        case 'G': closestFreq = strings[3].freq; break;
        case 'B': closestFreq = strings[4].freq; break;
        case 'e': closestFreq = strings[5].freq; break; // high E
        default:
            return 0; // invalid string
    }

    float diff = frequency - closestFreq;
    float absDiff = fabsf(diff);

    if (absDiff < 2.2f) {
        return 0;               // in tune
    } else {
        return (diff > 0) ? 1 : 2;
    }  // slightly sharp / flat
}




int main() {
    
    fft_setup();

    i2c_init(i2c0, 400 * 1000);
    gpio_set_function(16, GPIO_FUNC_I2C);
    gpio_set_function(17, GPIO_FUNC_I2C);
    gpio_pull_up(16);
    gpio_pull_up(17);


    sleep_ms(3000);
    printf("FFT Setup Complete\n");

    oled_init();
    memset(display_buffer, 0, sizeof(display_buffer));

    draw_char(32, 0, A_bitmap); // draw A in top-left
    draw_char(64, 0, Z_bitmap); // draw Z in top-right

    sleep_ms(2000);

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
        char closestString = closestGuitarString(freq);
        printf("Closest Guitar String: %c\n", closestString);
        printf("-----------------------------------------------------------------------\n");
        oled_clear();
        switch (closestString) {
            case 'E':
                draw_char(48, 0, E_bitmap);
                break;
            case 'A':
                draw_char(48, 0, A_bitmap);
                break;
            case 'D':
                draw_char(48, 0, D_bitmap);
                break;
            case 'G':
                draw_char(48, 0, G_bitmap);
                break;
            case 'B':
                draw_char(48, 0, B_bitmap);
                break;
            case 'e':
                draw_char(48, 0, E_bitmap);
            default:
                oled_clear();
                break;
        }

        int distance = distance_from_closest_note(freq, closestString);
        if (distance == 2) {
            draw_char(16, 0, Rect_bitmap);
        } else if (distance == 1) {
            draw_char(80, 0, Rect_bitmap);
        }


        


        
    }
    return 0;
}
