# QR Code Generator - RISC-V Bare-Metal Implementation

High-performance QR code generator for RISC-V (RV32I) bare-metal environments, optimized for QR code versions 1, 2, and 3 with multiple optimization levels.

## Project Overview

This project implements a complete QR code encoder in bare-metal RISC-V assembly and C, targeting RV32I without requiring the M extension (hardware multiplication). Features progressive optimization techniques from reference C code to hand-optimized inline assembly.

## Project Structure

```
qrcode_generator/
├── qrcode_v01.c         - Reference implementation (CODE_OPT_VER=0, 1)
├── qrcode_opt_v2_1.c    - Initial RISC-V assembly (CODE_OPT_VER=2')
├── qrcode_opt_v2_2.c    - Optimized RISC-V assembly (CODE_OPT_VER=2)
├── qrcode_opt_v3.c      - Full optimizations (CODE_OPT_VER=3a, 3b)
├── main.c               - Test harness with performance counters
├── newlib.c/h           - Bare-metal C library (strlen, sprintf, puts, etc.)
├── perfcounter.S        - RISC-V CSR performance counter access
├── start.S              - Bare-metal startup code
├── linker.ld            - Memory layout for bare-metal execution
└── Makefile             - Build system
```

## Quick Start

### Prerequisites

- RISC-V GNU Toolchain (riscv32-unknown-elf-gcc)
- rv32emu with ELF loader and system support enabled

### Build & Run

```bash
# Clean and build
make clean all

# Run on rv32emu
make run

# View disassembly
make dump          # Interactive (less)
make store_dump    # Save to files
```

### Select Optimization Level

Edit `main.c` and change `CODE_OPT_VER`:

```c
#define CODE_OPT_VER 3  // 0, 1, 2, or 3
```

Then rebuild:
```bash
make clean all run
```

**Recommendations:**
- **For best performance**: Use `CODE_OPT_VER=3a` (196,804 cycles, +512 bytes)
- **For no memory overhead**: Use `CODE_OPT_VER=3b` (240,483 cycles, 0 bytes)
- **For portability**: Use `CODE_OPT_VER=1` (370,333 cycles, pure C)
- **Avoid**: `CODE_OPT_VER=2'` (superseded by version 2)

## Optimization Levels

**Naming Convention:** `CODE_OPT_VER: {Iter/LUT}_{C/RISC-V}`

| Version | Name | Key Features | Cycles | Relative |
|---------|------|--------------|--------|----------|
| **0** | LUT_C | Log/exp LUT-based GF multiplication (C) | 201,172 | 1.00x |
| **1** | Iter_C | Iterative GF multiplication (C) | 370,333 | 1.84x |
| **2'** | Iter_RISC-V | Initial RISC-V assembly version | 347,533 | 1.73x |
| **2** | Iter_RISC-V | Optimized: removes unnecessary multiplication | 244,851 | 1.22x |
| **3a** | LUT_C + Opt | LUT + bit-shift & algorithm optimizations | 196,804 | 0.98x |
| **3b** | Iter_RISC-V + Opt | Version 2 + bit-shift & algorithm optimizations | 240,483 | 1.20x |

### Performance Comparison (Complete QR Code Generation)

Based on actual measurements (cycles = instructions for this implementation):

| CODE_OPT_VER | Implementation | Cycles | Speedup vs Iter_C | Memory Overhead |
|--------------|----------------|--------|-------------------|------------------|
| 0 | LUT_C | 201,172 | 1.84x | +512 bytes (LUT) |
| 1 | Iter_C | 370,333 | 1.00x (baseline) | 0 bytes |
| 2' | Iter_RISC-V (initial) | 347,533 | 1.07x | 0 bytes |
| 2 | Iter_RISC-V (optimized) | 244,851 | 1.51x | 0 bytes |
| 3a | LUT_C + Optimizations | 196,804 | 1.88x | +512 bytes (LUT) |
| 3b | Iter_RISC-V + Optimizations | 240,483 | 1.54x | 0 bytes |

**Key Insights:**
- **Version 3a (LUT_C + Opt)**: Fastest overall - 1.88x faster than baseline C, but requires 512-byte LUT
- **Version 3b (Iter_RISC-V + Opt)**: Best memory-free option - 1.54x faster than baseline C
- **Version 2 vs 1**: RISC-V assembly alone gives 1.51x speedup over C (no memory cost)
- **Version 3 improvements**: Additional optimizations provide ~2.2% speedup (4,368 cycles saved)

### Optimization Techniques

#### Version 0: LUT_C (Baseline Reference)
- Uses pre-computed log/antilog tables for GF(2^8) multiplication
- Fast but requires 512 bytes for lookup tables
- Converts `x * y` to `2^(log(x) + log(y))` via table lookups

