#ifndef FAST_RSQRT
#define FAST_RSQRT
static void mul32(uint32_t a, uint32_t b, uint32_t *hi, uint32_t *lo);
static uint32_t mul32_shift(uint32_t a, uint32_t b, uint8_t shift);
static int clz(uint32_t x);
uint32_t fast_rsqrt(uint32_t x);
#endif