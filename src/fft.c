#include <stdint.h>
#include <string.h>
#include "arm_math.h"

#define FFT_SIZE                2048
#define FFT_HALF                (FFT_SIZE / 2)
#define PEAK_MIN_BIN            20    /* ~49 Hz @ 5000 Hz rate — adjust to your min RPM */
#define PEAK_THRESHOLD_SHIFT    2     /* 25% of max                                     */
#define NO_ROTATION_RATIO       2     /* peak must be 4× the noise floor average        */
#define NO_ROTATION_MIN_MAG     10   /* absolute q15 magnitude floor                   */

static arm_rfft_instance_q15 fft_instance;
static q15_t     fft_in [FFT_SIZE]     __attribute__((aligned(4)));
static q15_t     fft_out[FFT_SIZE * 2] __attribute__((aligned(4)));
static q15_t     mag    [FFT_HALF]     __attribute__((aligned(4)));

void fft_init(void)
{
    arm_rfft_init_q15(&fft_instance, FFT_SIZE, 0, 1);
}

uint16_t get_rotation_frequency_q(const uint8_t *data, uint16_t sample_rate)
{
    uint16_t i;

    if (sample_rate == 0) return 0;

    /* ── Step 1: DC removal + convert uint8 → q15 ──────────────── */
    uint32_t sum = 0;
    for (i = 0; i < FFT_SIZE; i++) sum += data[i];
    int16_t dc = (int16_t)(sum / FFT_SIZE);

    for (i = 0; i < FFT_SIZE; i++)
        fft_in[i] = (q15_t)(((int16_t)data[i] - dc) << 7);

    /* ── Step 2: Forward 2048-point real FFT ────────────────────── */
    arm_rfft_q15(&fft_instance, fft_in, fft_out);

    /* ── Step 3: Magnitude spectrum ─────────────────────────────── */
    arm_cmplx_mag_q15(fft_out, mag, FFT_HALF);

    /* ── Step 4: No-rotation detection ──────────────────────────── */
    /* Find peak and average magnitude above minimum bin */
    q15_t    max_mag = 0;
    uint32_t mag_sum = 0;
    for (i = PEAK_MIN_BIN; i < FFT_HALF; i++)
    {
        mag_sum += mag[i];
        if (mag[i] > max_mag) max_mag = mag[i];
    }
    uint16_t avg_mag = (uint16_t)(mag_sum / (FFT_HALF - PEAK_MIN_BIN));

    /* Return 0 (no rotation) if signal is too weak or has no clear peak */
    if (max_mag < NO_ROTATION_MIN_MAG)          return 0;
    if (max_mag < (q15_t)(avg_mag * NO_ROTATION_RATIO)) return 0;

    /* ── Step 5: Adaptive threshold + peak search ───────────────── */
    q15_t threshold = max_mag >> PEAK_THRESHOLD_SHIFT;

    for (i = PEAK_MIN_BIN + 1; i < FFT_HALF - 1; i++)
    {
        if ((mag[i] > threshold)    &&
            (mag[i] > mag[i - 1])   &&
            (mag[i] > mag[i + 1]))
        {
            return (uint16_t)((uint32_t)i * sample_rate / FFT_SIZE);
        }
    }

    return 0;
}
