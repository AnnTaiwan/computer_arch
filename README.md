# computer_arch
## caHW2
Write programs to run in a bare-metal environment by using rv32emu's system emulation.
* [Assignment2: Complete Applications](https://hackmd.io/@sysprog/2025-arch-homework2)
* Follow instructions to build rv32emu : [Lab2: RISC-V Instruction Set Simulator and System Emulator](https://hackmd.io/@sysprog/Sko2Ja5pel)

### Directory
* **Q2**: run `uf8_decode` and `uf8_encode` assembly code.
* **Q3**: run **tower of hanoi** assembly code and `fast_sqrt` c code.
* **Q3_Ofast**: With `-Ofast` optimization flag using `riscv-none-elf-gcc`, run **tower of hanoi** assembly code and `fast_sqrt` c code.
* **Q3_Osize**: With `-Os` optimization flag using `riscv-none-elf-gcc`, run **tower of hanoi** assembly code and `fast_sqrt` c code.

### How to Use
* Run
```
cd caHW2/Q3
make clean all run
```
* Disassemble `elf`
```
make dump
make dump2
make store_dump # store dump result into files
```