# Curve9767

This is the Curve9767 reference implementation.

**All of this is meant for research purposes only. Use at your own
risk.**

## Curve Definition

Curve9767 is a prime-order elliptic curve defined in finite field
GF(9767^19). This specific field was chosen because it supports
efficient implementations on small 32-bit architectures, in
particular the ARM Cortex-M0 and M0+ CPU; the choice of the modulus
(9767) allows for mutualizing most Montgomery reductions, the bulk
of the operation (multiplication and squarings) being done with plain
integers over 32 bits, using the fast 1-cycle `muls` opcode.

Since this is an extension field, it supports fast inversion (with a
cost about six times that of a multiplication). We use it to implement
curve operations in affine coordinates, on a curve with a short
Weierstraß equation and a prime order. The field also allows fast square
root and cube root extractions, leading to efficient point compression
and constant-time hash-to-curve.

All the details about the field choice, curve choice, and involved
algorithms, are detailed in the accompanying
[whitepaper](doc/curve9767.pdf).

## Source Code

Source code is in the [`src/`](src/) directory. Compile with the
`Makefile` for the reference C code; use `Makefile.cm0` for the code
optimized for the ARM Cortex-M0+, and `Makefile.cm4` for the code
optimized for the ARM Cortex-M4. Depending on the target architecture,
different files may be used:

  - `ops_ref.c` is used only for the reference C code.
  - `ops_arm.c` is used only for the ARM Cortex-M0+ and M4 implementations.
  - `ops_cm0.s` is used only for the ARM Cortex-M0+ implementation.
  - `ops_cm4.s` and `sha3_cm4.c` are used only for the ARM Cortex-M4
    implementation.

Compilation produces an executable binary which runs tests. In the case
of the ARM implementations, the C compiler is invoked under the name
`arm-linux-gcc`; for cross-compilation, a suitable GCC and minimal libc
can be obtained from the [BUildroot project](https://buildroot.org/),
and the resulting executable can run with [QEMU](https://www.qemu.org/)
("user" emulator, simply use the `qemu-arm` command). QEMU provides a
reliable emulation environment, perfectly usable for development, with
the important caveats that QEMU will allow unaligned accesses that would
trigger CPU exceptions on the ARM Cortex-M0+, and emulation is not cycle
accurate and thus does not permit benchmarking.

The [`curve9767.h`](src/curve9767.h) file contains the public API. The
`inner.h` file declares functions that should not be called externally.
The `sha3.c` and `sha3.h` file are a portable stand-alone SHA3/SHAKE
implementation.

## Documentation

The header files, especially `curve9767.h`, are heavily commented and
define the API.

The [`doc/`](doc/) directory contains the whitepaper: LaTeX source file
(`curve9767.tex` and style file `llncs.cls`) and resulting PDF
(`curve9767.pdf`).

In the [`extra/`](extra/) directory are located a few extra scripts
and files:

  - `findcurve.gp`: [PARI/GP](https://pari.math.u-bordeaux.fr/) script
    that looks for prime order curves in the chosen field.

  - `findprime.sage`: [SageMath](https://www.sagemath.org/) script that
    finds the "best" small primes and degrees for the implementation
    techniques used in Curve9767.

  - `mktests.sage`: another SageMath script. It produces the test vectors
    detailed in the `test-vectors.txt` file.

## Benchmarks

Benchmark code for ARM Cortex-M0+ is in [`bench-cm0/`](bench-cm0/) and
can be used on a SAM D20 Xplained Pro board. The header files in the
`sysinc/` sub-directory have their own licensing requirements and should
not be blindly copied and reused. The rest of the code in this
repository is provided under MIT license (Informally, I do not care much
about reuse of my code by anybody, but I want to make it clear that if
anything breaks, it's not my fault, and you acknowledge that; my
understanding is that the MIT license exactly provides that guarantee).

Benchmark code for ARM Cortex-M4 is in [`bench-cm4/`](bench-cm4/) and
was written for the STM32F407 "discovery" board. Hardware abstraction
is provided with [LibOpenCM3](https://libopencm3.org/); `libopencm3`
must first be downloaded and compiled separately. The `Makefile` in the
`bench-cm4/` directory expects the `libopencm3/` directory to be located
immediately below the main `curve9767/` directory (i.e. as a sibling
folder to `bench-cm4/`).

The following execution times are specified in clock cycles. Hardware
configurations:

  - ARM Cortex-M0+: SAM D20 Xplained Pro board (ATSAMD20J18
    microcontroller), clocked at 8 MHz, zero wait state.

  - ARM Cortex-M4: STM32F4 "discovery" board (STM32F407VG-DISC1),
    clocked at 24 MHz, zero wait state (I-cache and D-cache are enabled,
    but disabling them does not change timings).

Reported times include all opcodes that are part of the function,
including the final "ret" (`bx lr` or `pop { pc }`, in ARM assembly);
however, the costs of the call opcode itself (`bl` in ARM) and the
push/removal of function arguments are not accounted as part of the
reported costs. For the operations marked with "(\*)", an internal ABI
is used, in which there are no callee-saved registers.

| Field operations                          | Cortex-M0+ | Cortex-M4 |
| :---------------------------------------- | ---------: | --------: |
| multiplication (\*)                       |       1574 |       628 |
| squaring (\*)                             |        994 |       395 |
| inversion (\*)                            |       9508 |      4115 |
| square root extraction (\*)               |      26962 |     11143 |
| test quadratic residue status (\*)        |       9341 |      4031 |
| cube root extraction (\*)                 |      31163 |     12763 |

| Base curve operations                     | Cortex-M0+ | Cortex-M4 |
| :---------------------------------------- | ---------: | --------: |
| Generic curve point addition              |      16331 |      7371 |
| Curve point x2 (doubling)                 |      16337 |      7379 |
| Curve point x4                            |      30368 |     13008 |
| Curve point x8                            |      41760 |     18066 |
| Curve point x16                           |      53152 |     23124 |
| Lookup in 8-point window                  |        777 |       500 |
| Curve point decoding (decompression)      |      32228 |     14192 |
| Curve point encoding (compression)        |       1527 |      1267 |
| Generic point multiplication              |    4493999 |   1992026 |
| Generator multiplication                  |    1877847 |    846946 |
| Two combined point multiplications        |    5590769 |   2499082 |
| MapToField                                |      20082 |     14518 |
| Icart's map                               |      50976 |     22097 |
| Hash 48 bytes to a curve point            |     195211 |    104427 |

| High-level operations                     | Cortex-M0+ | Cortex-M4 |
| :---------------------------------------- | ---------: | --------: |
| ECDH: key pair generation                 |    1937792 |    887624 |
| ECDH: compute shared secret               |    4598756 |   2054928 |
| Schnorr signature: generate               |    2054110 |    965957 |
| Schnorr signature: verify                 |    5688642 |   2565659 |
