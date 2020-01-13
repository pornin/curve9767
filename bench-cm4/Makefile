CC = arm-none-eabi-gcc
CFLAGS = -Wall -Wextra -std=c99 -ggdb3 -Os $(ARCHFLAGS) -fno-common -ffunction-sections -fdata-sections -I $(INCDIR) -DSTM32F4
LD = arm-none-eabi-gcc
LDFLAGS = -L$(LIBDIR) --static -nostartfiles -T$(LDSCRIPT) -ggdb3 $(ARCHFLAGS) -Wl,--gc-sections
LDLIBS = -lopencm3_stm32f4 -Wl,--start-group -lc -lgcc -lm -lnosys -Wl,--end-group
ARCHFLAGS = -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16

INCDIR = ../libopencm3/include
LIBDIR = ../libopencm3/lib
LDSCRIPT = stm32f4-discovery.ld

OBJ = curve9767.o ecdh.o hash.o keygen.o ops_arm.o scalar_ref.o ops_cm4.o sha3_cm4.o sign.o timing.o

all: timing.elf

timing.elf: $(OBJ)
	$(LD) $(LDFLAGS) -o timing.elf $(OBJ) $(LDLIBS)

clean:
	-rm -f timing.elf $(OBJ)

timing.o: timing.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o timing.o timing.c

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

ops_cm4.o: ops_cm4.s
	$(CC) $(CFLAGS) -c -o ops_cm4.o ops_cm4.s

scalar_ref.o: scalar_ref.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o scalar_ref.o scalar_ref.c

sha3_cm4.o: sha3_cm4.c sha3.h
	$(CC) $(CFLAGS) -c -o sha3_cm4.o sha3_cm4.c

sign.o: sign.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o sign.o sign.c
