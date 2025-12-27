# QR Code Generator - RISC-V Bare-Metal Implementation

QR code generator for RISC-V (RV32I) bare-metal environment, optimized for version 1, 2, 3 QR codes.

## Files

- **qrcode.c** - Reference C implementation (QR_OPT=0: LUT-based, QR_OPT=1: iterative)
- **qrcode_opt.c** - Assembly-optimized version (QR_OPT=2: inline RISC-V assembly)
- **qrcode_opt_v2.c** - Alternative optimized version
- **main.c** - Test harness with performance counters
- **newlib.c/h** - Bare-metal C library replacement (strlen, sprintf, puts, etc.)

## Build & Run

```bash
make clean all    # Build test.elf
make run          # Run on rv32emu
```

## Optimization Levels

| Level | Implementation | Method |
|-------|---------------|--------|
| QR_OPT=0 | Log/Exp LUT | Fastest, uses lookup tables |
| QR_OPT=1 | Iterative C | Portable, software multiply |
| QR_OPT=2 | RISC-V Assembly | Hand-optimized for RV32I (no M extension) |

## Key Features

- **Bare-metal**: No OS, no standard C library
- **RV32I only**: Software multiplication (no M extension)
- **Inline assembly**: Optimized Reed-Solomon GF(2^8) multiplication
- **Performance counters**: Measures cycles and instructions

## Technical Details

- **Target**: RISC-V RV32I + Zicsr
- **Capacity**: V1=17B, V2=32B, V3=53B (byte mode)
- **Execution**: rv32emu with ELF loader and system support enabled