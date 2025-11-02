BASE_ADDR = /home/chou/rv32emu
include $(BASE_ADDR)/mk/toolchain.mk

ARCH = -march=rv32izicsr
LINKER_SCRIPT = linker.ld

EMU ?= $(BASE_ADDR)/build/rv32emu

AFLAGS = -g $(ARCH)
CFLAGS = -g -march=rv32i_zicsr -Ofast
LDFLAGS = -T $(LINKER_SCRIPT)
EXEC = test.elf

CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
LD = $(CROSS_COMPILE)ld
OBJDUMP = $(CROSS_COMPILE)objdump

OBJS = start.o main.o perfcounter.o q2A_toh_v1.o q2A_toh_v2.o

.PHONY: all run dump dump2 store_dump clean

all: $(EXEC)

$(EXEC): $(OBJS) $(LINKER_SCRIPT)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.S
	$(AS) $(AFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@ -c

run: $(EXEC)
	@test -f $(EMU) || (echo "Error: $(EMU) not found" && exit 1)
	@grep -q "ENABLE_ELF_LOADER=1" $(BASE_ADDR)/build/.config || (echo "Error: ENABLE_ELF_LOADER=1 not set" && exit 1)
	@grep -q "ENABLE_SYSTEM=1" $(BASE_ADDR)/build/.config || (echo "Error: ENABLE_SYSTEM=1 not set" && exit 1)
	$(EMU) $<

dump: $(EXEC)
	$(OBJDUMP) -Ds $< | less

dump2: $(EXEC)
	$(OBJDUMP) -D $< | less
store_dump: $(EXEC)
	$(OBJDUMP) -Ds $< > dump_result
	$(OBJDUMP) -D $< > dump2_result
clean:
	rm -f $(EXEC) $(OBJS)
