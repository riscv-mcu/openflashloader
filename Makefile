CROSS_COMPILE ?= riscv-nuclei-elf-

RISCV_CC=$(CROSS_COMPILE)gcc
RISCV_OBJCOPY=$(CROSS_COMPILE)objcopy
RISCV_OBJDUMP=$(CROSS_COMPILE)objdump

CFLAGS = -nostdlib -nostartfiles -Wall -Os -fPIC -Wunused-result -g
RISCV32_CFLAGS = -march=rv32e -mabi=ilp32e $(CFLAGS)
RISCV64_CFLAGS = -march=rv64i -mabi=lp64 $(CFLAGS)

all: riscv32_nuspi.bin riscv64_nuspi.bin

.PHONY: clean

# .c -> .o
riscv32_%.o: %.c
	$(RISCV_CC) -c $(RISCV32_CFLAGS) $^ -o $@

riscv64_%.o: %.c
	$(RISCV_CC) -c $(RISCV64_CFLAGS) $< -o $@

# .S -> .o
riscv32_%.o: %.S
	$(RISCV_CC) -c $(RISCV32_CFLAGS) $^ -o $@

riscv64_%.o: %.S
	$(RISCV_CC) -c $(RISCV64_CFLAGS) $^ -o $@

# .o -> .elf
riscv32_%.elf: riscv32_%.o riscv32_startup.o
	$(RISCV_CC) -T riscv.lds $(RISCV32_CFLAGS) $^ -o $@

riscv64_%.elf: riscv64_%.o riscv64_startup.o
	$(RISCV_CC) -T riscv.lds $(RISCV64_CFLAGS) $^ -o $@

# .elf -> .bin
%.bin: %.elf
	$(RISCV_OBJCOPY) -Obinary $< $@

# utility
%.lst: %.elf
	$(RISCV_OBJDUMP) -S $< > $@

clean:
	-rm -f *.elf *.o *.lst *.bin
