/*
 * File: schroeder.cpp
 *
 * Schroeder's reverbration
 *
 */

#include "userrevfx.h"

#define MAX_DEPTH 0.99f

static float s_depth;
static float s_time;

typedef struct delay_buffer_t {
    int length;
    int pos;
    float gain;
    float *buf;
} delay_buffer_t;

inline void delay_setmem(delay_buffer_t *delay, int length, float* mem) {
    delay->buf = mem;
    delay->length = length;
    delay->pos = 0;
    delay->gain = 0.7;
}

inline void delay_add(delay_buffer_t *delay, float data) {
    delay->buf[delay->pos++] = data;
    if (delay->pos == delay->length) {
        delay->pos = 0;
    }
}

inline float delay_read_write(delay_buffer_t *delay, float data){
    float buf_in;
    float buf_out;

    buf_out = delay->buf[delay->pos];
    delay_add(delay, data + delay->gain * buf_out);
    return buf_out;
}

#define DELAY1_LEN 1913
#define DELAY1_GAIN 0.871402
static delay_buffer_t delay1;
static __sdram float delay1_mem[DELAY1_LEN] = {0.f};

#define DELAY2_LEN 1733
#define DELAY2_GAIN 0.882762
static delay_buffer_t delay2;
static __sdram float delay2_mem[DELAY2_LEN] = {0.f};

#define DELAY3_LEN 1597
#define DELAY3_GAIN 0.891443
static delay_buffer_t delay3;
static __sdram float delay3_mem[DELAY3_LEN] = {0.f};

#define DELAY4_LEN 1447
#define DELAY4_GAIN 0.901117
static delay_buffer_t delay4;
static __sdram float delay4_mem[DELAY4_LEN] = {0.f};


inline float allpass_read_write(delay_buffer_t *allpass, float data){
    float buf_in;
    float buf_out;

    buf_out = allpass->buf[allpass->pos];
    buf_in = data + allpass->gain * buf_out;
    delay_add(allpass, buf_in);
    return (buf_out - allpass->gain * buf_in);
}

#define ALLPASS1_LEN 241
#define ALLPASS1_GAIN 0.7
static delay_buffer_t allpass1;
static __sdram float allpass1_mem[ALLPASS1_LEN] = {0.f};

#define ALLPASS2_LEN 83
#define ALLPASS2_GAIN 0.7
static delay_buffer_t allpass2;
static __sdram float allpass2_mem[ALLPASS2_LEN] = {0.f};

void REVFX_INIT(uint32_t platform, uint32_t api)
{
    s_depth = 0.f;
    delay_setmem(&delay1, DELAY1_LEN, delay1_mem);
    delay_setmem(&delay2, DELAY2_LEN, delay2_mem);
    delay_setmem(&delay3, DELAY3_LEN, delay3_mem);
    delay_setmem(&delay4, DELAY4_LEN, delay4_mem);
    delay_setmem(&allpass1, ALLPASS1_LEN, allpass1_mem);
    delay_setmem(&allpass2, ALLPASS2_LEN, allpass2_mem);
}

void REVFX_PROCESS(float *main_xn, 
                   uint32_t frames)
{
    float * __restrict my = main_xn;
    const float * my_e = my + 2 * frames;

    for (; my != my_e; ) {
        float dry = *my;
        float r;
        float reverb = 0;
        float early = 0;

        reverb += 0.25 * delay_read_write(&delay1, dry);
        reverb += 0.25 * delay_read_write(&delay2, dry);
        reverb += 0.25 * delay_read_write(&delay3, dry);
        reverb += 0.25 * delay_read_write(&delay4, dry);

        reverb = allpass_read_write(&allpass1, reverb);
        reverb = allpass_read_write(&allpass2, reverb);

        *(my++) = dry + s_depth * reverb;
        *(my++) = dry + s_depth * reverb;
    }
}


void REVFX_PARAM(uint8_t index, int32_t value)
{
    const float valf = q31_to_f32(value);
    switch (index) {
    case k_user_revfx_param_time:
        s_time = valf;
        delay1.gain = DELAY1_GAIN * s_time;
        delay2.gain = DELAY2_GAIN * s_time;
        delay3.gain = DELAY3_GAIN * s_time;
        delay4.gain = DELAY4_GAIN * s_time;
        allpass1.gain = ALLPASS1_GAIN * s_time;
        allpass2.gain = ALLPASS2_GAIN * s_time;
        break;
    case k_user_revfx_param_depth:
        s_depth = valf * MAX_DEPTH;
        break;
    case k_user_revfx_param_shift_depth:
        break;
    default:
        break;
  }
}
