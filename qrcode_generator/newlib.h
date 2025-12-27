#ifndef NEWLIB_H
#define NEWLIB_H

/*
 * Bare-Metal C Library Replacement (newlib)
 *
 * This header provides minimal implementations of standard C library functions
 * for bare-metal RISC-V environments without linking against libc.
 *
 * WHY THIS EXISTS:
 * - Bare-metal programs run directly on hardware without an OS
 * - Standard C library (libc) requires OS support (syscalls, memory management)
 * - Linking against libc in bare-metal causes undefined references
 * - We provide lightweight replacements for commonly-used functions
 *
 * WHAT'S PROVIDED:
 * - String functions: str_len
 * - Memory functions: memcpy
 * - I/O functions: puts, sprintf (limited %d support), printstr macro
 * - Utility functions: print_dec, print_hex for debugging
 * - Math helpers: Software multiply for RV32I (no M extension)
 *
 * USAGE:
 * - Include this header instead of <string.h>, <stdio.h>, <stdlib.h>
 * - Link newlib.o with your other object files
 * - No standard C library linking required
 */

#include <stdbool.h>
#include <stdint.h>

/* ============= Macros ============= */

/* Print string macro using RISC-V ecall */
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

/* ============= Type Definitions ============= */

#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef unsigned long size_t;
#endif

/* ============= Standard C Library Functions ============= */

/* String functions */
uint32_t str_len(const char *s);

/* Memory functions */
void *memcpy(void *dest, const void *src, size_t n);

/* I/O functions */
int puts(const char *s);
int sprintf(char *str, const char *format, ...);  /* Supports %d only */

/* ============= Utility Functions ============= */

/* Print functions for debugging */
void print_hex(unsigned long val);       /* Prints value in hexadecimal */
void print_dec(unsigned long val);       /* Prints value in decimal with newline */
void print_dec_wo_n(unsigned long val);  /* Prints value in decimal without newline */

/* Math functions for RV32I (no M extension) */
uint32_t __mulsi3(uint32_t a, uint32_t b);

#endif /* NEWLIB_H */