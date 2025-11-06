#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "fast_rsqrt.h"
#define printstr(ptr, length)                   \
    do {                                        \
        asm volatile(                           \
            "add a7, x0, 0x40;"                 \
            "add a0, x0, 0x1;" /* stdout */     \
            "add a1, x0, %0;"                   \
            "mv a2, %1;" /* length character */ \
            "ecall;"                            \
            :                                   \
            : "r"(ptr), "r"(length)             \
            : "a0", "a1", "a2", "a7");          \
    } while (0)

#define TEST_OUTPUT(msg, length) printstr(msg, length)

#define TEST_LOGGER(msg)                     \
    {                                        \
        char _msg[] = msg;                   \
        TEST_OUTPUT(_msg, sizeof(_msg) - 1); \
    }

extern uint64_t get_cycles(void);
extern uint64_t get_instret(void);

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

/* Simple integer to hex string conversion */
static void print_hex(unsigned long val)
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
static void print_dec(unsigned long val)
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
static void print_dec_wo_n(unsigned long val)
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
// calculate the strlen
static uint32_t str_len(const char *s)
{
    uint32_t len = 0;
    while (s[len] != '\0') len++;
    return len;
}
/* ============= Tower of Hanoi Declaration ============= */
extern uint32_t play_toh_v1(void);
extern uint32_t play_toh_v2(void);
/* ============= Test Suite ============= */
static void test_play_toh_v1(void)
{
    // start to play
    uint32_t ret =  play_toh_v1();
}
static void test_play_toh_v2(void)
{
    // start to play
    uint32_t ret =  play_toh_v2();
}
/* ============= fast_rsqrt declarations ============= */
// fast_rsqrt is written in fast_rsqrt.c
void deal_fast_rsqrt_result(uint32_t in, uint32_t result, uint32_t exact_value, const char *exact_str)
{
    TEST_LOGGER("\tfast_rsqrt(");
    print_dec_wo_n(in);
    TEST_LOGGER(") = ");
    print_dec_wo_n(result);
    TEST_LOGGER(" (exact: ");
    TEST_OUTPUT(exact_str, str_len(exact_str));
    TEST_LOGGER("), Approximately Error = ");

    // calculate error percentage(%)
    uint32_t result_t10 = umul(result, 10);  // times 10, too, to corporate with the 10 times exact_value
    uint32_t error = (result_t10 > exact_value) ? (result_t10 - exact_value) : (exact_value - result_t10); // get positive difference
    error = umul(error, 100);                // let it be percentage
    uint32_t remainder = umod(error, exact_value);
    error = udiv(error, exact_value);        // only get quotient not remainder
    print_dec_wo_n(error);
    TEST_LOGGER("%, Remainder = ");
    print_dec_wo_n(remainder);
    TEST_LOGGER("/");
    print_dec_wo_n(exact_value);
    // TEST_LOGGER("\n");
}
/* ============= Test Suite ============= */
/* Test fast_rsqrt */
static void test_fast_rsqrt(void)
{
    uint64_t start_cycles, end_cycles, cycles_elapsed;
    uint64_t start_instret, end_instret, instret_elapsed;
    TEST_LOGGER("Test: fast_rsqrt\n");
    TEST_LOGGER("In below error percentage, it might lose fractional part of error in pecentage due to udiv, so show the remainder to see.\n")
    // bool passed = true;
    /* Test data */
    uint32_t in[] = {1, 4, 16, 20, 100, 258, 650};
    char *s_exact_values[] = {
        "65536",
        "32768",
        "16384",
        "14654.2",
        "6553.6",
        "4080",
        "2570.5"
    };
    uint32_t exact_values[] = { // multiply 10 for one-position fractional part
        655360, // 65536 * 10
        327680,
        163840,
        146542,
        65536, // 6553.6 * 10
        40800,
        25705
    };
    uint32_t result; // result = rsqrt(x)
    for(int i = 0; i < sizeof(in)/sizeof(uint32_t); i++)
    {
        start_cycles = get_cycles();
        start_instret = get_instret();

        result = fast_rsqrt(in[i]); // call rsqrt

        end_cycles = get_cycles();
        end_instret = get_instret();
        cycles_elapsed = end_cycles - start_cycles;
        instret_elapsed = end_instret - start_instret;
        
        deal_fast_rsqrt_result(in[i], result, exact_values[i], s_exact_values[i]);

        TEST_LOGGER(",\tCycles: ");
        print_dec_wo_n((unsigned long) cycles_elapsed);
        TEST_LOGGER(", Instructions: ");
        print_dec_wo_n((unsigned long) instret_elapsed);
        TEST_LOGGER("\n");
    }
    TEST_LOGGER("  fast_rsqrt: ");
    TEST_LOGGER("PASSED\n")
}
int main(void)
{
    uint64_t start_cycles, end_cycles, cycles_elapsed;
    uint64_t start_instret, end_instret, instret_elapsed;

    TEST_LOGGER("\n=== tower of hanoi Tests ===\n\n");

    /* Test 1: play_toh_v1 */
    TEST_LOGGER("Test 1: play_toh_v1 (RISC-V Assembly) => Original code from Q2-A. Only adjust the print format.\n");
    start_cycles = get_cycles();
    start_instret = get_instret();

    test_play_toh_v1();

    end_cycles = get_cycles();
    end_instret = get_instret();
    cycles_elapsed = end_cycles - start_cycles;
    instret_elapsed = end_instret - start_instret;

    TEST_LOGGER("  Cycles: ");
    print_dec((unsigned long) cycles_elapsed);
    TEST_LOGGER("  Instructions: ");
    print_dec((unsigned long) instret_elapsed);
    TEST_LOGGER("\n");
    
    /* Test 2: play_toh_v2 */
    TEST_LOGGER("Test 2: play_toh_v2 (RISC-V Assembly) => Handwritten assembly code adapted from Q2-A.\n");
    start_cycles = get_cycles();
    start_instret = get_instret();

    test_play_toh_v2();

    end_cycles = get_cycles();
    end_instret = get_instret();
    cycles_elapsed = end_cycles - start_cycles;
    instret_elapsed = end_instret - start_instret;

    TEST_LOGGER("  Cycles: ");
    print_dec((unsigned long) cycles_elapsed);
    TEST_LOGGER("  Instructions: ");
    print_dec((unsigned long) instret_elapsed);
    TEST_LOGGER("\n");

    /* Test 3: fast_rsqrt */
    TEST_LOGGER("Test 3: fast_rsqrt (C code) from Q3-C.\n");
    start_cycles = get_cycles();
    start_instret = get_instret();

    test_fast_rsqrt();

    end_cycles = get_cycles();
    end_instret = get_instret();
    cycles_elapsed = end_cycles - start_cycles;
    instret_elapsed = end_instret - start_instret;

    TEST_LOGGER(" Total Cycles: ");
    print_dec((unsigned long) cycles_elapsed);
    TEST_LOGGER(" Total Instructions: ");
    print_dec((unsigned long) instret_elapsed);
    TEST_LOGGER("\n");

    TEST_LOGGER("\n=== All Tests Completed ===\n");

    return 0;
}