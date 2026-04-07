#ifndef FFT_H
#define FFT_H

#include <stdint.h>

void fft_init(void);
uint16_t get_rotation_frequency_q(const uint8_t *data, uint16_t sample_rate);

#endif
