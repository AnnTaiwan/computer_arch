#include <stdint.h>
#include "fast_rsqrt.h"
static const uint32_t rsqrt_table[32] = {
    65536, 46341, 32768, 23170, 16384,  /* 2^0 to 2^4 */
    11585,  8192,  5793,  4096,  2896,  /* 2^5 to 2^9 */
     2048,  1448,  1024,   724,   512,  /* 2^10 to 2^14 */
      362,   256,   181,   128,    90,  /* 2^15 to 2^19 */
       64,    45,    32,    23,    16,  /* 2^20 to 2^24 */
       11,     8,     6,     4,     3,  /* 2^25 to 2^29 */
        2,     1                         /* 2^30, 2^31 */
};
/* Adjust the code to prevent from using 64bits shift because it will be optimized to use 64-bit library like __lshrdi3, which lead to compiling error.*/
/* 32x32 -> 64 bit multiplication implemented by hand */
static void mul32(uint32_t a, uint32_t b, uint32_t *hi, uint32_t *lo)
{
    uint32_t a_hi = a >> 16;
    uint32_t a_lo = a & 0xFFFF;
    uint32_t b_hi = b >> 16;
    uint32_t b_lo = b & 0xFFFF;

    uint32_t p0 = a_lo * b_lo;
    uint32_t p1 = a_lo * b_hi;
    uint32_t p2 = a_hi * b_lo;
    uint32_t p3 = a_hi * b_hi;

    uint32_t mid = (p1 & 0xFFFF) + (p2 & 0xFFFF) + (p0 >> 16);
    *lo = (p0 & 0xFFFF) | ((mid & 0xFFFF) << 16);
    *hi = p3 + (p1 >> 16) + (p2 >> 16) + (mid >> 16);
}

/* Extract (a*b)>>shift, safe for shift<=32 */
static uint32_t mul32_shift(uint32_t a, uint32_t b, uint8_t shift)
{
    uint32_t hi, lo;
    mul32(a, b, &hi, &lo);
    if (shift == 0)
        return lo;
    else if (shift < 32)
        return (lo >> shift) | (hi << (32 - shift));
    else
        return hi >> (shift - 32);
}

static int clz(uint32_t x)
{
    if (!x) return 32;
    int n = 0;
    if (!(x & 0xFFFF0000)) { n += 16; x <<= 16; }
    if (!(x & 0xFF000000)) { n += 8; x <<= 8; }
    if (!(x & 0xF0000000)) { n += 4; x <<= 4; }
    if (!(x & 0xC0000000)) { n += 2; x <<= 2; }
    if (!(x & 0x80000000)) { n += 1; }
    return n;
}

uint32_t fast_rsqrt(uint32_t x)
{
    if (x == 0) return 0xFFFFFFFF; // represents inf
    if (x == 1) return 65536;

    int exp = 31 - clz(x);
    uint32_t y = rsqrt_table[exp];

    /* linear interpolation */
    if (x > (1u << exp))
    {
        uint32_t y_next = (exp < 31) ? rsqrt_table[exp + 1] : 0;
        uint32_t delta = y - y_next;

        // ((x - (1<<exp)) << 16) >> exp
        uint32_t frac = ((x - (1u << exp)) << 16) >> exp;

        y -= (delta * frac) >> 16;
    }

    /* Newton-Raphson Iteration */
    for (int i = 0; i < 2; i++)
    {
        uint32_t y2 = mul32_shift(y, y, 0);
        uint32_t xy2 = mul32_shift(x, y2, 16);
        uint32_t term = (3u << 16) - xy2;
        y = mul32_shift(y, term, 17); // divide by 2 and scale back
    }

    return y;
}
