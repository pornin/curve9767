# Curve9767

This is the Curve9767 reference implementation. Curve9767 is a prime-order
elliptic curve defined in finite field GF(9767^19).

Code source is in `src/`. Compile with the `Makefile` for the reference
C code; use `Makefile.cm0` for the code optimized for the ARM Cortex-M0+.
The `ops_ref.c` file is used only for the C code; the `ops_arm.c` and
`ops_cm0.s` are used only for the M0+ implementation.

The `curve9767.h` file contains the public API. The `inner.h` file
declares functions that should not be called externally. The `sha3.c`
and `sha3.h` file are a portable stand-alone SHA3/SHAKE implementation.

Benchmark code for ARM Cortex-M0+ is in `bench-cm0/` and can be used on
a SAM D20 Xplained Pro board. The header files in the `sysinc/`
sub-directory have their own licensing requirements and should not be
blindly copied and reused. The rest of the code in this repository is
provided under MIT license.

In `extra/` are located a few scripts used to choose the base prime and
the curve parameters, and generate test vectors.

**All of this is meant for research purposes. Use at your own risk.**
