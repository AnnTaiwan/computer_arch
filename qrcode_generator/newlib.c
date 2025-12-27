#include "newlib.h"

/* Bare metal memcpy implementation */
void *memcpy(void *dest, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *) dest;
    const uint8_t *s = (const uint8_t *) src;
    while (n--)
        *d++ = *s++;
    return dest;
}

/* Software division for RV32I (no M extension) */
static unsigned long udiv(unsigned long dividend, unsigned long divisor)
{
    if (divisor == 0)
        return 0;

    unsigned long quotient = 0;
    unsigned long remainder = 0;

    for (int i = 31; i >= 0; i--) {
        remainder <<= 1;
        remainder |= (dividend >> i) & 1;

        if (remainder >= divisor) {
            remainder -= divisor;
            quotient |= (1UL << i);
        }
    }

    return quotient;
}

static unsigned long umod(unsigned long dividend, unsigned long divisor)
{
    if (divisor == 0)
        return 0;

    unsigned long remainder = 0;

    for (int i = 31; i >= 0; i--) {
        remainder <<= 1;
        remainder |= (dividend >> i) & 1;

        if (remainder >= divisor) {
            remainder -= divisor;
        }
    }

    return remainder;
}

/* Software multiplication for RV32I (no M extension) */
static uint32_t umul(uint32_t a, uint32_t b)
{
    uint32_t result = 0;
    while (b) {
        if (b & 1)
            result += a;
        a <<= 1;
        b >>= 1;
    }
    return result;
}

/* Provide __mulsi3 for GCC */
uint32_t __mulsi3(uint32_t a, uint32_t b)
{
    return umul(a, b);
}

/* Bare metal strlen implementation */
uint32_t str_len(const char *s)
{
    const char *p = s;
    while (*p)
        p++;
    return p - s;
}

/* Simple integer to hex string conversion */
void print_hex(unsigned long val)
{
    char buf[20];
    char *p = buf + sizeof(buf) - 1;
    *p = '\n';
    p--;

    if (val == 0) {
        *p = '0';
        p--;
    } else {
        while (val > 0) {
            int digit = val & 0xf;
            *p = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
            p--;
            val >>= 4;
        }
    }

    p++;
    printstr(p, (buf + sizeof(buf) - p));
}

/* Simple integer to decimal string conversion */
void print_dec(unsigned long val)
{
    char buf[20];
    char *p = buf + sizeof(buf) - 1;
    *p = '\n';
    p--;

    if (val == 0) {
        *p = '0';
        p--;
    } else {
        while (val > 0) {
            *p = '0' + umod(val, 10);
            p--;
            val = udiv(val, 10);
        }
    }

    p++;
    printstr(p, (buf + sizeof(buf) - p));
}
// don't print \n
void print_dec_wo_n(unsigned long val)
{
    char buf[20];
    char *p = buf + sizeof(buf) - 1;

    if (val == 0) {
        *p = '0';
        p--;
    } else {
        while (val > 0) {
            *p = '0' + umod(val, 10);
            p--;
            val = udiv(val, 10);
        }
    }

    p++;
    printstr(p, (buf + sizeof(buf) - p));
}

/* Bare metal puts implementation */
int puts(const char *s)
{
    size_t len = str_len(s);
    printstr((char *)s, len);
    char newline = '\n';
    printstr(&newline, 1);
    return len + 1;
}

/* Simple sprintf for integer formatting (supports %d only) */
int sprintf(char *str, const char *format, ...)
{
    char *dst = str;
    const char *fmt = format;
    
    /* Use va_list for variadic arguments */
    typedef __builtin_va_list va_list;
    #define va_start(ap, last) __builtin_va_start(ap, last)
    #define va_arg(ap, type) __builtin_va_arg(ap, type)
    #define va_end(ap) __builtin_va_end(ap)
    
    va_list args;
    va_start(args, format);
    
    while (*fmt) {
        if (*fmt == '%' && *(fmt + 1) == 'd') {
            /* Handle %d format specifier */
            int val = va_arg(args, int);
            char buf[12];
            char *p = buf + sizeof(buf) - 1;
            *p = '\0';
            p--;
            
            int is_negative = 0;
            if (val < 0) {
                is_negative = 1;
                val = -val;
            }
            
            if (val == 0) {
                *p = '0';
                p--;
            } else {
                while (val > 0) {
                    *p = '0' + umod(val, 10);
                    p--;
                    val = udiv(val, 10);
                }
            }
            
            if (is_negative) {
                *p = '-';
                p--;
            }
            
            p++;
            /* Copy to destination */
            while (*p) {
                *dst++ = *p++;
            }
            
            fmt += 2;
        } else {
            *dst++ = *fmt++;
        }
    }
    
    *dst = '\0';
    va_end(args);
    return dst - str;
}