#### Version 1: Iter_C (Pure C Baseline)
- Iterative Galois Field multiplication in pure C
- Software-based multiplication (no hardware MUL)
- Most portable but slowest

#### Version 2': Iter_RISC-V (Initial Assembly)
- Direct translation of C iterative algorithm to RISC-V assembly
- Manual register allocation
- Contains unnecessary multiplication operations

#### Version 2: Iter_RISC-V (Optimized)
- **Key optimization**: Removes unnecessary multiplication in `_rs_mul`
- Result: 1.51x speedup over C version

#### Version 3a: LUT_C + Full Optimizations
- Combines LUT-based multiplication with algorithm improvements
- Bit-shift optimizations (replace `* 2` with `<< 1`, `/ 2` with `>> 1`)
- Reed-Solomon algorithm improvements
- Stack-allocated parameter arrays (avoid .rodata issues)
- **Improvement**: 4,368 cycles saved compared to Version 0 (~2.2%)

#### Version 3b: Iter_RISC-V + Full Optimizations
- Version 2 optimizations + additional improvements from 3a
- Same bit-shift and algorithm optimizations as 3a
- No lookup tables required
- **Improvement**: 4,368 cycles saved compared to Version 2 (~1.8%)

## Technical Details

### Target Architecture
- **ISA**: RV32I + Zicsr (CSR instructions for performance counters)
- **No M Extension**: All multiplication implemented in software
- **Bare-metal**: No OS, no libc, direct hardware access

### QR Code Specifications
| Version | Size | Capacity (Byte Mode) | ECC Degree |
|---------|------|----------------------|------------|
| 1 | 21×21 | 17 bytes | 7 |
| 2 | 25×25 | 32 bytes | 10 |
| 3 | 29×29 | 53 bytes | 15 |

### Reed-Solomon GF(2^8) Multiplication

The core operation `_rs_mul(x, y)` performs Galois Field multiplication:

```c
// Algorithm (QR_OPT=1, iterative C):
z = 0
for i = 7 down to 0:
    z = (z << 1) ^ ((z >> 7) * 0x11D)  // GF reduction
    z ^= ((y >> i) & 1) * x             // Conditional add
return z
```

**Progressive Optimization Strategy:**
1. **CODE_OPT_VER=0**: Log/antilog LUT method (x×y = 2^(log(x)+log(y)))
2. **CODE_OPT_VER=1**: Iterative C baseline (software multiply)
3. **CODE_OPT_VER=2'**: Initial RISC-V assembly translation
4. **CODE_OPT_VER=2**: Remove unnecessary operations + branchless conditionals
5. **CODE_OPT_VER=3a/3b**: Add bit-shift & algorithm optimizations

## Performance Monitoring

The test harness measures execution using RISC-V CSRs:
- `mcycle` - Cycle count
- `minstret` - Instruction count

Example output:
```
=== QR Code Tests ===

Test 3: QR code (Fully optimized version)
[QR code ASCII art output]
Exit Successfully.
  Cycles: 240483
  Instructions: 240483

=== All Tests Completed ===
```

**Note**: In this implementation, cycle count equals instruction count (CPI = 1.0).

## Build System

### Makefile Targets

```bash
make all          # Build test.elf
make run          # Execute on rv32emu
make dump         # View disassembly (interactive)
make dump2        # Full disassembly (interactive)
make store_dump   # Save disassembly to files
make clean        # Remove build artifacts
```

### Compiler Flags
- `-march=rv32i_zicsr` - RV32I with CSR support
- `-g` - Debug symbols for disassembly
- Custom linker script for bare-metal layout

## Usage Example

```c
#include "qrcode.h"

int main(void) {
    const char *url = "https://github.com/sysprog21/rv32emu";
    
    // Version 0: Reference implementation
    generate_qrcode();
    
    // Version 3: Fully optimized
    generate_qrcode_opt_v3();
    
    return 0;
}
```

## Verification

Verify generated QR codes at:
- https://www.nayuki.io/page/creating-a-qr-code-step-by-step

## Key Implementation Notes

1. **Bare-metal Environment**: No malloc, no libc - all functions implemented in newlib.c
2. **Fixed Mask Pattern**: Uses mask pattern 0 for simplicity
3. **Error Correction Level**: Fixed at L (Low, ~7% recovery)
4. **ASCII Output**: QR codes printed as Unicode block characters (██)

## References

- [QR Code Tutorial](https://www.thonky.com/qr-code-tutorial/)
- [Reed-Solomon Error Correction](https://en.wikiversity.org/wiki/Reed%E2%80%93Solomon_codes_for_coders)
- Original QR123 by Ling LI (lix2ng@gmail.com)

## License

MIT License - See file headers for details