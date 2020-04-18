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
    microcontroller), clocked at 8 MHz, zero wait state. Compiler
    is GCC-7.3.1, with flags: `-Os -mthumb -mlong-calls -mcpu=cortex-m0plus`

  - ARM Cortex-M4: STM32F4 "discovery" board (STM32F407VG-DISC1),
    clocked at 24 MHz, zero wait state (I-cache and D-cache are enabled,
    but disabling them does not change timings). Compiler is GCC-7.3.1,
    with flags: `-Os -mthumb -mcpu=cortex-m4 -mfloat-abi=hard
    -mfpu=fpv4-sp-d16`

  - x86+AMD64 (Coffee Lake): Intel i5-8259U at 2.3 GHz, TurboBoost is
    disabled. Compiler is Clang-9.0.0, with flags: `-O3 -mavx2 -mlzcnt`

Reported times include all opcodes that are part of the function,
including the final "ret" (`bx lr` or `pop { pc }`, in ARM assembly);
however, the costs of the call opcode itself (`bl` in ARM) and the
push/removal of function arguments are not accounted as part of the
reported costs. For the operations marked with "(\*)", on ARM systems,
an internal ABI is used, in which there are no callee-saved registers.

The variable-time functions (tagged "vartime") are averages over 100
randomized samples. Reported figures therefore have relatively low
precision, certainly no better than 5%.

On x86, since the CPU uses out-of-order processing of instructions, the
exact cost of a routine is hard to define, especially for fast routines
such as field element multiplication: the routine opcodes may execute
concurrently with some opcodes pertaining to the previous or next
routine in the instruction sequence. Moreover, the API forces encoding
into arrays of 16-bit values, whereas internally calls may be inlined
and values need not leave AVX2 registers at all. To obtain the values
provided below, the following process was used:

  - A warm-up is first performed by performing point multiplications
    for a total time of at least 100 milliseconds. This is meant to
    ensure that the CPU wakes from power-saving modes and reaches its
    nominal frequency (2.3 GHz on our test system; TurboBoost is
    disabled).

  - The benchmarked function is first invoked 200 times, so that all
    caches are filled (including branch prediction).

  - The function is again invoked 200 times. Each invocation is preceded
    and followed by a read of the timestamp counter; such a read is a
    combination of a serializing opcode (`_mm_lfence()`) and then the
    read of the counter (`__rdtsc()`). The difference between the two
    readings is deemed to be the function execution time. Negative
    values (which could be possible in case of unexpected migration
    from one CPU core to another) are ignored. Otherwise, the best
    value (i.e. the smallest) is returned.

  - For field operations (`gf_*` functions), an additional dummy
    function is called, with the same API. The dummy function uses the
    same decoding and encoding steps, but is otherwise trivial;
    therefore, the benchmark for the dummy function is assumed to
    account for the measure overhead (function call, read of the
    counter, decoding/encoding). In practice, this overhead is about 36
    cycles. The figures below, for the `gf_*` functions, are then
    obtained by subtracting 36 from the values reported by the benchmark
    program.

  - For the variable-time operations (tagged "vartime"), 200 randomized
    samples are used. Each of the 200 calls is measured. Values which
    are more than 20 times the median time, or less than 1/20th of the
    median time, are rejected; then, the average of the remaining values
    is taken. The whole process is done 10 times and the best average is
    returned. Since only 200 samples are used, there is some unavoidable
    variance and the reported values should not considered to have
    accuracy better than 5% or so.

Values marked with "~" are approximate; this is used for the "vartime"
operations. Also, as explained above, the exact time taken by a given
routine is not a well-defined notion on platforms with out-of-order
execution, notably the modern x86 processors; therefore, there cannot
be, in a strong sense, a non-approximate measure of that time.

| Field operations                          | Cortex-M0+ | Cortex-M4 | x86+AVX2 (Coffee Lake) |
| :---------------------------------------- | ---------: | --------: | ---------------------: |
| Multiplication (\*)                       |       1574 |       628 |                    134 |
| Squaring (\*)                             |        994 |       396 |                    134 |
| Inversion (\*)                            |       9508 |      4108 |                   1026 |
| Square root extraction (\*)               |      26962 |     11139 |                   3262 |
| Test quadratic residue status (\*)        |       9341 |      4026 |                    992 |
| Cube root extraction (\*)                 |      31163 |     12759 |                   3442 |
| Lattice basis reduction (vartime)         |    ~106300 |    ~92010 |                 ~15250 |

| Base curve operations                     | Cortex-M0+ | Cortex-M4 | x86+AVX2 (Coffee Lake) |
| :---------------------------------------- | ---------: | --------: | ---------------------: |
| Generic curve point addition              |      16331 |      7369 |                   1710 |
| Curve point x2 (doubling)                 |      16337 |      7377 |                   1724 |
| Curve point x4                            |      30368 |     13005 |                   3230 |
| Curve point x8                            |      41760 |     18063 |                   4336 |
| Curve point x16                           |      53152 |     23123 |                   5466 |
| Lookup in 8-point window                  |        777 |       500 |                        |
| Curve point decoding (decompression)      |      32228 |     14188 |                   3928 |
| Curve point encoding (compression)        |       1527 |      1267 |                    198 |
| Generic point multiplication              |    4493999 |   1991892 |                 386842 |
| Generator multiplication                  |    1877847 |    846837 |                 167110 |
| Two combined point multiplications        |    5590769 |   2498799 |                 536780 |
| MapToField                                |      20082 |     14518 |                   7184 |
| Icart's map                               |      50976 |     22084 |                        |
| Hash 48 bytes to a curve point            |     195211 |    104397 |                        |

| High-level operations                     | Cortex-M0+ | Cortex-M4 | x86+AVX2 (Coffee Lake) |
| :---------------------------------------- | ---------: | --------: | ---------------------: |
| ECDH: key pair generation                 |    1937792 |    887520 |                 172660 |
| ECDH: compute shared secret               |    4598756 |   2054792 |                 392714 |
| Schnorr signature: generate               |    2054110 |    965850 |                 182892 |
| Schnorr signature: verify                 |    5688642 |   2565404 |                 543176 |
| Schnorr signature: verify (vartime)       |   ~3818000 |  ~1779000 |                ~380500 |
