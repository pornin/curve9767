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
[whitepaper](extra/curve9767.pdf).

## Source Code

Source code is in the [`src/`](src/) directory. Compile with the
`Makefile` for the reference C code; use `Makefile.cm0` for the code
optimized for the ARM Cortex-M0+. The `ops_ref.c` file is used only for
the C code; the `ops_arm.c` and `ops_cm0.s` are used only for the M0+
implementation. The other source files are used for both. Compilation
produces an executable binary which runs tests.

The [`curve9767.h`](src/curve9767.h) file contains the public API. The
`inner.h` file declares functions that should not be called externally.
The `sha3.c` and `sha3.h` file are a portable stand-alone SHA3/SHAKE
implementation.

Benchmark code for ARM Cortex-M0+ is in [`bench-cm0/`](bench-cm0/) and
can be used on a SAM D20 Xplained Pro board. The header files in the
`sysinc/` sub-directory have their own licensing requirements and should
not be blindly copied and reused. The rest of the code in this
repository is provided under MIT license (Informally, I do not care much
about reuse of my code by anybody, but I want to make it clear that if
anything breaks, it's not my fault, and you acknowledge that; my
understanding is that the MIT license exactly provides that guarantee).

In the [`extra/`](extra/) directory are located a few extra scripts
and files:

  - `findcurve.gp`: [PARI/GP](https://pari.math.u-bordeaux.fr/) script
    that looks for prime order curves in the chosen field.

  - `findprime.sage`: [SageMath](https://www.sagemath.org/) script that
    finds the "best" small primes and degrees for the implementation
    techniques used in Curve9767.

  - `mktests.sage`: another SageMath script. It produces the test vectors
    detailed in the `test-vectors.txt` file.

The [`doc/`](doc/) directory contains the whitepaper: LaTeX source file
(`curve9767.tex` and style file `llncs.cls`) and resulting PDF
(`curve9767.pdf`).
