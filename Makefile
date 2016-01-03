CFLAGS = -Wall -g

all: disassemble.c emulator.c error.c hardware.c instructions.c register.c shift_register.c utility.c
	gcc $(CFLAGS) -o emulator disassemble.c emulator.c error.c hardware.c instructions.c register.c shift_register.c \
		utility.c -lSDL2

clean:
	rm emulator
