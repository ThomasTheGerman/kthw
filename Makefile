CC=arm-none-eabi-gcc
AS=arm-none-eabi-as
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy
OBJDUMP=arm-none-eabi-objdump

CFLAGS=-I./ -std=c11 -mcpu=cortex-m4 -mthumb -fno-common -fno-builtin -g -Wextra -Wall -Wno-missing-field-initializers -Wno-unused-parameter
ASFLAGS=-mcpu=cortex-m4 -mthumb
LDFLAGS=-nostartfiles -T stm32f407vg.ld

all: main.elf

clean:
	rm -f *.bin *.elf *.o

main.bin: main.elf
	$(OBJCOPY) -O binary $< $@

main.elf: startup.o main.o util.o bomb.o simonsays.o morse.o wires.o capacitor.o memory.o password.o
	$(LD) $(LDFLAGS) -o $@ $^

main.o: pins.inc.h

startup.o: stm32f407vg.h
