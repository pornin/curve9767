CC = arm-none-eabi-gcc
CFLAGS = -Wall -Wextra -Wshadow -ggdb3 -Os $(ARCHFLAGS) -D__SAMD20E18__ -DDONT_USE_CMSIS_INIT -DCF_SIDE_CHANNEL_PROTECTION=0 -DCORTEX_M0
LD = arm-none-eabi-gcc
LDFLAGS = -Wl,--gc-sections $(ARCHFLAGS)
LDLIBS = -Wl,--start-group -lgcc -lnosys -Wl,--end-group
ARCHFLAGS = -mthumb -mlong-calls -mcpu=cortex-m0plus
LINKER_SCRIPT = samd20e18.ld

OBJS = core.o curve9767.o ecdh.o hash.o keygen.o ops_arm.o ops_cm0.o scalar_cm0.o scalar_arm.o sha3.o sign.o timing.o

all: benchmark.elf

clean:
	-rm -f benchmark.elf $(OBJS)

benchmark.elf: $(OBJS)
	$(LD) $(LDFLAGS) -T$(LINKER_SCRIPT) -o benchmark.elf $(OBJS) $(LDLIBS)

core.o: core.c
	$(CC) $(CFLAGS) -c -o core.o core.c

curve9767.o: curve9767.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o curve9767.o curve9767.c

ecdh.o: ecdh.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o ecdh.o ecdh.c

hash.o: hash.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o hash.o hash.c

keygen.o: keygen.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o keygen.o keygen.c

ops_arm.o: ops_arm.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o ops_arm.o ops_arm.c

ops_cm0.o: ops_cm0.s
	$(CC) $(CFLAGS) -c -o ops_cm0.o ops_cm0.s

scalar_arm.o: scalar_arm.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o scalar_arm.o scalar_arm.c

scalar_cm0.o: scalar_cm0.s
	$(CC) $(CFLAGS) -c -o scalar_cm0.o scalar_cm0.s

sha3.o: sha3.c sha3.h
	$(CC) $(CFLAGS) -c -o sha3.o sha3.c

sign.o: sign.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o sign.o sign.c

timing.o: timing.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o timing.o timing.c
