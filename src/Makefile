CC = clang
CFLAGS = -Wall -Wextra -Wshadow -Wundef -O3
LD = clang
LDFLAGS =
LIBS =

OBJ = curve9767.o ecdh.o hash.o keygen.o ops_ref.o scalar_ref.o sha3.o sign.o
OBJTEST = test_curve9767.o
OBJSPEED = speed_ref.o

all: test_curve9767 speed_curve9767

test_curve9767: $(OBJ) $(OBJTEST)
	$(LD) $(LDFLAGS) -o test_curve9767 $(OBJ) $(OBJTEST) $(LIBS)

speed_curve9767: $(OBJ) $(OBJSPEED)
	$(LD) $(LDFLAGS) -o speed_curve9767 $(OBJ) $(OBJSPEED) $(LIBS)

clean:
	-rm -f test_curve9767 speed_curve9767 $(OBJ) $(OBJTEST) $(OBJSPEED)

curve9767.o: curve9767.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o curve9767.o curve9767.c

ecdh.o: ecdh.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o ecdh.o ecdh.c

hash.o: hash.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o hash.o hash.c

keygen.o: keygen.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o keygen.o keygen.c

ops_ref.o: ops_ref.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o ops_ref.o ops_ref.c

scalar_ref.o: scalar_ref.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o scalar_ref.o scalar_ref.c

sha3.o: sha3.c sha3.h
	$(CC) $(CFLAGS) -c -o sha3.o sha3.c

sign.o: sign.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o sign.o sign.c

speed_ref.o: speed_ref.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o speed_ref.o speed_ref.c

test_curve9767.o: test_curve9767.c curve9767.h inner.h sha3.h
	$(CC) $(CFLAGS) -c -o test_curve9767.o test_curve9767.c
