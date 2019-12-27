@ =======================================================================
@ =======================================================================

	.syntax	unified
	.cpu	cortex-m0
	.file	"ops_cm0.s"
	.text

@ =======================================================================

@ Perform Montgomery reduction.
@   rx     register to reduce (within r0..r7)
@   rp     contains p = 9767 (unmodified)
@   rp1i   contains p1i = 3635353193 (unmodified)
@ Input value x must be between 1 and 3654952486 (inclusive); output
@ value is equal to x/(2^32) mod 9767, and is between 1 and 9767 (inclusive).
@ Cost: 5
.macro MONTYRED  rx, rp, rp1i
	muls	\rx, \rp1i
	lsrs	\rx, #16
	muls	\rx, \rp
	lsrs	\rx, #16
	adds	\rx, #1
.endm

@ Perform normal reduction (not Montgomery).
@   rx     register to reduce (within r0..r7)
@   rp     contains p = 9767 (unmodified)
@   rp2i   contains p2i = 439742 (unmodified)
@ Input value x must be between 1 and 509232 (inclusive); output
@ value is equal to x mod 9767, and is between 1 and 9767 (inclusive).
@ Cost: 5
.macro BASERED  rx, rp, rp2i
	muls	\rx, \rp2i
	lsrs	\rx, #16
	muls	\rx, \rp
	lsrs	\rx, #16
	adds	\rx, #1
.endm

@ Montgomery multiplication:
@   rx <- (rx*ry)/(2^32) mod p  (in the 1..p range)
@ rp and rp1i must contain p and p1i, respectively.
@ Cost: 6
.macro MONTYMUL  rx, ry, rp, rp1i
	muls	\rx, \ry
	MONTYRED  \rx, \rp, \rp1i
.endm

@ Montgomery multiplication with Frobenius constant. The constant has
@ been pre-multiplied by p1i.
@   rx   value to multiply
@   ry   Frobenius constant (unmodified)
@   rp   modulus p = 9767 (unmodified)
@ Cost: 5
.macro FROBMUL   rx, ry, rp
	muls	\rx, \ry
	lsrs	\rx, #16
	muls	\rx, \rp
	lsrs	\rx, #16
	adds	\rx, #1
.endm

@ MUL_SET_1x5 computes:
@    s1 = r0*r3
@    s2 = r0*r4
@    s3 = r0*r5
@    s4 = r0*r6
@    r0 = r0*r2
@ Parameters s0..s4 are within high registers: r8, r10, r11, r12, r14.
@ r1..r6 are conserved. r7 is scratch. s0 is not actually used.
@ Cost: 13
.macro MUL_SET_1x5 s0, s1, s2, s3, s4
	movs	r7, r0
	muls	r7, r3
	mov	\s1, r7
	movs	r7, r0
	muls	r7, r4
	mov	\s2, r7
	movs	r7, r0
	muls	r7, r5
	mov	\s3, r7
	movs	r7, r0
	muls	r7, r6
	mov	\s4, r7
	muls	r0, r2
.endm

@ MUL_ACC_1x5 computes:
@    s1 = s1 + r0*r3
@    s2 = s2 + r0*r4
@    s3 = s3 + r0*r5
@    s4 =      r0*r6
@    r0 = s0 + r0*r2
@ Parameters s0..s4 are within registers: r8, r10, r11, r12, r14.
@ r1..r6 are conserved. r7 is scratch. s0 is unmodified.
@ Cost: 14
.macro MUL_ACC_1x5 s0, s1, s2, s3, s4
	movs	r7, r0
	muls	r7, r3
	add	\s1, r7
	movs	r7, r0
	muls	r7, r4
	add	\s2, r7
	movs	r7, r0
	muls	r7, r5
	add	\s3, r7
	movs	r7, r0
	muls	r7, r6
	mov	\s4, r7
	muls	r0, r2
	add	r0, \s0
.endm

@ MUL_SET_1x4 computes:
@    s1 = r0*r3
@    s2 = r0*r4
@    s3 = r0*r5
@    r0 = r0*r2
@ Parameters s0..s3 are within high registers: r8, r10, r11, r12, r14.
@ r1..r6 are conserved. r7 is scratch. s0 is not actually used.
@ Cost: 10
.macro MUL_SET_1x4 s0, s1, s2, s3
	movs	r7, r0
	muls	r7, r3
	mov	\s1, r7
	movs	r7, r0
	muls	r7, r4
	mov	\s2, r7
	movs	r7, r0
	muls	r7, r5
	mov	\s3, r7
	muls	r0, r2
.endm

@ MUL_ACC_1x4 computes:
@    s1 = s1 + r0*r3
@    s2 = s2 + r0*r4
@    s3 =      r0*r5
@    r0 = s0 + r0*r2
@ Parameters s0..s3 are within registers: r8, r10, r11, r12, r14.
@ r1..r6 are conserved. r7 is scratch. s0 is unmodified.
@ Cost: 11
.macro MUL_ACC_1x4 s0, s1, s2, s3
	movs	r7, r0
	muls	r7, r3
	add	\s1, r7
	movs	r7, r0
	muls	r7, r4
	add	\s2, r7
	movs	r7, r0
	muls	r7, r5
	mov	\s3, r7
	muls	r0, r2
	add	r0, \s0
.endm

@ One step of MUL_SET_10x10 (phase 1).
@ Cost: 18
.macro STEP_MUL_SET_10x10_P1  s0, s1, s2, s3, s4, kc, i
	ldrh	r0, [r1, #(2 * (\i))]
	MUL_ACC_1x5 \s0, \s1, \s2, \s3, \s4
	str	r0, [sp, #(4 * ((\kc) + (\i)))]
.endm

@ One step of MUL_SET_10x10 (phase 2).
@ Cost: 21
.macro STEP_MUL_SET_10x10_P2  s0, s1, s2, s3, s4, kc, i
	ldrh	r0, [r1, #(2 * (\i))]
	MUL_ACC_1x5 \s0, \s1, \s2, \s3, \s4
	ldr	r7, [sp, #(4 * ((\kc) + (\i)))]
	adds	r0, r0, r7
	str	r0, [sp, #(4 * ((\kc) + (\i)))]
.endm

@ Let A = a0..a9; pointer to a0 is in r0.
@ Let B = b0..b9; pointer to b0 is in r1.
@ This macro computes A*B, and stores it in the stack buffer at index kc.
@ kt is the index of a free stack slot.
@ If trunc is non-zero, then the top word is not computed or written, i.e.
@ only 18 words of output are produced.
@ Upon exit, r1 is unchanged; r0 is consumed.
@ Cost: 408 (403 if trunc)
.macro MUL_SET_10x10  kc, kt, trunc
	@ First 10x5 multiplication.
	@ Load a0..a4 into registers r2..r6.
	ldm	r0!, { r2, r3, r4 }
	uxth	r6, r4
	lsrs	r5, r3, #16
	uxth	r4, r3
	lsrs	r3, r2, #16
	uxth	r2, r2
	str	r0, [sp, #(4 * (\kt))]
	ldrh	r0, [r1, #(2 * 0)]
	MUL_SET_1x5 r8, r10, r11, r12, r14
	str	r0, [sp, #(4 * ((\kc) +  0))]
	STEP_MUL_SET_10x10_P1  r10, r11, r12, r14, r8, (\kc), 1
	STEP_MUL_SET_10x10_P1  r11, r12, r14, r8, r10, (\kc), 2
	STEP_MUL_SET_10x10_P1  r12, r14, r8, r10, r11, (\kc), 3
	STEP_MUL_SET_10x10_P1  r14, r8, r10, r11, r12, (\kc), 4
	STEP_MUL_SET_10x10_P1  r8, r10, r11, r12, r14, (\kc), 5
	STEP_MUL_SET_10x10_P1  r10, r11, r12, r14, r8, (\kc), 6
	STEP_MUL_SET_10x10_P1  r11, r12, r14, r8, r10, (\kc), 7
	STEP_MUL_SET_10x10_P1  r12, r14, r8, r10, r11, (\kc), 8
	ldrh	r0, [r1, #(2 * 9)]
	muls	r2, r0
	add	r2, r14
	muls	r3, r0
	add	r3, r8
	muls	r4, r0
	add	r4, r10
	muls	r5, r0
	add	r5, r11
	muls	r6, r0
	add	r0, sp, #(4 * ((\kc) + 9))
	stm	r0!, { r2, r3, r4, r5, r6 }

	@ Second 10x5 multiplication.
	ldr	r0, [sp, #(4 * (\kt))]
	subs	r0, #4
	ldm	r0!, { r2, r3, r4 }
	lsrs	r6, r4, #16
	uxth	r5, r4
	lsrs	r4, r3, #16
	uxth	r3, r3
	lsrs	r2, r2, #16
	ldrh	r0, [r1, #(2 * 0)]
	MUL_SET_1x5 r8, r10, r11, r12, r14
	ldr	r7, [sp, #(4 * ((\kc) +  5))]
	adds	r0, r0, r7
	str	r0, [sp, #(4 * ((\kc) +  5))]
	STEP_MUL_SET_10x10_P2  r10, r11, r12, r14, r8, ((\kc) + 5), 1
	STEP_MUL_SET_10x10_P2  r11, r12, r14, r8, r10, ((\kc) + 5), 2
	STEP_MUL_SET_10x10_P2  r12, r14, r8, r10, r11, ((\kc) + 5), 3
	STEP_MUL_SET_10x10_P2  r14, r8, r10, r11, r12, ((\kc) + 5), 4
	STEP_MUL_SET_10x10_P2  r8, r10, r11, r12, r14, ((\kc) + 5), 5
	STEP_MUL_SET_10x10_P2  r10, r11, r12, r14, r8, ((\kc) + 5), 6
	STEP_MUL_SET_10x10_P2  r11, r12, r14, r8, r10, ((\kc) + 5), 7

	.if (\trunc) != 0

	@ If truncating, then we don't produce the final product,
	@ and we can consume some registers earlier.
	ldrh	r0, [r1, #(2 * 8)]
	movs	r7, r0
	muls	r7, r3
	add	r14, r7
	movs	r7, r0
	muls	r7, r4
	add	r8, r7
	movs	r7, r0
	muls	r7, r5
	add	r10, r7
	muls	r6, r0
	muls	r0, r2
	add	r0, r12
	ldr	r7, [sp, #(4 * ((\kc) + 13))]
	adds	r0, r0, r7
	ldrh	r7, [r1, #(2 * 9)]
	muls	r2, r7
	add	r2, r14
	muls	r3, r7
	add	r3, r8
	muls	r4, r7
	add	r4, r10
	muls	r5, r7
	add	r5, r6
	add	r7, sp, #(4 * ((\kc) + 13))
	stm	r7!, { r0, r2, r3, r4, r5 }

	.else

	STEP_MUL_SET_10x10_P2  r12, r14, r8, r10, r11, ((\kc) + 5), 8
	ldrh	r0, [r1, #(2 * 9)]
	muls	r2, r0
	add	r2, r14
	muls	r3, r0
	add	r3, r8
	muls	r4, r0
	add	r4, r10
	muls	r5, r0
	add	r5, r11
	muls	r6, r0
	add	r0, sp, #(4 * ((\kc) + 14))
	stm	r0!, { r2, r3, r4, r5, r6 }

	.endif
.endm

@ One step of MUL_SET_9x9 (phase 1).
@ Cost: 18
.macro STEP_MUL_SET_9x9_P1  s0, s1, s2, s3, s4, kc, i
	ldrh	r0, [r1, #(2 * (\i))]
	MUL_ACC_1x5 \s0, \s1, \s2, \s3, \s4
	str	r0, [sp, #(4 * ((\kc) + (\i)))]
.endm

@ One step of MUL_SET_9x9 (phase 2).
@ Cost: 18
.macro STEP_MUL_SET_9x9_P2  s0, s1, s2, s3, kc, i
	ldrh	r0, [r1, #(2 * (\i))]
	MUL_ACC_1x4 \s0, \s1, \s2, \s3
	ldr	r7, [sp, #(4 * ((\kc) + (\i)))]
	adds	r0, r0, r7
	str	r0, [sp, #(4 * ((\kc) + (\i)))]
.endm

@ Let A = a0..a8; pointer to a0 is in r0.
@ Let B = b0..b8; pointer to b0 is in r1.
@ This macro computes A*B, and stores it in the stack buffer at index kc.
@ kt is the index of a free stack slot.
@ Upon exit, r1 is unchanged; r0 is consumed.
@ Cost: 341
.macro MUL_SET_9x9  kc, kt
	@ First 9x5 multiplication.
	ldm	r0!, { r2, r3, r4 }
	uxth	r6, r4
	lsrs	r5, r3, #16
	uxth	r4, r3
	lsrs	r3, r2, #16
	uxth	r2, r2
	str	r0, [sp, #(4 * (\kt))]
	ldrh	r0, [r1, #(2 * 0)]
	MUL_SET_1x5 r8, r10, r11, r12, r14
	str	r0, [sp, #(4 * ((\kc) +  0))]
	STEP_MUL_SET_9x9_P1  r10, r11, r12, r14, r8, (\kc), 1
	STEP_MUL_SET_9x9_P1  r11, r12, r14, r8, r10, (\kc), 2
	STEP_MUL_SET_9x9_P1  r12, r14, r8, r10, r11, (\kc), 3
	STEP_MUL_SET_9x9_P1  r14, r8, r10, r11, r12, (\kc), 4
	STEP_MUL_SET_9x9_P1  r8, r10, r11, r12, r14, (\kc), 5
	STEP_MUL_SET_9x9_P1  r10, r11, r12, r14, r8, (\kc), 6
	STEP_MUL_SET_9x9_P1  r11, r12, r14, r8, r10, (\kc), 7
	ldrh	r0, [r1, #(2 * 8)]
	muls	r2, r0
	add	r2, r12
	muls	r3, r0
	add	r3, r14
	muls	r4, r0
	add	r4, r8
	muls	r5, r0
	add	r5, r10
	muls	r6, r0
	add	r0, sp, #(4 * ((\kc) + 8))
	stm	r0!, { r2, r3, r4, r5, r6 }

	@ Second 9x4 multiplication.
	ldr	r0, [sp, #(4 * (\kt))]
	subs	r0, #4
	ldm	r0!, { r2, r3, r4 }
	uxth	r5, r4
	lsrs	r4, r3, #16
	uxth	r3, r3
	lsrs	r2, r2, #16
	ldrh	r0, [r1, #(2 * 0)]
	MUL_SET_1x4 r8, r10, r11, r12
	ldr	r7, [sp, #(4 * ((\kc) +  5))]
	adds	r0, r0, r7
	str	r0, [sp, #(4 * ((\kc) +  5))]
	STEP_MUL_SET_9x9_P2  r10, r11, r12, r8, ((\kc) + 5), 1
	STEP_MUL_SET_9x9_P2  r11, r12, r8, r10, ((\kc) + 5), 2
	STEP_MUL_SET_9x9_P2  r12, r8, r10, r11, ((\kc) + 5), 3
	STEP_MUL_SET_9x9_P2  r8, r10, r11, r12, ((\kc) + 5), 4
	STEP_MUL_SET_9x9_P2  r10, r11, r12, r8, ((\kc) + 5), 5
	STEP_MUL_SET_9x9_P2  r11, r12, r8, r10, ((\kc) + 5), 6
	STEP_MUL_SET_9x9_P2  r12, r8, r10, r11, ((\kc) + 5), 7
	ldrh	r0, [r1, #(2 * 8)]
	muls	r2, r0
	add	r2, r8
	muls	r3, r0
	add	r3, r10
	muls	r4, r0
	add	r4, r11
	muls	r5, r0
	add	r0, sp, #(4 * ((\kc) + 13))
	stm	r0!, { r2, r3, r4, r5 }
.endm

@ Square a 10-word polynomial (a0..a9) and write into the stack buffer.
@   r1   pointer to a0
@   kc   output index in stack buffer
@ If trunc is non-zero, then the top word is not computed or written, i.e.
@ only 18 words of output are produced.
@ On output, r1 is incremented by 20.
@ Cost: 219 (215 if trunc)
.macro SQR_SET_10x10  kc, trunc
	@ A = a0..a9

	@ Load a5..a9 into low registers and a0..a4 into high registers.
	ldm	r1!, { r3, r4, r5, r6, r7 }
	uxth	r0, r3
	mov	r8, r0
	lsrs	r0, r3, #16
	mov	r10, r0
	uxth	r0, r4
	mov	r11, r0
	lsrs	r0, r4, #16
	mov	r12, r0
	uxth	r0, r5
	mov	r14, r0
	lsrs	r3, r5, #16
	uxth	r4, r6
	lsrs	r5, r6, #16
	uxth	r6, r7
	lsrs	r7, r7, #16

	@ Principle: we compute output words one by one. Each computation
	@ involves products from two source coefficients; since we do not
	@ want to consume either coefficient, a 'mov' is involved. A
	@ further complication is that high registers cannot be used as
	@ operand to a 'muls'; thus, we need to make sure that no product
	@ for a given output word will use two values from high registers.
	@ We do so by swapping values between low and high at relevant times.

	@ a0..a9 = r8 r10 r11 r12 r14 r3 r4 r5 r6 r7
	@ scratch: r0 r2

	.if (\trunc) == 0
	@ a9*a9
	movs	r0, r7
	muls	r0, r7
	str	r0, [sp, #(4 * ((\kc) + 18))]
	.endif

	@ 2*a8*a9
	movs	r0, r7
	muls	r0, r6
	lsls	r0, #1
	str	r0, [sp, #(4 * ((\kc) + 17))]

	@ a8*a8 + 2*a7*a9
	movs	r0, r7
	muls	r0, r5
	lsls	r0, #1
	movs	r2, r6
	muls	r2, r6
	adds	r0, r2
	str	r0, [sp, #(4 * ((\kc) + 16))]

	@ 2*(a6*a9 + a7*a8)
	movs	r0, r7
	muls	r0, r4
	movs	r2, r6
	muls	r2, r5
	adds	r0, r2
	lsls	r0, #1
	str	r0, [sp, #(4 * ((\kc) + 15))]

	@ a7*a7 + 2*(a5*a9 + a6*a8)
	movs	r0, r7
	muls	r0, r3
	movs	r2, r6
	muls	r2, r4
	adds	r0, r2
	lsls	r0, #1
	movs	r2, r5
	muls	r2, r5
	adds	r0, r2
	str	r0, [sp, #(4 * ((\kc) + 14))]

	@ 2*(a4*a9 + a5*a8 + a6*a7)
	mov	r0, r14
	muls	r0, r7
	movs	r2, r6
	muls	r2, r3
	adds	r0, r2
	movs	r2, r5
	muls	r2, r4
	adds	r0, r2
	lsls	r0, #1
	str	r0, [sp, #(4 * ((\kc) + 13))]

	@ a6*a6 + 2*(a3*a9 + a4*a8 + a5*a7)
	mov	r0, r12
	muls	r0, r7
	mov	r2, r14
	muls	r2, r6
	adds	r0, r2
	movs	r2, r5
	muls	r2, r3
	adds	r0, r2
	lsls	r0, #1
	movs	r2, r4
	muls	r2, r4
	adds	r0, r2
	str	r0, [sp, #(4 * ((\kc) + 12))]

	@ 2*(a2*a9 + a3*a8 + a4*a7 + a5*a6)
	mov	r0, r11
	muls	r0, r7
	mov	r2, r12
	muls	r2, r6
	adds	r0, r2
	mov	r2, r14
	muls	r2, r5
	adds	r0, r2
	movs	r2, r4
	muls	r2, r3
	adds	r0, r2
	lsls	r0, #1
	str	r0, [sp, #(4 * ((\kc) + 11))]

	@ a5*a5 + 2*(a1*a9 + a2*a8 + a3*a7 + a4*a6)
	mov	r0, r10
	muls	r0, r7
	mov	r2, r11
	muls	r2, r6
	adds	r0, r2
	mov	r2, r12
	muls	r2, r5
	adds	r0, r2
	mov	r2, r14
	muls	r2, r4
	adds	r0, r2
	lsls	r0, #1
	movs	r2, r3
	muls	r2, r3
	adds	r0, r2
	str	r0, [sp, #(4 * ((\kc) + 10))]

	@ 2*(a0*a9 + a1*a8 + a2*a7 + a3*a6 + a4*a5)
	@ This is the last time we use a9, hence we can consume it.
	@ We also need to pull a4 into the low registers.
	mov	r0, r8
	muls	r0, r7
	mov	r2, r10
	muls	r2, r6
	adds	r0, r2
	mov	r2, r11
	muls	r2, r5
	adds	r0, r2
	mov	r2, r12
	muls	r2, r4
	adds	r0, r2
	mov	r7, r14
	movs	r2, r7
	muls	r2, r3
	adds	r0, r2
	lsls	r0, #1
	str	r0, [sp, #(4 * ((\kc) + 9))]

	@ a0..a8 = r8 r10 r11 r12 r7 r3 r4 r5 r6

	@ a4*a4 + 2*(a0*a8 + a1*a7 + a2*a6 + a3*a5)
	mov	r0, r8
	muls	r0, r6
	mov	r2, r10
	muls	r2, r5
	adds	r0, r2
	mov	r2, r11
	muls	r2, r4
	adds	r0, r2
	mov	r2, r12
	muls	r2, r3
	adds	r0, r2
	lsls	r0, #1
	movs	r2, r7
	muls	r2, r7
	adds	r0, r2
	str	r0, [sp, #(4 * ((\kc) + 8))]

	@ 2*(a0*a7 + a1*a6 + a2*a5 + a3*a4)
	@ a7 and a8 won't be used afterwards. We pull a3 into low registers.
	mov	r0, r8
	muls	r0, r5
	mov	r2, r10
	muls	r2, r4
	adds	r0, r2
	mov	r2, r11
	muls	r2, r3
	adds	r0, r2
	mov	r6, r12
	movs	r2, r6
	muls	r2, r7
	adds	r0, r2
	lsls	r0, #1
	str	r0, [sp, #(4 * ((\kc) + 7))]

	@ a0..a6 = r8 r10 r11 r6 r7 r3 r4

	@ a3*a3 + 2*(a0*a6 + a1*a5 + a2*a4)
	mov	r0, r8
	muls	r0, r4
	mov	r2, r10
	muls	r2, r3
	adds	r0, r2
	mov	r2, r11
	muls	r2, r7
	adds	r0, r2
	lsls	r0, #1
	movs	r2, r6
	muls	r2, r6
	adds	r0, r2
	str	r0, [sp, #(4 * ((\kc) + 6))]

	@ 2*(a0*a5 + a1*a4 + a2*a3)
	@ a5 and a6 won't be used afterwards. We pull a2 into low registers.
	mov	r0, r8
	muls	r0, r3
	mov	r2, r10
	muls	r2, r7
	adds	r0, r2
	mov	r5, r11
	movs	r2, r5
	muls	r2, r6
	adds	r0, r2
	lsls	r0, #1
	str	r0, [sp, #(4 * ((\kc) + 5))]

	@ a0..a4 = r8 r10 r5 r6 r7

	@ We pull back a3 and a4 into low registers.
	mov	r3, r8
	mov	r4, r10

	@ a0..a4 = r3 r4 r5 r6 r7

	@ We now finish the computations in low registers only. We can
	@ consume the values as we go.

	@ a2*a2 + 2*(a0*a4 + a1*a3) -> r7
	muls	r7, r3
	movs	r0, r6
	muls	r0, r4
	adds	r7, r0
	lsls	r7, #1
	movs	r0, r5
	muls	r0, r5
	adds	r7, r0

	@ 2*(a0*a3 + a1*a2) -> r6
	muls	r6, r3
	movs	r0, r5
	muls	r0, r4
	adds	r6, r0
	lsls	r6, #1

	@ a1*a1 + 2*a0*a2 -> r5
	muls	r5, r3
	lsls	r5, #1
	movs	r0, r4
	muls	r0, r4
	adds	r5, r0

	@ 2*a0*a1 -> r4
	muls	r4, r3
	lsls	r4, #1

	@ a0*a0 -> r3
	muls	r3, r3

	@ Write the five final values.
	add	r0, sp, #(4 * ((\kc) + 0))
	stm	r0!, { r3, r4, r5, r6, r7 }
.endm

@ Square a 9-word polynomial (a0..a8) and write into the stack buffer.
@   r1   pointer to a0
@   kc   output index in stack buffer
@ On output, r1 has been incremented by 20.
@ Cost: 182
.macro SQR_SET_9x9  kc
	@ A = a0..a8

	@ Load a0..a3 into high registers, and a4..a8 into low registers.
	ldm	r1!, { r3, r4, r5, r6, r7 }
	uxth	r0, r3
	mov	r8, r0
	lsrs	r0, r3, #16
	mov	r10, r0
	uxth	r0, r4
	mov	r11, r0
	lsrs	r0, r4, #16
	mov	r12, r0
	uxth	r3, r5
	lsrs	r4, r5, #16
	uxth	r5, r6
	lsrs	r6, r6, #16
	uxth	r7, r7

	@ a0..a8 = r8 r10 r11 r12 r3 r4 r5 r6 r7

	@ a8*a8
	movs	r0, r7
	muls	r0, r7
	str	r0, [sp, #(4 * ((\kc) + 16))]

	@ 2*a7*a8
	movs	r0, r7
	muls	r0, r6
	lsls	r0, #1
	str	r0, [sp, #(4 * ((\kc) + 15))]

	@ a7*a7 + 2*a6*a8
	movs	r0, r7
	muls	r0, r5
	lsls	r0, #1
	movs	r2, r6
	muls	r2, r6
	adds	r0, r2
	str	r0, [sp, #(4 * ((\kc) + 14))]

	@ 2*(a5*a8 + a6*a7)
	movs	r0, r7
	muls	r0, r4
	movs	r2, r6
	muls	r2, r5
	adds	r0, r2
	lsls	r0, #1
	str	r0, [sp, #(4 * ((\kc) + 13))]

	@ a6*a6 + 2*(a4*a8 + a5*a7)
	movs	r0, r7
	muls	r0, r3
	movs	r2, r6
	muls	r2, r4
	adds	r0, r2
	lsls	r0, #1
	movs	r2, r5
	muls	r2, r5
	adds	r0, r2
	str	r0, [sp, #(4 * ((\kc) + 12))]

	@ 2*(a3*a8 + a4*a7 + a5*a6)
	mov	r0, r12
	muls	r0, r7
	movs	r2, r6
	muls	r2, r3
	adds	r0, r2
	movs	r2, r5
	muls	r2, r4
	adds	r0, r2
	lsls	r0, #1
	str	r0, [sp, #(4 * ((\kc) + 11))]

	@ a5*a5 + 2*(a2*a8 + a3*a7 + a4*a6)
	mov	r0, r11
	muls	r0, r7
	mov	r2, r12
	muls	r2, r6
	adds	r0, r2
	movs	r2, r5
	muls	r2, r3
	adds	r0, r2
	lsls	r0, #1
	movs	r2, r4
	muls	r2, r4
	adds	r0, r2
	str	r0, [sp, #(4 * ((\kc) + 10))]

	@ 2*(a1*a8 + a2*a7 + a3*a6 + a4*a5)
	mov	r0, r10
	muls	r0, r7
	mov	r2, r11
	muls	r2, r6
	adds	r0, r2
	mov	r2, r12
	muls	r2, r5
	adds	r0, r2
	movs	r2, r4
	muls	r2, r3
	adds	r0, r2
	lsls	r0, #1
	str	r0, [sp, #(4 * ((\kc) + 9))]

	@ a4*a4 + 2*(a0*a8 + a1*a7 + a2*a6 + a3*a5)
	@ a8 won't be used afterwards.
	mov	r0, r8
	muls	r0, r7
	mov	r2, r10
	muls	r2, r6
	adds	r0, r2
	mov	r2, r11
	muls	r2, r5
	adds	r0, r2
	mov	r2, r12
	muls	r2, r4
	adds	r0, r2
	lsls	r0, #1
	movs	r2, r3
	muls	r2, r3
	adds	r0, r2
	str	r0, [sp, #(4 * ((\kc) + 8))]

	@ 2*(a0*a7 + a1*a6 + a2*a5 + a3*a4)
	@ a7 won't be used afterwards. We pull a3 into low registers.
	mov	r0, r8
	muls	r0, r6
	mov	r2, r10
	muls	r2, r5
	adds	r0, r2
	mov	r2, r11
	muls	r2, r4
	adds	r0, r2
	mov	r6, r12
	movs	r2, r6
	muls	r2, r3
	adds	r0, r2
	lsls	r0, #1
	str	r0, [sp, #(4 * ((\kc) + 7))]

	@ a0..a6 = r8 r10 r11 r6 r3 r4 r5

	@ a3*a3 + 2*(a0*a6 + a1*a5 + a2*a4)
	@ a6 won't be used afterwards. We pull a2 into low registers.
	mov	r0, r8
	muls	r0, r5
	mov	r2, r10
	muls	r2, r4
	adds	r0, r2
	mov	r5, r11
	movs	r2, r5
	muls	r2, r3
	adds	r0, r2
	lsls	r0, #1
	movs	r2, r6
	muls	r2, r6
	adds	r0, r2
	str	r0, [sp, #(4 * ((\kc) + 6))]

	@ a0..a5 = r8 r10 r5 r6 r3 r4

	@ 2*(a0*a5 + a1*a4 + a2*a3)
	@ a5 won't be used afterwards. We pull a1 into low registers.
	mov	r0, r8
	muls	r0, r4
	mov	r4, r10
	movs	r2, r4
	muls	r2, r3
	adds	r0, r2
	movs	r2, r5
	muls	r2, r6
	adds	r0, r2
	lsls	r0, #1
	str	r0, [sp, #(4 * ((\kc) + 5))]

	@ a0..a4 = r8 r4 r5 r6 r3

	@ We pull a0 into low registers.
	mov	r2, r8

	@ a0..a4 = r2 r4 r5 r6 r3

	@ We finish the computation into registers, to mutualize the
	@ five last write operations. We need values to be in the
	@ correct order for stm.

	@ a2*a2 + 2*(a0*a4 + a1*a3) -> r7
	muls	r3, r2
	movs	r7, r6
	muls	r7, r4
	adds	r7, r3
	lsls	r7, #1
	movs	r3, r5
	muls	r3, r5
	adds	r7, r3

	@ 2*(a0*a3 + a1*a2) -> r6
	muls	r6, r2
	movs	r3, r5
	muls	r3, r4
	adds	r6, r3
	lsls	r6, #1

	@ a1*a1 + 2*a0*a2 -> r5
	muls	r5, r2
	lsls	r5, #1
	movs	r3, r4
	muls	r3, r4
	adds	r5, r3

	@ 2*a0*a1 -> r4
	muls	r4, r2
	lsls	r4, #1

	@ a0*a0 -> r2
	muls	r2, r2

	@ Write the five words into the output.
	add	r0, sp, #(4 * ((\kc) + 0))
	stm	r0!, { r2, r4, r5, r6, r7 }
.endm

@ Process two elements in gf_frob_inner.
@   r0   pointer to output buffer (updated)
@   r1   pointer to input buffer for field element (updated)
@   r2   pointer to input buffer for Frobenius constants (updated)
@   r7   constant p = 9767 (unmodified)
@ Cost: 21
.macro STEP_FROB_X2
	ldm	r1!, { r3 }
	lsrs	r4, r3, #16
	uxth	r3, r3
	ldm	r2!, { r5, r6 }
	FROBMUL  r3, r5, r7
	FROBMUL  r4, r6, r7
	lsls	r4, #16
	orrs	r3, r4
	stm	r0!, { r3 }
.endm

@ First step for computing mul_to_base; process coefficients a0..a3.
@   r0   points to the element of index 0 of operand 1
@   r1   points to the element of index 16 of operand 2
@ r0 and r1 are incremented by 8 each.
@ r8 is set to the current accumulated value.
@ a0 is copied into r10.
@ Cost: 20
.macro STEP_MUL_TO_BASE_X4_START
	ldm	r0!, { r2, r3 }
	ldm	r1!, { r5, r6 }
	uxth	r7, r2
	mov	r10, r7
	lsrs	r2, #16
	uxth	r4, r6
	muls	r4, r2
	mov	r8, r4
	uxth	r2, r3
	lsrs	r4, r5, #16
	muls	r4, r2
	add	r8, r4
	lsrs	r2, r3, #16
	uxth	r4, r5
	muls	r4, r2
	add	r8, r4
.endm

@ Next four coefficients in computing mul_to_base.
@ Cost: 23
.macro STEP_MUL_TO_BASE_X4_CONT
	ldm	r0!, { r2, r3 }
	subs	r1, #16
	ldm	r1!, { r6, r7 }
	uxth	r4, r2
	lsrs	r5, r7, #16
	muls	r4, r5
	add	r8, r4
	lsrs	r4, r2, #16
	uxth	r5, r7
	muls	r4, r5
	add	r8, r4
	uxth	r4, r3
	lsrs	r5, r6, #16
	muls	r4, r5
	add	r8, r4
	lsrs	r4, r3, #16
	uxth	r5, r6
	muls	r4, r5
	add	r8, r4
.endm

@ Last three coefficients in computing mul_to_base.
@ Final result (not yet reduced) is returned in r0.
@ Cost: 24
.macro STEP_MUL_TO_BASE_X4_END
	ldm	r0!, { r2, r3 }
	subs	r1, #16
	ldm	r1!, { r6, r7 }
	uxth	r4, r2
	lsrs	r5, r7, #16
	muls	r4, r5
	add	r8, r4
	lsrs	r4, r2, #16
	uxth	r5, r7
	muls	r4, r5
	add	r8, r4
	uxth	r2, r3
	lsrs	r5, r6, #16
	muls	r2, r5
	add	r2, r8
	lsls	r2, #1
	uxth	r6, r6
	mov	r5, r10
	muls	r5, r6
	adds	r0, r2, r5
.endm

@ Four steps of multiplication of a polynomial by an element in the
@ base field.
@ Parameters:
@   r0   multiplier (base field element)
@   r1   destination pointer (updated)
@   r2   source pointer (updated)
@   r6   modulus p
@   r7   reduction value p1i
@ Cost: 38
.macro STEP_MULCONST_MOVE_X4
	ldm	r2!, { r3, r4 }
	lsrs	r5, r3, #16
	uxth	r3, r3
	MONTYMUL  r3, r0, r6, r7
	MONTYMUL  r5, r0, r6, r7
	lsls	r5, #16
	orrs	r3, r5
	lsrs	r5, r4, #16
	uxth	r4, r4
	MONTYMUL  r4, r0, r6, r7
	MONTYMUL  r5, r0, r6, r7
	lsls	r5, #16
	orrs	r4, r5
	stm	r1!, { r3, r4 }
.endm

@ Last three steps of multiplication of a polynomial by an element in the
@ base field.
@ Parameters:
@   r0   multiplier (base field element)
@   r1   destination pointer (updated)
@   r2   source pointer (updated)
@   r6   modulus p
@   r7   reduction value p1i
@ Cost: 30
.macro STEP_MULCONST_MOVE_X4_END
	ldm	r2!, { r3, r4 }
	lsrs	r5, r3, #16
	uxth	r3, r3
	MONTYMUL  r3, r0, r6, r7
	MONTYMUL  r5, r0, r6, r7
	lsls	r5, #16
	orrs	r3, r5
	uxth	r4, r4
	MONTYMUL  r4, r0, r6, r7
	stm	r1!, { r3 }
	strh	r4, [r1]
.endm

@ Karatsuba fix up, producing output value at index i (0 to 18).
@ Expectations:
@   r0   pointer to the output buffer
@   r1   (A0*B0)[(i - 10) mod 19]  (ignored if i == 9)
@   r3   (A1*B1)[(i - 1) mod 19]  (ignored if i == 0 or 18)
@   r6   constant p = 9767
@   r7   constant p1i = 3635353193
@   ka   index for alpha = A0*B0
@   kb   index for beta = A1*B1
@   kg   index for gamma = (A0+A1)*(B0+B1)
@ On output, r1 contains (A0*B0)[i] and r3 contains (A1*B1)[(i + 9) mod 19]
@ (if i == 8 or 9, r3 is not modified).
@ Cost: depends on kc:
@   0        17
@   1..7     18
@   8        15
@   9        11
@   10..17   18
@   18       16
.macro STEP_KFIX  ka, kb, kg, i
	@ If we note the following (with indices in stack):
	@  ka   alpha = A0*B0  (19 words)
	@  ka   alpha' = alpha without the last word  (18 words)
	@  kb   beta  = A1*B1  (17 words)
	@  kg   gamma = (A0+A1)*(B0+B1)  (18 words)
	@ Then:
	@  out[i] = alpha[i] + gamma[i - 10] - alpha'[i - 10] - beta[i - 10]
	@         + 2*(beta[i - 1] + gamma[i + 9] - alpha'[i + 9] - beta[i + 9])
	@ With the following rule:
	@  xxx[i] is zero outside of the range for that array
	@
	@ Each invocation of this macro assumes that it received
	@ alpha[(i-10) mod 19] already in r1; this value will be either
	@ alpha'[i-10] (for 10 <= i <= 18) or alpha'[i+9] (for 0 <= i <= 8).
	@ Note that for i == 9, alpha'[i - 10] and alpha'[i + 9] are both
	@ zero, so r1 is ignored in that case.
	.if (\i) == 0
	ldr	r2, [sp, #(4 * ((\kg) + 9))]
	subs	r2, r1
	ldr	r3, [sp, #(4 * ((\kb) + 9))]
	subs	r2, r3
	lsls	r2, #1
	ldr	r1, [sp, #(4 * ((\ka) + 0))]
	adds	r2, r1
	.elseif (\i) <= 7
	subs	r3, r1
	ldr	r2, [sp, #(4 * ((\kg) + (\i) + 9))]
	adds	r2, r3
	ldr	r3, [sp, #(4 * ((\kb) + (\i) + 9))]
	subs	r2, r3
	lsls	r2, #1
	ldr	r1, [sp, #(4 * ((\ka) + (\i)))]
	adds	r2, r1
	.elseif (\i) == 8
	subs	r3, r1
	ldr	r2, [sp, #(4 * ((\kg) + 17))]
	adds	r2, r3
	lsls	r2, #1
	ldr	r1, [sp, #(4 * ((\ka) + 8))]
	adds	r2, r1
	.elseif (\i) == 9
	lsls	r3, #1
	ldr	r1, [sp, #(4 * ((\ka) + 9))]
	adds	r2, r1, r3
	.elseif (\i) <= 17
	lsls	r3, #1
	subs	r3, r1
	ldr	r2, [sp, #(4 * ((\kg) + (\i) - 10))]
	adds	r2, r3
	ldr	r3, [sp, #(4 * ((\kb) + (\i) - 10))]
	subs	r2, r3
	ldr	r1, [sp, #(4 * ((\ka) + (\i)))]
	adds	r2, r1
	.elseif (\i) == 18
	ldr	r2, [sp, #(4 * ((\kg) + 8))]
	subs	r2, r1
	ldr	r3, [sp, #(4 * ((\kb) + 8))]
	subs	r2, r3
	ldr	r1, [sp, #(4 * ((\ka) + 18))]
	adds	r2, r1
	.endif
	MONTYRED  r2, r6, r7
	strh	r2, [r0, #(2 * ((\ka) + (\i)))]
.endm

@ Perform Karastuba fix-up and Montgomery reduction.
@   ka   index for stack buffer for alpha = A0*B0
@   kb   index for stack buffer for beta = A1*B1
@   kg   index for stack buffer for gamma = (A0+A1)*(B0+B1)
@ Expectations:
@   r0   pointer to output buffer (unmodified)
@   r6   constant p = 9767 (unmodified)
@   r7   constant p1i = 3635353193 (unmodified)
@ Cost: 331
.macro KFIX  ka, kb, kg
	@ We compute the output words in such an order that two words
	@ read at one step are used in the next one and can thus be
	@ passed through registers r1 and r3 instead of being read again.
	ldr	r3, [sp, #(4 * ((\kb) + 8))]
	STEP_KFIX  (\ka), (\kb), (\kg),  9
	STEP_KFIX  (\ka), (\kb), (\kg),  0
	STEP_KFIX  (\ka), (\kb), (\kg), 10
	STEP_KFIX  (\ka), (\kb), (\kg),  1
	STEP_KFIX  (\ka), (\kb), (\kg), 11
	STEP_KFIX  (\ka), (\kb), (\kg),  2
	STEP_KFIX  (\ka), (\kb), (\kg), 12
	STEP_KFIX  (\ka), (\kb), (\kg),  3
	STEP_KFIX  (\ka), (\kb), (\kg), 13
	STEP_KFIX  (\ka), (\kb), (\kg),  4
	STEP_KFIX  (\ka), (\kb), (\kg), 14
	STEP_KFIX  (\ka), (\kb), (\kg),  5
	STEP_KFIX  (\ka), (\kb), (\kg), 15
	STEP_KFIX  (\ka), (\kb), (\kg),  6
	STEP_KFIX  (\ka), (\kb), (\kg), 16
	STEP_KFIX  (\ka), (\kb), (\kg),  7
	STEP_KFIX  (\ka), (\kb), (\kg), 17
	STEP_KFIX  (\ka), (\kb), (\kg),  8
	STEP_KFIX  (\ka), (\kb), (\kg), 18
.endm

@ Perform reduction after addition.
@   rx   value to reduce, receives the result
@   rt   scratch register
@   rp   register containing p = 9767 (unmodified)
@ Cost: 4
.macro MPMOD_AFTER_ADD  rx, rt, rp
	subs	\rt, \rp, \rx
	asrs	\rt, #31
	ands	\rt, \rp
	subs	\rx, \rt
.endm

@ Processing four elements of each operand in curve9767_inner_gf_add()
@   r0   pointer to output array (updated)
@   r1   pointer to first input array (updated)
@   r2   pointer to second input array (updated)
@   r7   modulus p = 9767 (unmodified)
@ Cost: 35
.macro STEP_ADD_X4
	ldm	r1!, { r3, r4 }
	ldm	r2!, { r5, r6 }
	adds	r3, r5
	adds	r4, r6
	lsrs	r6, r4, #16
	MPMOD_AFTER_ADD  r6, r5, r7
	lsls	r6, r6, #16
	uxth	r5, r4
	MPMOD_AFTER_ADD  r5, r4, r7
	orrs	r6, r5
	lsrs	r5, r3, #16
	MPMOD_AFTER_ADD  r5, r4, r7
	lsls	r5, r5, #16
	uxth	r3, r3
	MPMOD_AFTER_ADD  r3, r4, r7
	orrs	r5, r3
	stm	r0!, { r5, r6 }
.endm

@ Perform reduction after subtraction.
@   rx   value to reduce, receives the result
@   rt   scratch register
@   rp   register containing p = 9767 (unmodified)
@ Cost: 4
.macro MPMOD_AFTER_SUB  rx, rt, rp
	subs	\rt, \rx, #1
	asrs	\rt, #31
	ands	\rt, \rp
	adds	\rx, \rt
.endm

@ Processing four elements of each operand in curve9767_inner_gf_sub()
@   r0   pointer to output array (updated)
@   r1   pointer to first input array (updated)
@   r2   pointer to second input array (updated)
@   r7   modulus p = 9767 (unmodified)
@ Cost: 37
.macro STEP_SUB_X4
	ldm	r1!, { r3, r4 }
	ldm	r2!, { r5, r6 }
	subs	r3, r5
	subs	r4, r6
	sxth	r5, r4
	subs	r4, r5
	asrs	r6, r4, #16
	MPMOD_AFTER_SUB  r5, r4, r7
	MPMOD_AFTER_SUB  r6, r4, r7
	lsls	r6, #16
	orrs	r6, r5
	sxth	r4, r3
	subs	r3, r4
	asrs	r5, r3, #16
	MPMOD_AFTER_SUB  r5, r3, r7
	MPMOD_AFTER_SUB  r4, r3, r7
	lsls	r5, #16
	orrs	r5, r4
	stm	r0!, { r5, r6 }
.endm

@ Negate two elements in the same register.
@   ry   destination register
@   rx   source register (unmodified if not same as ry)
@   r6   constant 0x8001 (unmodified)
@   r7   constant 9767 + (9767 << 16) (unmodified)
@ Registers r4 and r5 are scratch.
@ Cost: 10
.macro STEP_NEG_X2  ry, rx
	subs	\ry, r7, \rx
	subs	r4, \ry, r6
	asrs	r5, r4, #31
	lsls	r5, #16
	lsls	r4, #17
	asrs	r4, #31
	uxth	r4, r4
	orrs	r4, r5
	ands	r4, r7
	adds	\ry, r4
.endm

@ Processing six elements of the operand in curve9767_inner_gf_neg()
@   r0   pointer to output array (updated)
@   r1   pointer to input array (updated)
@   r6   constant 0x8001 (unmodified)
@   r7   constant 9767 + (9767 << 16) (unmodified)
@ Cost: 26
.macro STEP_NEG_X4
	ldm	r1!, { r2, r3 }
	STEP_NEG_X2  r2, r2
	STEP_NEG_X2  r3, r3
	stm	r0!, { r2, r3 }
.endm

@ Processing two elements of the operand in curve9767_inner_gf_condneg()
@   r0    pointer to input/output array (updated)
@   r3    -1 if the negated value is kept, 0 otherwise
@   r6    constant 0x8001 (unmodified)
@   r7    constant 9767 + (9767 << 16) (unmodified)
@ Cost: 17
.macro STEP_CONDNEG_X2
	ldr	r1, [r0]
	STEP_NEG_X2  r2, r1
	bics	r1, r3
	ands	r2, r3
	orrs	r1, r2
	stm	r0!, { r1 }
.endm

@ Process four words of input (twice two words) for Frobenius+multiplication
@ (start).
@ Assumptions:
@   r1   pointer to A[0] (unchanged)
@   r2   pointer to the Frobenius coefficients (unchanged)
@   r6   constant p = 9676 (unchanged)
@   r7   pointer to B[0] (unchanged)
@   ka   index for stack buffer that receives A0+A1
@   kb   index for stack buffer that receives B0+B1
@ Cost: 43
.macro STEP_MULFROB_X4_START  ka, kb
	@ Load words A[0], A[1], and A[10], A[11].
	ldr	r0, [r1, #(2 * 0)]
	ldr	r3, [r1, #(2 * 10)]

	@ Compute and store A0+A1 (two words).
	adds	r4, r0, r3
	str	r4, [sp, #(4 * (\ka) + 2 * 0)]

	@ Apply Frobenius coefficient on A[1].
	uxth	r4, r0
	lsrs	r0, #16
	ldr	r5, [r2, #(4 * 0)]
	FROBMUL  r0, r5, r6
	lsls	r0, #16
	orrs	r0, r4
	str	r0, [r7, #(2 * 0)]

	@ Apply Frobenius coefficients on A[10] and A[11].
	uxth	r4, r3
	ldr	r5, [r2, #(4 * 9)]
	FROBMUL  r4, r5, r6
	lsrs	r3, #16
	ldr	r5, [r2, #(4 * 10)]
	FROBMUL  r3, r5, r6
	lsls	r3, #16
	orrs	r3, r4
	str	r3, [r7, #(2 * 10)]

	@ Compute the two words for B0+B1 and store them.
	adds	r0, r3
	str	r0, [sp, #(4 * (\kb))]
.endm

@ Process four words of input (twice two words) for Frobenius+multiplication
@ (continued).
@ Assumptions:
@   r1   pointer to A[0] (unchanged)
@   r2   pointer to the Frobenius coefficients (unchanged)
@   r6   constant p = 9676 (unchanged)
@   r7   pointer to B[0] (unchanged)
@   ka   index for stack buffer that receives A0+A1
@   kb   index for stack buffer that receives B0+B1
@   i    element index (even, 2 <= i <= 6)
@ Cost: 50
.macro STEP_MULFROB_X4_CONT  ka, kb, i
	@ Load words A[i], A[i+1], and A[i+10], A[i+11].
	ldr	r0, [r1, #(2 * (\i))]
	ldr	r3, [r1, #(2 * ((\i) + 10))]

	@ Compute and store A0+A1 (two words).
	adds	r4, r0, r3
	str	r4, [sp, #(4 * (\ka) + 2 * (\i))]

	@ Apply Frobenius coefficients on A[i] and A[i+1].
	uxth	r4, r0
	ldr	r5, [r2, #(4 * ((\i) - 1))]
	FROBMUL  r4, r5, r6
	lsrs	r0, #16
	ldr	r5, [r2, #(4 * ((\i) + 0))]
	FROBMUL  r0, r5, r6
	lsls	r0, #16
	orrs	r0, r4
	str	r0, [r7, #(2 * (\i))]

	@ Apply Frobenius coefficients on A[i+10] and A[i+11].
	uxth	r4, r3
	ldr	r5, [r2, #(4 * ((\i) + 9))]
	FROBMUL  r4, r5, r6
	lsrs	r3, #16
	ldr	r5, [r2, #(4 * ((\i) + 10))]
	FROBMUL  r3, r5, r6
	lsls	r3, #16
	orrs	r3, r4
	str	r3, [r7, #(2 * ((\i) + 10))]

	@ Compute the two words for B0+B1 and store them.
	adds	r0, r3
	str	r0, [sp, #(4 * (\kb) + 2 * (\i))]
.endm

@ Process three words of input (two words and one word) for
@ Frobenius+multiplication (end of sequence).
@ Assumptions:
@   r1   pointer to A[0] (unchanged)
@   r2   pointer to the Frobenius coefficients (unchanged)
@   r6   constant p = 9676 (unchanged)
@   r7   pointer to B[0] (unchanged)
@   ka   index for stack buffer that receives A0+A1
@   kb   index for stack buffer that receives B0+B1
@ Cost: 39
.macro STEP_MULFROB_X4_END  ka, kb
	@ Load words A[8], A[9], and A[18] (no A[19]).
	ldr	r0, [r1, #(2 * 8)]
	ldrh	r3, [r1, #(2 * 18)]

	@ Compute and store A0+A1 (two words).
	adds	r4, r0, r3
	str	r4, [sp, #(4 * (\ka) + 2 * 8)]

	@ Apply Frobenius coefficients on A[8] and A[9].
	uxth	r4, r0
	ldr	r5, [r2, #(4 * 7)]
	FROBMUL  r4, r5, r6
	lsrs	r0, #16
	ldr	r5, [r2, #(4 * 8)]
	FROBMUL  r0, r5, r6
	lsls	r0, #16
	orrs	r0, r4
	str	r0, [r7, #(2 * 8)]

	@ Apply Frobenius coefficients on A[18].
	ldr	r5, [r2, #(4 * (17))]
	FROBMUL  r3, r5, r6
	strh	r3, [r7, #(2 * 18)]

	@ Compute the two words for B0+B1 and store them.
	adds	r0, r3
	str	r0, [sp, #(4 * (\kb) + 2 * 8)]
.endm

@ Process two elements of x1, x2 and y1 in computation of lambda for
@ curve point addition (denominator).
@   r1   pointer to Q1
@   r2   pointer to Q2
@   r5   value -ex (-1 if x1 == x2, 0 otherwise) (unmodified)
@   r6   constant p = 9767 (unmodified)
@   r7   constant p2i = 439742 (unmodified)
@   r8   constant p + (p << 16) (unmodified)
@   kc   base stack index for output
@   i    current index (even, 0 to 16)
@ Cost: 28
.macro STEP_LAMBDA_D_X2  kc, i
	ldr	r3, [r1, #(4 + 2 * (\i))]
	ldr	r4, [r2, #(4 + 2 * (\i))]
	subs	r4, r3
	add	r4, r8
	ldr	r0, [r1, #(44 + 2 * (\i))]
	lsls	r0, #1
	ands	r0, r5
	bics	r4, r5
	orrs	r0, r4
	lsrs	r3, r0, #16
	uxth	r0, r0
	BASERED  r0, r6, r7
	BASERED  r3, r6, r7
	lsls	r3, #16
	orrs	r0, r3
	str	r0, [sp, #(4 * (\kc) + 2 * (\i))]
.endm

@ Process two elements of y1, y2 and x1^2 in computation of lambda for
@ curve point addition (numerator).
@   r1    pointer to Q1
@   r2    pointer to Q2
@   r5    value -ex (-1 if x1 == x2, 0 otherwise) (unmodified)
@   r6    constant p = 9767 (unmodified)
@   r7    constant p2i = 439742 (unmodified)
@   r8    constant p + (p << 16) (unmodified)
@   kt    base stack index x1^2
@   kc    base stack index for output
@   i     current index (even, 0 to 16)
@ Cost: 29 (32 if i == 0)
.macro STEP_LAMBDA_N_X2  kt, kc, i
	ldr	r3, [r1, #(44 + 2 * (\i))]
	ldr	r4, [r2, #(44 + 2 * (\i))]
	subs	r4, r3
	add	r4, r8
	ldr	r0, [sp, #(4 * (\kt) + 2 * (\i))]
	lsls	r3, r0, #1
	adds	r0, r3
	.if (\i) == 0
	ldr	r3, const_curve_add1_a
	adds	r0, r3
	.endif
	ands	r0, r5
	bics	r4, r5
	orrs	r0, r4
	lsrs	r3, r0, #16
	uxth	r0, r0
	BASERED  r0, r6, r7
	BASERED  r3, r6, r7
	lsls	r3, #16
	orrs	r0, r3
	str	r0, [sp, #(4 * (\kc) + 2 * (\i))]
.endm

@ Produce two coefficients of x3 (in point addition).
@   r1   pointer to Q1 (unmodified)
@   r2   pointer to Q2 (unmodified)
@   r6   constant p = 9767 (unmodified)
@   r7   constant p2i = 439742 (unmodified)
@   r8   constant 2*(p + (p << 16))
@   kl   base stack index for lambda^2
@   kc   base stack index for output x3
@   i    current index (even, 2 to 16)
@ Cost: 25
.macro STEP_SUB2_X2  kl, kc, i
	ldr	r3, [sp, #(4 * (\kl) + 2 * (\i))]
	add	r3, r8
	ldr	r4, [r1, #(4 + 2 * (\i))]
	subs	r3, r4
	ldr	r4, [r2, #(4 + 2 * (\i))]
	subs	r3, r4
	lsrs	r4, r3, #16
	uxth	r3, r3
	BASERED  r3, r6, r7
	BASERED  r4, r6, r7
	lsls	r4, #16
	orrs	r3, r4
	str	r3, [sp, #(4 * (\kc) + 2 * (\i))]
.endm

@ Assemble a value (four 16-bit elements) from three sources.
@   r0    pointer to first value and output (updated)
@   r1    pointer to second value (updated)
@   r2    pointer to third value (updated)
@   r7    -1 if first value is kept, 0 otherwise
@   r12   -1 if third value is kept, 0 otherwise
@ Cost: 27
.macro STEP_SELECT3_X4
	ldm	r0!, { r3, r4 }
	ands	r3, r7
	ands	r4, r7
	ldm	r1!, { r5, r6 }
	bics	r5, r7
	bics	r6, r7
	orrs	r3, r5
	orrs	r4, r6
	mov	r6, r12
	ldm	r2!, { r5 }
	ands	r5, r6
	bics	r3, r6
	orrs	r3, r5
	ldm	r2!, { r5 }
	ands	r5, r6
	bics	r4, r6
	orrs	r4, r5
	subs	r0, #8
	stm	r0!, { r3, r4 }
.endm

@ Assemble a value (four 16-bit elements) from three sources.
@   r0    pointer to output (updated)
@   r1    pointer to first value (updated)
@   r2    pointer to second value (updated)
@   r7    -1 if first value is kept, 0 otherwise
@   r8    -1 if second value is kept, 0 otherwise
@   r10   -1 if third value is kept, 0 otherwise
@   kt    base index of third value (stack buffer)
@   i     current index (multiple of 4, from 0 to 36)
@ Cost: 25
.macro STEP_SELECT3_STACK_X4  kt, i
	ldm	r2!, { r5, r6 }
	mov	r4, r8
	ands	r5, r4
	ands	r6, r4
	ldm	r1!, { r3, r4 }
	ands	r3, r7
	ands	r4, r7
	orrs	r5, r3
	orrs	r6, r4
	mov	r4, r10
	ldr	r3, [sp, #(4 * (\kt) + 2 * (\i) + 0)]
	ands	r3, r4
	orrs	r5, r3
	ldr	r3, [sp, #(4 * (\kt) + 2 * (\i) + 4)]
	ands	r3, r4
	orrs	r6, r3
	stm	r0!, { r5, r6 }
.endm

@ Step for gf_mul2_move_inner() (6 elements).
@   r0   output pointer (updated)
@   r1   input pointer (updated)
@   r6   constant p = 9767 (unchanged)
@   r7   constant p2i = 439742 (unchanged)
@ Cost: 53
.macro STEP_MUL2_MOVE_X6
	ldm	r1!, { r2, r3, r4 }
	lsls	r2, #1
	lsls	r3, #1
	lsls	r4, #1
	lsrs	r5, r2, #16
	uxth	r2, r2
	BASERED  r2, r6, r7
	BASERED  r5, r6, r7
	lsls	r5, #16
	orrs	r2, r5
	lsrs	r5, r3, #16
	uxth	r3, r3
	BASERED  r3, r6, r7
	BASERED  r5, r6, r7
	lsls	r5, #16
	orrs	r3, r5
	lsrs	r5, r4, #16
	uxth	r4, r4
	BASERED  r4, r6, r7
	BASERED  r5, r6, r7
	lsls	r5, #16
	orrs	r4, r5
	stm	r0!, { r2, r3, r4 }
.endm

@ Step for gf_mul4_move_inner() (6 elements).
@   r0   output pointer (updated)
@   r1   input pointer (updated)
@   r6   constant p = 9767 (unchanged)
@   r7   constant p2i = 439742 (unchanged)
@ Cost: 53
.macro STEP_MUL4_MOVE_X6
	ldm	r1!, { r2, r3, r4 }
	lsls	r2, #2
	lsls	r3, #2
	lsls	r4, #2
	lsrs	r5, r2, #16
	uxth	r2, r2
	BASERED  r2, r6, r7
	BASERED  r5, r6, r7
	lsls	r5, #16
	orrs	r2, r5
	lsrs	r5, r3, #16
	uxth	r3, r3
	BASERED  r3, r6, r7
	BASERED  r5, r6, r7
	lsls	r5, #16
	orrs	r3, r5
	lsrs	r5, r4, #16
	uxth	r4, r4
	BASERED  r4, r6, r7
	BASERED  r5, r6, r7
	lsls	r5, #16
	orrs	r4, r5
	stm	r0!, { r2, r3, r4 }
.endm

@ Step for gf_mul16_move_inner() (6 elements).
@   r0   output pointer (updated)
@   r1   input pointer (updated)
@   r6   constant p = 9767 (unchanged)
@   r7   constant p2i = 439742 (unchanged)
@ Cost: 56
.macro STEP_MUL16_MOVE_X6
	ldm	r1!, { r2, r3, r4 }
	lsrs	r5, r2, #16
	uxth	r2, r2
	lsls	r2, #4
	lsls	r5, #4
	BASERED  r2, r6, r7
	BASERED  r5, r6, r7
	lsls	r5, #16
	orrs	r2, r5
	lsrs	r5, r3, #16
	uxth	r3, r3
	lsls	r3, #4
	lsls	r5, #4
	BASERED  r3, r6, r7
	BASERED  r5, r6, r7
	lsls	r5, #16
	orrs	r3, r5
	lsrs	r5, r4, #16
	uxth	r4, r4
	lsls	r4, #4
	lsls	r5, #4
	BASERED  r4, r6, r7
	BASERED  r5, r6, r7
	lsls	r5, #16
	orrs	r4, r5
	stm	r0!, { r2, r3, r4 }
.endm

@ Step for gf_sub2_inner() (4 elements).
@   r0   input/output pointer (updated)
@   r1   input pointer (updated)
@   r6   constant p = 9767 (unchanged)
@   r7   constant p2i = 439742 (unchanged)
@   r12  constant 2pp = 2 * (9767 + (9767 << 16)) (unchanged)
@ Cost: 44
.macro STEP_SUB2_X4
	ldm	r0!, { r2, r3 }
	ldm	r1!, { r4, r5 }
	add	r2, r12
	add	r3, r12
	subs	r2, r4
	subs	r2, r4
	subs	r3, r5
	subs	r3, r5
	lsrs	r4, r2, #16
	uxth	r2, r2
	BASERED  r2, r6, r7
	BASERED  r4, r6, r7
	lsls	r4, #16
	orrs	r2, r4
	lsrs	r4, r3, #16
	uxth	r3, r3
	BASERED  r3, r6, r7
	BASERED  r4, r6, r7
	lsls	r4, #16
	orrs	r3, r4
	subs	r0, #8
	stm	r0!, { r2, r3 }
.endm

@ Step for gf_sub_sub_inner() (2 elements).
@   r0   input/output pointer (updated)
@   r1   first input pointer (updated)
@   r2   second input pointer (updated)
@   r6   constant p = 9767 (unchanged)
@   r7   constant p2i = 439742 (unchanged)
@   r12  constant 2pp = 2 * (9767 + (9767 << 16)) (unchanged)
@ Cost: 25
.macro STEP_SUB_SUB_X2
	ldr	r3, [r0]
	ldm	r1!, { r4 }
	ldm	r2!, { r5 }
	add	r3, r12
	subs	r3, r4
	subs	r3, r5
	lsrs	r4, r3, #16
	uxth	r3, r3
	BASERED  r3, r6, r7
	BASERED  r4, r6, r7
	lsls	r4, #16
	orrs	r3, r4
	stm	r0!, { r3 }
.endm

@ Step for gf_sub_sub_mul2_inner() (2 elements).
@   r0   input/output pointer (updated)
@   r1   first input pointer (updated)
@   r2   second input pointer (updated)
@   r6   constant p = 9767 (unchanged)
@   r7   constant p2i = 439742 (unchanged)
@   r12  constant 2pp = 2 * (9767 + (9767 << 16)) (unchanged)
@ Cost: 26
.macro STEP_SUB_SUB_MUL2_X2
	ldr	r3, [r0]
	ldm	r1!, { r4 }
	ldm	r2!, { r5 }
	add	r3, r12
	subs	r3, r4
	subs	r3, r5
	lsls	r3, #1
	lsrs	r4, r3, #16
	uxth	r3, r3
	BASERED  r3, r6, r7
	BASERED  r4, r6, r7
	lsls	r4, #16
	orrs	r3, r4
	stm	r0!, { r3 }
.endm

@ Step for gf_sub_mul3_move_inner() (2 elements).
@   r0   output pointer (updated)
@   r1   first input pointer (updated)
@   r2   second input pointer (updated)
@   r5   constant 3 (unchanged)
@   r6   constant p = 9767 (unchanged)
@   r7   constant p2i = 439742 (unchanged)
@   r12  constant pp = 9767 + (9767 << 16) (unchanged)
@ Cost: 23
.macro STEP_SUB_MUL3_MOVE_X2
	ldm	r1!, { r3 }
	ldm	r2!, { r4 }
	add	r3, r12
	subs	r3, r4
	muls	r3, r5
	lsrs	r4, r3, #16
	uxth	r3, r3
	BASERED  r3, r6, r7
	BASERED  r4, r6, r7
	lsls	r4, #16
	orrs	r3, r4
	stm	r0!, { r3 }
.endm

@ Step for gf_sub8_inner() (2 elements).
@   r0   input/output pointer (updated)
@   r1   input pointer (updated)
@   r6   constant p = 9767 (unchanged)
@   r7   constant p2i = 439742 (unchanged)
@   r12  constant 8p = 8 * 9767 (unchanged)
@ Cost: 28
.macro STEP_SUB8_X2
	ldr	r2, [r0]
	lsrs	r3, r2, #16
	uxth	r2, r2
	ldm	r1!, { r4 }
	lsrs	r5, r4, #16
	uxth	r4, r4
	add	r2, r12
	add	r3, r12
	lsls	r4, #3
	lsls	r5, #3
	subs	r2, r4
	subs	r3, r5
	BASERED  r2, r6, r7
	BASERED  r3, r6, r7
	lsls	r3, #16
	orrs	r2, r3
	stm	r0!, { r2 }
.endm

@ One iteration of a window lookup. This assembles two elements of a
@ coordinate of the result point.
@   r0   output pointer (updated)
@   r1   input pointer (updated)
@ The eight masks are in r5, r6, r7, r8, r10, r11, r12 and r14.
@ Cost: 36
.macro STEP_WINDOW_LOOKUP
	ldm	r1!, { r2, r3, r4 }
	ands	r2, r5
	ands	r3, r6
	ands	r4, r7
	orrs	r2, r3
	orrs	r2, r4
	ldm	r1!, { r3 }
	mov	r4, r8
	ands	r3, r4
	orrs	r2, r3
	ldm	r1!, { r3 }
	mov	r4, r10
	ands	r3, r4
	orrs	r2, r3
	ldm	r1!, { r3 }
	mov	r4, r11
	ands	r3, r4
	orrs	r2, r3
	ldm	r1!, { r3 }
	mov	r4, r12
	ands	r3, r4
	orrs	r2, r3
	ldm	r1!, { r3 }
	mov	r4, r14
	ands	r3, r4
	orrs	r2, r3
	stm	r0!, { r2 }
.endm

@ =======================================================================
@ const field_element curve9767_inner_gf_zero
@
@ The field element of value 0 (with Montgomery representation for
@ the base field).
@ =======================================================================

	.align	2
	.global	curve9767_inner_gf_zero
curve9767_inner_gf_zero:
	.long	640099879   @ 9167 + (9767 << 16)
	.long	640099879   @ 9767 + (9767 << 16)
	.long	640099879   @ 9767 + (9767 << 16)
	.long	640099879   @ 9767 + (9767 << 16)
	.long	640099879   @ 9767 + (9767 << 16)
	.long	640099879   @ 9767 + (9767 << 16)
	.long	640099879   @ 9767 + (9767 << 16)
	.long	640099879   @ 9767 + (9767 << 16)
	.long	640099879   @ 9767 + (9767 << 16)
	.long	9767        @ 9767
	.size	curve9767_inner_gf_zero, .-curve9767_inner_gf_zero

@ =======================================================================
@ const field_element curve9767_inner_gf_one
@
@ The field element of value 1 (with Montgomery representation for
@ the base field).
@ =======================================================================

	.align	2
	.global	curve9767_inner_gf_one
curve9767_inner_gf_one:
	.long	640097294   @ 7182 + (9767 << 16)
	.long	640099879   @ 9767 + (9767 << 16)
	.long	640099879   @ 9767 + (9767 << 16)
	.long	640099879   @ 9767 + (9767 << 16)
	.long	640099879   @ 9767 + (9767 << 16)
	.long	640099879   @ 9767 + (9767 << 16)
	.long	640099879   @ 9767 + (9767 << 16)
	.long	640099879   @ 9767 + (9767 << 16)
	.long	640099879   @ 9767 + (9767 << 16)
	.long	9767        @ 9767
	.size	curve9767_inner_gf_one, .-curve9767_inner_gf_one

@ =======================================================================
@ void gf_add_inner(uint16_t *c,
@                   const uint16_t *a, const uint16_t *b)
@
@ Source/destination arrays are guaranteed to be 32-bit aligned.
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function. High registers
@ are not modified.
@
@ Cost: 173
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_add_inner, %function
gf_add_inner:
	ldr	r7, const_add_p
	STEP_ADD_X4
	STEP_ADD_X4
	STEP_ADD_X4
	STEP_ADD_X4
	ldm	r1!, { r3, r4 }
	ldm	r2!, { r5, r6 }
	adds	r3, r5
	adds	r4, r6
	uxth	r6, r4
	MPMOD_AFTER_ADD  r6, r4, r7
	lsrs	r5, r3, #16
	MPMOD_AFTER_ADD  r5, r4, r7
	lsls	r5, r5, #16
	uxth	r3, r3
	MPMOD_AFTER_ADD  r3, r4, r7
	orrs	r5, r3
	stm	r0!, { r5 }
	strh	r6, [r0]
	bx	lr
	.align	2
const_add_p:
	.long	9767
	.size	gf_add_inner, .-gf_add_inner

@ =======================================================================
@ void curve9767_inner_gf_add(uint16_t *c,
@                             const uint16_t *a, const uint16_t *b)
@
@ Public wrapper for gf_add_inner() (conforms to the ABI).
@ =======================================================================

	.align	1
	.global	curve9767_inner_gf_add
	.thumb
	.thumb_func
	.type	curve9767_inner_gf_add, %function
curve9767_inner_gf_add:
	push	{ r4, r5, r6, r7, lr }
	bl	gf_add_inner
	pop	{ r4, r5, r6, r7, pc }
	.size	curve9767_inner_gf_add, .-curve9767_inner_gf_add

@ =======================================================================
@ void gf_sub_inner(uint16_t *c,
@                   const uint16_t *a, const uint16_t *b)
@
@ Source/destination arrays are guaranteed to be 32-bit aligned.
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function. High registers
@ are not modified.
@
@ Cost: 182
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_sub_inner, %function
gf_sub_inner:
	ldr	r7, const_sub_p
	STEP_SUB_X4
	STEP_SUB_X4
	STEP_SUB_X4
	STEP_SUB_X4
	ldm	r1!, { r3, r4 }
	ldm	r2!, { r5, r6 }
	subs	r3, r5
	subs	r4, r6
	sxth	r6, r4
	MPMOD_AFTER_SUB  r6, r4, r7
	sxth	r4, r3
	subs	r3, r4
	asrs	r5, r3, #16
	MPMOD_AFTER_SUB  r5, r3, r7
	MPMOD_AFTER_SUB  r4, r3, r7
	lsls	r5, #16
	orrs	r5, r4
	stm	r0!, { r5 }
	strh	r6, [r0]
	bx	lr
	.align	2
const_sub_p:
	.long	9767
	.size	gf_sub_inner, .-gf_sub_inner

@ =======================================================================
@ void curve9767_inner_gf_sub(uint16_t *c,
@                             const uint16_t *a, const uint16_t *b)
@
@ Public wrapper for gf_add_inner() (conforms to the ABI).
@ =======================================================================

	.align	1
	.global	curve9767_inner_gf_sub
	.thumb
	.thumb_func
	.type	curve9767_inner_gf_sub, %function
curve9767_inner_gf_sub:
	push	{ r4, r5, r6, r7, lr }
	bl	gf_sub_inner
	pop	{ r4, r5, r6, r7, pc }
	.size	curve9767_inner_gf_sub, .-curve9767_inner_gf_sub

@ =======================================================================
@ void curve9767_inner_gf_neg(uint16_t *c, const uint16_t *a)
@
@ Source/destination arrays are guaranteed to be 32-bit aligned.
@
@ Cost: 146
@ =======================================================================

	.align	1
	.global	curve9767_inner_gf_neg
	.thumb
	.thumb_func
	.type	curve9767_inner_gf_neg, %function
curve9767_inner_gf_neg:
	push	{ r4, r5, r6, r7 }
	ldr	r6, const_neg_nf
	ldr	r7, const_neg_pp
	STEP_NEG_X4
	STEP_NEG_X4
	STEP_NEG_X4
	STEP_NEG_X4
	STEP_NEG_X4
	pop	{ r4, r5, r6, r7 }
	bx	lr
	.align	2
const_neg_nf:
	.long	32769
const_neg_pp:
	.long	640099879
	.size	curve9767_inner_gf_neg, .-curve9767_inner_gf_neg

@ =======================================================================
@ void curve9767_inner_gf_condneg(uint16_t *c, uint32_t ctl)
@
@ Source/destination arrays are guaranteed to be 32-bit aligned.
@
@ Cost: 187
@ =======================================================================

	.align	1
	.global	curve9767_inner_gf_condneg
	.thumb
	.thumb_func
	.type	curve9767_inner_gf_condneg, %function
curve9767_inner_gf_condneg:
	push	{ r4, r5, r6, r7 }
	rsbs	r3, r1, #0
	ldr	r6, const_condneg_nf
	ldr	r7, const_condneg_pp
	STEP_CONDNEG_X2
	STEP_CONDNEG_X2
	STEP_CONDNEG_X2
	STEP_CONDNEG_X2
	STEP_CONDNEG_X2
	STEP_CONDNEG_X2
	STEP_CONDNEG_X2
	STEP_CONDNEG_X2
	STEP_CONDNEG_X2
	STEP_CONDNEG_X2
	pop	{ r4, r5, r6, r7 }
	bx	lr
	.align	2
const_condneg_nf:
	.long	32769
const_condneg_pp:
	.long	640099879
	.size	curve9767_inner_gf_condneg, .-curve9767_inner_gf_condneg

@ =======================================================================
@ void gf_mul2_move_inner(uint16_t *d, const uint16_t *a);
@
@ Compute: d <- 2*a
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function. High registers are
@ preserved.
@
@ Cost: 175
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_mul2_move_inner, %function
gf_mul2_move_inner:
	ldr	r6, const_mul2_move_p
	ldr	r7, const_mul2_move_p2i
	STEP_MUL2_MOVE_X6
	STEP_MUL2_MOVE_X6
	STEP_MUL2_MOVE_X6
	ldrh	r2, [r1]
	lsls	r2, #1
	BASERED  r2, r6, r7
	strh	r2, [r0]
	bx	lr
	.align	2
const_mul2_move_p:
	.long	9767
const_mul2_move_p2i:
	.long	439742
	.size	gf_mul2_move_inner, .-gf_mul2_move_inner

@ =======================================================================
@ void gf_mul4_move_inner(uint16_t *d, const uint16_t *a);
@
@ Compute: d <- 4*a
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function. High registers are
@ preserved.
@
@ Cost: 175
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_mul4_move_inner, %function
gf_mul4_move_inner:
	ldr	r6, const_mul4_move_p
	ldr	r7, const_mul4_move_p2i
	STEP_MUL4_MOVE_X6
	STEP_MUL4_MOVE_X6
	STEP_MUL4_MOVE_X6
	ldrh	r2, [r1]
	lsls	r2, #2
	BASERED  r2, r6, r7
	strh	r2, [r0]
	bx	lr
	.align	2
const_mul4_move_p:
	.long	9767
const_mul4_move_p2i:
	.long	439742
	.size	gf_mul4_move_inner, .-gf_mul4_move_inner

@ =======================================================================
@ void gf_mul16_move_inner(uint16_t *d, const uint16_t *a);
@
@ Compute: d <- 16*a
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function. High registers are
@ preserved.
@
@ Cost: 184
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_mul16_move_inner, %function
gf_mul16_move_inner:
	ldr	r6, const_mul16_move_p
	ldr	r7, const_mul16_move_p2i
	STEP_MUL16_MOVE_X6
	STEP_MUL16_MOVE_X6
	STEP_MUL16_MOVE_X6
	ldrh	r2, [r1]
	lsls	r2, #4
	BASERED  r2, r6, r7
	strh	r2, [r0]
	bx	lr
	.align	2
const_mul16_move_p:
	.long	9767
const_mul16_move_p2i:
	.long	439742
	.size	gf_mul16_move_inner, .-gf_mul16_move_inner

@ =======================================================================
@ void gf_sub2_inner(uint16_t *d, const uint16_t *a);
@
@ Compute: d <- d-2*a
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function. High registers are
@ preserved.
@
@ Cost: 221
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_sub2_inner, %function
gf_sub2_inner:
	ldr	r6, const_sub2_p
	ldr	r7, const_sub2_p2i
	ldr	r5, const_sub2_2pp
	mov	r12, r5
	STEP_SUB2_X4
	STEP_SUB2_X4
	STEP_SUB2_X4
	STEP_SUB2_X4
	ldm	r0!, { r2, r3 }
	ldm	r1!, { r4, r5 }
	add	r2, r12
	add	r3, r12
	subs	r2, r4
	subs	r2, r4
	subs	r3, r5
	subs	r3, r5
	lsrs	r4, r2, #16
	uxth	r2, r2
	BASERED  r2, r6, r7
	BASERED  r4, r6, r7
	lsls	r4, #16
	orrs	r2, r4
	uxth	r3, r3
	BASERED  r3, r6, r7
	subs	r0, #8
	stm	r0!, { r2, r3 }
	bx	lr
	.align	2
const_sub2_p:
	.long	9767
const_sub2_p2i:
	.long	439742
const_sub2_2pp:
	.long	1280199758
	.size	gf_sub2_inner, .-gf_sub2_inner

@ =======================================================================
@ void gf_sub_sub_inner(uint16_t *d, const uint16_t *a);
@
@ Compute: d <- d-a-b
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function. High registers are
@ preserved.
@
@ Cost: 251
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_sub_sub_inner, %function
gf_sub_sub_inner:
	ldr	r6, const_sub_sub_p
	ldr	r7, const_sub_sub_p2i
	ldr	r5, const_sub_sub_2pp
	mov	r12, r5
	STEP_SUB_SUB_X2
	STEP_SUB_SUB_X2
	STEP_SUB_SUB_X2
	STEP_SUB_SUB_X2
	STEP_SUB_SUB_X2
	STEP_SUB_SUB_X2
	STEP_SUB_SUB_X2
	STEP_SUB_SUB_X2
	STEP_SUB_SUB_X2
	ldrh	r3, [r0]
	ldrh	r4, [r1]
	ldrh	r5, [r2]
	add	r3, r12
	subs	r3, r4
	subs	r3, r5
	uxth	r3, r3
	BASERED  r3, r6, r7
	strh	r3, [r0]
	bx	lr
	.align	2
const_sub_sub_p:
	.long	9767
const_sub_sub_p2i:
	.long	439742
const_sub_sub_2pp:
	.long	1280199758
	.size	gf_sub_sub_inner, .-gf_sub_sub_inner

@ =======================================================================
@ void gf_sub_sub_mul2_inner(uint16_t *d, const uint16_t *a);
@
@ Compute: d <- 2*(d-a-b)
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function. High registers are
@ preserved.
@
@ Cost: 261
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_sub_sub_mul2_inner, %function
gf_sub_sub_mul2_inner:
	ldr	r6, const_sub_sub_mul2_p
	ldr	r7, const_sub_sub_mul2_p2i
	ldr	r5, const_sub_sub_mul2_2pp
	mov	r12, r5
	STEP_SUB_SUB_MUL2_X2
	STEP_SUB_SUB_MUL2_X2
	STEP_SUB_SUB_MUL2_X2
	STEP_SUB_SUB_MUL2_X2
	STEP_SUB_SUB_MUL2_X2
	STEP_SUB_SUB_MUL2_X2
	STEP_SUB_SUB_MUL2_X2
	STEP_SUB_SUB_MUL2_X2
	STEP_SUB_SUB_MUL2_X2
	ldrh	r3, [r0]
	ldrh	r4, [r1]
	ldrh	r5, [r2]
	add	r3, r12
	subs	r3, r4
	subs	r3, r5
	lsls	r3, #1
	uxth	r3, r3
	BASERED  r3, r6, r7
	strh	r3, [r0]
	bx	lr
	.align	2
const_sub_sub_mul2_p:
	.long	9767
const_sub_sub_mul2_p2i:
	.long	439742
const_sub_sub_mul2_2pp:
	.long	1280199758
	.size	gf_sub_sub_mul2_inner, .-gf_sub_sub_mul2_inner

@ =======================================================================
@ void gf_sub_mul3_move_inner(uint16_t *d,
@                             const uint16_t *a, const uint16_t *b);
@
@ Compute: d <- 3*(a-b)
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function. High registers are
@ preserved.
@
@ Cost: 231
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_sub_mul3_move_inner, %function
gf_sub_mul3_move_inner:
	movs	r5, #3
	ldr	r6, const_sub_mul3_move_p
	ldr	r7, const_sub_mul3_move_p2i
	ldr	r4, const_sub_mul3_move_pp
	mov	r12, r4
	STEP_SUB_MUL3_MOVE_X2
	STEP_SUB_MUL3_MOVE_X2
	STEP_SUB_MUL3_MOVE_X2
	STEP_SUB_MUL3_MOVE_X2
	STEP_SUB_MUL3_MOVE_X2
	STEP_SUB_MUL3_MOVE_X2
	STEP_SUB_MUL3_MOVE_X2
	STEP_SUB_MUL3_MOVE_X2
	STEP_SUB_MUL3_MOVE_X2
	ldrh	r3, [r1]
	ldrh	r4, [r2]
	adds	r3, r6
	subs	r3, r4
	muls	r3, r5
	BASERED  r3, r6, r7
	strh	r3, [r0]
	bx	lr
	.align	2
const_sub_mul3_move_p:
	.long	9767
const_sub_mul3_move_p2i:
	.long	439742
const_sub_mul3_move_pp:
	.long	640099879
	.size	gf_sub_mul3_move_inner, .-gf_sub_mul3_move_inner

@ =======================================================================
@ void gf_mulz9_inner(uint16_t *d, const uint16_t *a);
@
@ Compute: d <- (z^9)*a; some coefficients are set in a doubled range
@ (doubling without modular reduction).
@
@ WARNING: the source and destination arrays MUST be disjoint.
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function. High registers are
@ preserved.
@
@ Cost: 49
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_mulz9_inner, %function
gf_mulz9_inner:
	@ Source elements 0..9 go to destination elements 9..18.
	ldm	r1!, { r2, r3, r4, r5, r6 }
	strh	r2, [r0, #18]
	lsrs	r2, #16
	lsls	r7, r3, #16
	orrs	r2, r7
	lsrs	r3, #16
	lsls	r7, r4, #16
	orrs	r3, r7
	lsrs	r4, #16
	lsls	r7, r5, #16
	orrs	r4, r7
	lsrs	r5, #16
	lsls	r7, r6, #16
	orrs	r5, r7
	lsrs	r6, #16
	adds	r0, #20
	stm	r0!, { r2, r3, r4, r5, r6 }

	@ Source elements 10..18 go to destination elements 0..8, but
	@ doubled (for reduction modulo z^19-2).
	ldm	r1!, { r2, r3, r4, r5, r6 }
	lsls	r2, #1
	lsls	r3, #1
	lsls	r4, #1
	lsls	r5, #1
	lsls	r6, #1
	subs	r0, #40
	stm	r0!, { r2, r3, r4, r5 }
	strh	r6, [r0]

	bx	lr
	.size	gf_mulz9_inner, .-gf_mulz9_inner

@ =======================================================================
@ void gf_sub8_inner(uint16_t *d, const uint16_t *a);
@
@ Compute: d <- d-8*a
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function. High registers are
@ preserved.
@
@ Cost: 275
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_sub8_inner, %function
gf_sub8_inner:
	ldr	r6, const_sub8_p
	ldr	r7, const_sub8_p2i
	ldr	r5, const_sub8_8p
	mov	r12, r5
	STEP_SUB8_X2
	STEP_SUB8_X2
	STEP_SUB8_X2
	STEP_SUB8_X2
	STEP_SUB8_X2
	STEP_SUB8_X2
	STEP_SUB8_X2
	STEP_SUB8_X2
	STEP_SUB8_X2
	ldrh	r2, [r0]
	ldrh	r4, [r1]
	add	r2, r12
	lsls	r4, #3
	subs	r2, r4
	BASERED  r2, r6, r7
	strh	r2, [r0]
	bx	lr
	.align	2
const_sub8_p:
	.long	9767
const_sub8_p2i:
	.long	439742
const_sub8_8p:
	.long	78136
	.size	gf_sub8_inner, .-gf_sub8_inner

@ =======================================================================
@ void gf_mulfrob_inner(uint16_t *c,
@                       const uint16_t *a, int knum, const uint16_t *t)
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function.
@
@ This function combines a Frobenius operator application and a
@ multiplication; it computes a^(1+p^k), where k is encoded by knum
@ (set gf_get_frob_constants()). Array t[] is a temporary array which
@ is modified by this function.
@
@ CAUTION: THIS FUNCTION JUMPS DIRECTLY INTO THE NEXT ONE (gf_mul_inner()).
@ This is a code size optimization to avoid duplication of the bulk of
@ the multiplication routine.
@
@ Source/destination arrays are guaranteed to be 32-bit aligned.
@
@ Cost: 1756
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_mulfrob_inner, %function
gf_mulfrob_inner:
	push	{ lr }

	@ CAUTION: DO NOT CHANGE STACK SIZE AND LAYOUT.
	@ This function is an alternate entry point for gf_mul_inner();
	@ it must use the same stack size and layout.

	sub	sp, #284
	str	r0, [sp, #264]   @ Output buffer address.
	str	r1, [sp, #268]   @ First operand address.

	@ Get Frobenius coefficients and load reduction constants.
	movs	r7, r3
	bl	gf_get_frob_constants
	ldr	r6, const_mulfrob_p

	@ Apply Frobenius operator and save it as second operand (B). Also,
	@ compute A0+A1 and B0+B1 at the same time.
	STEP_MULFROB_X4_START  37, 42
	STEP_MULFROB_X4_CONT   37, 42, 2
	STEP_MULFROB_X4_CONT   37, 42, 4
	STEP_MULFROB_X4_CONT   37, 42, 6
	STEP_MULFROB_X4_END    37, 42

	@ Finish multiplication by jumping into gf_mul_inner().
	mov	r1, r7
	b	gf_mul_inner_alternate_entry
	.align	2
const_mulfrob_p:
	.long	9767
	.size	gf_mulfrob_inner, .-gf_mulfrob_inner

@ =======================================================================
@ void gf_mul_inner(uint16_t *c, const uint16_t *a, const uint16_t *b)
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function.
@
@ Source/destination arrays are guaranteed to be 32-bit aligned.
@
@ Cost: 1574
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_mul_inner, %function
gf_mul_inner:
	push	{ lr }

	@ We split A and B into low and high halves, for Karatsuba
	@ multiplication:
	@   A = A0 + A1*X^10
	@   B = B0 + B1*X^10

	@ Stack elements (indices for 4-byte words):
	@   0..36     A*B
	@   37..41    A0+A1  (16-bit words)
	@   42..46    B0+B1  (16-bit words)
	@   47..65    (A0+A1)*(B0+B1)
	@   66        pointer to output
	@   67        pointer to first operand
	@   68        pointer to second operand
	@   69        tmp1
	@
	@ CAUTION: DO NOT CHANGE STACK SIZE AND LAYOUT.
	@ The gf_mulfrob_inner() is an alternate entry point for this
	@ function and relies on the stack layout.

	sub	sp, #284
	str	r0, [sp, #264]   @ Output buffer address.
	str	r1, [sp, #268]   @ First operand address.
	str	r2, [sp, #272]   @ Second operand address.

	@ Compute and store A0+A1.
	add	r0, sp, #(4 * 37)
	ldm	r1!, { r2, r3, r4 }
	adds	r1, #8
	ldm	r1!, { r5, r6, r7 }
	adds	r2, r5
	adds	r3, r6
	adds	r4, r7
	stm	r0!, { r2, r3, r4 }
	ldm	r1!, { r5, r6 }
	uxth	r6, r6
	subs	r1, #28
	ldm	r1!, { r2, r3 }
	adds	r2, r5
	adds	r3, r6
	stm	r0!, { r2, r3 }

	@ Compute and store B0+B1.
	add	r0, sp, #(4 * 42)
	ldr	r1, [sp, #272]
	ldm	r1!, { r2, r3, r4 }
	adds	r1, #8
	ldm	r1!, { r5, r6, r7 }
	adds	r2, r5
	adds	r3, r6
	adds	r4, r7
	stm	r0!, { r2, r3, r4 }
	ldm	r1!, { r5, r6 }
	uxth	r6, r6
	subs	r1, #28
	ldm	r1!, { r2, r3 }
	adds	r2, r5
	adds	r3, r6
	stm	r0!, { r2, r3 }

	subs	r1, #20

	@ Second entry point: execution jumps here from gf_mulfrob_inner().
	@ Assumptions:
	@   [sp, #264]   Output buffer address
	@   [sp, #268]   First operand address
	@   r1           Second operand address
	@   A0+B0 and A1+B1 are already computed and stored in the stack
gf_mul_inner_alternate_entry:

	@ Compute A0*B0, into stack at index 0
	ldr	r0, [sp, #268]
	MUL_SET_10x10  0, 69, 0

	@ Compute A1*B1, into stack at index 20
	ldr	r0, [sp, #268]
	adds	r0, #20
	adds	r1, #20
	MUL_SET_9x9  20, 69

	@ Compute (A0+A1)*(B0+B1), into stack at index 47
	@ (truncated to 18 words, the last word won't be used)
	add	r0, sp, #148
	add	r1, sp, #168
	MUL_SET_10x10  47, 69, 1

	@ Karatsuba fix-up and reductions.
	ldr	r0, [sp, #264]
	ldr	r6, const_mul_p
	ldr	r7, const_mul_p1i
	KFIX  0, 20, 47

	add	sp, #284
	pop	{ pc }

	.align	2
const_mul_p:
	.long	9767
const_mul_p1i:
	.long	3635353193

	.size	gf_mul_inner, .-gf_mul_inner

@ =======================================================================
@ void curve9767_inner_gf_mul(uint16_t *c,
@                             const uint16_t *a, const uint16_t *b)
@
@ This is a public wrapper over gf_mul_inner(); it saves registers
@ as per the ABI conventions.
@ =======================================================================

	.align	1
	.global	curve9767_inner_gf_mul
	.thumb
	.thumb_func
	.type	curve9767_inner_gf_mul, %function
curve9767_inner_gf_mul:
	push	{ r4, r5, r6, r7, lr }
	mov	r4, r8
	mov	r5, r10
	mov	r6, r11
	push	{ r4, r5, r6 }

	bl	gf_mul_inner

	pop	{ r4, r5, r6 }
	mov	r8, r4
	mov	r10, r5
	mov	r11, r6
	pop	{ r4, r5, r6, r7, pc }
	.size	curve9767_inner_gf_mul, .-curve9767_inner_gf_mul

@ =======================================================================
@ void gf_sqr_inner(uint16_t *c, const uint16_t *a)
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function.
@
@ Source/destination arrays are guaranteed to be 32-bit aligned.
@
@ Cost: 994
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_sqr_inner, %function
gf_sqr_inner:
	push	{ lr }

	@ A = A0 + A1*X^10

	@ Stack elements (indices for 4-byte words):
	@   0..36    A*A
	@   37..41   A0+A1
	@   42..60   (A0+A1)*(A0+A1)
	@   61       pointer to output

	sub	sp, #252
	str	r0, [sp, #244]   @ Output buffer address.

	@ Compute A0*A0, into stack buffer at index 0
	SQR_SET_10x10  0, 0

	@ Compute A1*A1, into stack buffer at index 20
	SQR_SET_9x9  20

	@ Compute A0+A1 at index 37
	subs	r1, #20
	movs	r0, r1
	subs	r0, #20
	ldm	r0!, { r2, r3, r4 }
	ldm	r1!, { r5, r6, r7 }
	adds	r2, r5
	adds	r3, r6
	adds	r4, r7
	ldm	r0!, { r5, r6 }
	ldm	r1, { r0, r1 }
	uxth	r1, r1
	adds	r5, r0
	adds	r6, r1
	add	r0, sp, #(4 * 37)
	stm	r0!, { r2, r3, r4, r5, r6 }

	@ Compute (A0+A1)*(A0+A1) at index 42
	add	r1, sp, #(4 * 37)
	SQR_SET_10x10  42, 1

	@ Fix up and reductions.
	ldr	r0, [sp, #244]
	ldr	r6, const_sqr_p
	ldr	r7, const_sqr_p1i
	KFIX  0, 20, 42

	add	sp, #252
	pop	{ pc }

	.align	2
const_sqr_p:
	.long	9767
const_sqr_p1i:
	.long	3635353193

	.size	gf_sqr_inner, .-gf_sqr_inner

@ =======================================================================
@ void curve9767_inner_gf_sqr(uint16_t *c, const uint16_t *a)
@
@ This is a public wrapper over gf_sqr_inner(); it saves registers
@ as per the ABI conventions.
@ =======================================================================

	.align	1
	.global	curve9767_inner_gf_sqr
	.thumb
	.thumb_func
	.type	curve9767_inner_gf_sqr, %function
curve9767_inner_gf_sqr:
	push	{ r4, r5, r6, r7, lr }
	mov	r4, r8
	mov	r5, r10
	mov	r6, r11
	push	{ r4, r5, r6 }

	bl	gf_sqr_inner

	pop	{ r4, r5, r6 }
	mov	r8, r4
	mov	r10, r5
	mov	r11, r6
	pop	{ r4, r5, r6, r7, pc }
	.size	curve9767_inner_gf_sqr, .-curve9767_inner_gf_sqr

@ =======================================================================
@ uint32_t *gf_get_frob_constants(int knum)
@
@ This function does conform to the ABI: it expects its parameter in r2,
@ and writes the return value in r2. Register r3 is used as scratch
@ register.
@
@ This function returns the Frobenius coefficients for the operator that
@ raises an input element to the power p^k. Coefficients already take into
@ account Montgomery representation and include the multiplication by p1i.
@ Returned array contains 18 elements (the Frobenius coefficient for
@ degree 0 is always 1).
@
@ Parameter knum (in r2) specifies k with the following correspondance:
@   k   knum
@   1    0
@   2    1
@   4    2
@   8    3
@   9    4
@
@ Cost: 6
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_get_frob_constants, %function
gf_get_frob_constants:
	@ Each array is 18 words = 72 bytes
	movs	r3, #72
	muls	r2, r3
	adr	r3, const_frob1_cc
	adds	r2, r3
	bx	lr
	.align	2
const_frob1_cc:
	.long	1114308091
	.long	1863189969
	.long	1154324680
	.long	186011177
	.long	3200887370
	.long	2150341975
	.long	2948035297
	.long	1373316562
	.long	1060659477
	.long	3356556298
	.long	1478415076
	.long	1092320954
	.long	1982360250
	.long	2484106711
	.long	2599319308
	.long	2490263110
	.long	1019763403
	.long	2805118908
const_frob2_cc:
	.long	1863189969
	.long	186011177
	.long	2150341975
	.long	1373316562
	.long	3356556298
	.long	1092320954
	.long	2484106711
	.long	2490263110
	.long	2805118908
	.long	1114308091
	.long	1154324680
	.long	3200887370
	.long	2948035297
	.long	1060659477
	.long	1478415076
	.long	1982360250
	.long	2599319308
	.long	1019763403
const_frob4_cc:
	.long	186011177
	.long	1373316562
	.long	1092320954
	.long	2490263110
	.long	1114308091
	.long	3200887370
	.long	1060659477
	.long	1982360250
	.long	1019763403
	.long	1863189969
	.long	2150341975
	.long	3356556298
	.long	2484106711
	.long	2805118908
	.long	1154324680
	.long	2948035297
	.long	1478415076
	.long	2599319308
const_frob8_cc:
	.long	1373316562
	.long	2490263110
	.long	3200887370
	.long	1982360250
	.long	1863189969
	.long	3356556298
	.long	2805118908
	.long	2948035297
	.long	2599319308
	.long	186011177
	.long	1092320954
	.long	1114308091
	.long	1060659477
	.long	1019763403
	.long	2150341975
	.long	2484106711
	.long	1154324680
	.long	1478415076
const_frob9_cc:
	.long	1060659477
	.long	2805118908
	.long	1373316562
	.long	1019763403
	.long	2948035297
	.long	2490263110
	.long	2150341975
	.long	2599319308
	.long	3200887370
	.long	2484106711
	.long	186011177
	.long	1982360250
	.long	1154324680
	.long	1092320954
	.long	1863189969
	.long	1478415076
	.long	1114308091
	.long	3356556298
	.size	gf_get_frob_constants, .-gf_get_frob_constants

@ =======================================================================
@ void gf_frob_inner(uint16_t *c, const uint16_t *a, int knum)
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function.
@   r0   incremented by 36
@   r1   incremented by 36
@ r2 to r7 are consumed. High registers are unmodified.
@
@ Compute the Frobenius operator on a[], i.e. raising it to the power
@ p^k. Since p is the characteristic of the field and the reduction
@ polynomial is X^19-2, it suffices to multiply each coefficient by
@ a constant. The constant for coefficient i is 2^((p-1)/19)*k*i (in
@ particular, the constant for coefficient 0 is 1). Parameter knum
@ encodes the value of k (see gf_get_frob_constants()).
@
@ Source/destination arrays are guaranteed to be 32-bit aligned.
@
@ Cost: 211
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_frob_inner, %function
gf_frob_inner:
	push	{ lr }
	bl	gf_get_frob_constants
	ldr	r7, const_frob_p

	@ In the first word, constant for degree 0 is 1, so only the
	@ high half must be modified.
	ldm	r1!, { r3 }
	lsrs	r4, r3, #16
	uxth	r3, r3
	ldm	r2!, { r6 }
	FROBMUL  r4, r6, r7
	lsls	r4, #16
	orrs	r3, r4
	stm	r0!, { r3 }
	STEP_FROB_X2
	STEP_FROB_X2
	STEP_FROB_X2
	STEP_FROB_X2
	STEP_FROB_X2
	STEP_FROB_X2
	STEP_FROB_X2
	STEP_FROB_X2
	ldrh	r3, [r1]
	ldm	r2!, { r5 }
	FROBMUL  r3, r5, r7
	strh	r3, [r0]
	pop	{ pc }
	.align	2
const_frob_p:
	.long	9767
	.size	gf_frob_inner, .-gf_frob_inner

@ =======================================================================
@ uint32_t gf_mul_to_base_inner(const uint16_t *a, const uint16_t *b)
@
@ Multiply two elements, assuming that the product fits in GF(p); i.e.,
@ compute the coefficient of index 0 of the product of a by b.
@
@ This function does not conform to the ABI; instead, it uses the
@ following conventions:
@   Input:
@     r0   pointer to a
@     r1   pointer to element of index 16 in b
@   Output:
@     r0   result (in GF(p))
@     r6   value p = 9767
@     r7   value p1i = 3635353193
@ Registers r11 and r12 are preserved, other registers are not.
@
@ Cost: 124
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_mul_to_base_inner, %function
gf_mul_to_base_inner:
	STEP_MUL_TO_BASE_X4_START
	STEP_MUL_TO_BASE_X4_CONT
	STEP_MUL_TO_BASE_X4_CONT
	STEP_MUL_TO_BASE_X4_CONT
	STEP_MUL_TO_BASE_X4_END
	ldr	r6, const_mul_to_base_p
	ldr	r7, const_mul_to_base_p1i
	MONTYRED  r0, r6, r7
	bx	lr
	.align	2
const_mul_to_base_p:
	.long	9767
const_mul_to_base_p1i:
	.long	3635353193
	.size	gf_mul_to_base_inner, .-gf_mul_to_base_inner

@ =======================================================================
@ uint32_t mp_inv_inner(const uint32_t x)
@
@ Given x in GF(p), compute 1/x. If x is zero, then zero is returned.
@
@ This function does not conform to the ABI; instead, it uses the
@ following conventions:
@   Input:
@     r0   input value x
@     r6   constant value p = 9767 (unmodified)
@     r7   constant value p1i = 3635353193 (unmodified)
@   Output:
@     r0   result (in GF(p))
@ Registers r1..r3 are not preserved; however, r4-r7 and high registers
@ are preserved.
@
@ Cost: 107
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	mp_inv_inner, %function
mp_inv_inner:
	@ The source value is in r0; we raise it to power p-2 = 9765.

	@ Set r1 = x^8
	movs	r1, r0
	MONTYMUL  r1, r1, r6, r7
	MONTYMUL  r1, r1, r6, r7
	MONTYMUL  r1, r1, r6, r7
	@ Set r2 = x^9
	movs	r2, r1
	MONTYMUL  r2, r0, r6, r7
	@ Set r3 = x^(9*16+8)
	movs	r3, r2
	MONTYMUL  r3, r3, r6, r7
	MONTYMUL  r3, r3, r6, r7
	MONTYMUL  r3, r3, r6, r7
	MONTYMUL  r3, r3, r6, r7
	MONTYMUL  r3, r1, r6, r7
	@ Set r3 = x^((9*16+8)*16+9)
	MONTYMUL  r3, r3, r6, r7
	MONTYMUL  r3, r3, r6, r7
	MONTYMUL  r3, r3, r6, r7
	MONTYMUL  r3, r3, r6, r7
	MONTYMUL  r3, r2, r6, r7
	@ Set r0 = x^(((9*16+8)*16+9)*4+1)
	MONTYMUL  r3, r3, r6, r7
	MONTYMUL  r3, r3, r6, r7
	MONTYMUL  r0, r3, r6, r7

	bx	lr
	.size	mp_inv_inner, .-mp_inv_inner

@ =======================================================================
@ void gf_mulconst_move_inner(uint16_t *d, const uint16_t *a, uint32_t c)
@
@ Multiply field element a by constant c (in GF(p)), result in d. Source
@ and destination may be the same array.
@
@ This function does not conform to the ABI; instead, it uses the
@ following conventions:
@   Input:
@     r0   input constant x
@     r1   destination pointer d
@     r2   source pointer a
@     r6   constant value p = 9767 (unmodified)
@     r7   constant value p1i = 3635353193 (unmodified)
@ Registers r0, r6 and r7 are preserved, as well as high registers.
@ Registers r1-r5 are NOT preserved.
@
@ Cost: 184
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_mulconst_move_inner, %function
gf_mulconst_move_inner:
	STEP_MULCONST_MOVE_X4
	STEP_MULCONST_MOVE_X4
	STEP_MULCONST_MOVE_X4
	STEP_MULCONST_MOVE_X4
	STEP_MULCONST_MOVE_X4_END
	bx	lr
	.size	gf_mulconst_move_inner, .-gf_mulconst_move_inner

@ =======================================================================
@ void gf_inv_inner(uint16_t *c, const uint16_t *a)
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function.
@
@ This computes the inverse of 'a' into 'c'. This assumes that 'a' is
@ in Montgomery representation, and returns the value in Montgomery
@ representation as well.
@
@ Cost: 9508
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_inv_inner, %function
gf_inv_inner:
	push	{ lr }

	@ Stack layout (indices in 32-bit words)
	@   0    t1
	@   10   t2
	@   20   saved output buffer pointer
	@   21   saved input buffer pointer

	sub	sp, #92
	str	r0, [sp, #80]   @ Output buffer pointer
	str	r1, [sp, #84]   @ Input buffer pointer

	@ Let r = (p^19-1)/(p-1) = 1 + p + p^2 + p^3 + ... + p^18.
	@ The inverse of 'a' is: 1/a = a^(r-1) * a^(-r).
	@ It so happens that a^r is necessarily in the base field
	@ (it is a solution to x^(p-1) = 1), so its inverse is
	@ relatively inexpensive to compute. Moreover, a^(r-1) can
	@ be computed with only a few multiplications, thanks to
	@ the efficiency of the Frobenius operator, i.e. raising
	@ a value to the power p.

	@ Compute a^(1+p)
	mov	r0, sp
	movs	r2, #0
	add	r3, sp, #40
	bl	gf_mulfrob_inner

	@ Compute a^(1+p+p^2+p^3)
	mov	r0, sp
	mov	r1, sp
	movs	r2, #1
	add	r3, sp, #40
	bl	gf_mulfrob_inner

	@ Compute a^(1+p+p^2+p^3+p^4+p^5+p^6+p^7)
	mov	r0, sp
	mov	r1, sp
	movs	r2, #2
	add	r3, sp, #40
	bl	gf_mulfrob_inner

	@ Compute a^(1+p+p^2+p^3+p^4+p^5+p^6+p^7+p^8)
	mov	r0, sp
	mov	r1, sp
	movs	r2, #0
	bl	gf_frob_inner
	mov	r0, sp
	mov	r1, sp
	ldr	r2, [sp, #84]
	bl	gf_mul_inner

	@ Compute a^(1+p+p^2+p^3+...+p^17)
	mov	r0, sp
	mov	r1, sp
	movs	r2, #4
	add	r3, sp, #40
	bl	gf_mulfrob_inner

	@ Compute a^(p+p^2+p^3+...+p^17+p^18) = a^(r-1)
	mov	r0, sp
	mov	r1, sp
	movs	r2, #0
	bl	gf_frob_inner

	@ Compute a^r. Since we know that this value is in GF(p), we can
	@ simply compute the low coefficient of the product a*a^(r-1).
	ldr	r0, [sp, #84]
	add	r1, sp, #32
	bl	gf_mul_to_base_inner

	@ Compute a^(-r). a^r is in r0, and gf_mul_to_base_inner() already
	@ set r6 and r7 to p and p1i, respectively.
	bl	mp_inv_inner

	@ Compute 1/a = a^(r-1)*a^(-r). Multiplier (a^(-r)) is in r0, and
	@ r6 and r7 still contain p and p1i.
	mov	r2, sp
	ldr	r1, [sp, #80]
	bl	gf_mulconst_move_inner

	add	sp, #92
	pop	{ pc }
	.size	gf_inv_inner, .-gf_inv_inner

@ =======================================================================
@ void curve9767_inner_gf_inv(uint32_t *c, const uint32_t *a)
@
@ This is a public wrapper over gf_inv_inner(), to be used for tests.
@ =======================================================================

	.align	1
	.global	curve9767_inner_gf_inv
	.thumb
	.thumb_func
	.type	curve9767_inner_gf_inv, %function
curve9767_inner_gf_inv:
	push	{ r4, r5, r6, r7, lr }
	mov	r4, r8
	mov	r5, r10
	mov	r6, r11
	push	{ r4, r5, r6 }

	bl	gf_inv_inner

	pop	{ r4, r5, r6 }
	mov	r8, r4
	mov	r10, r5
	mov	r11, r6
	pop	{ r4, r5, r6, r7, pc }
	.size	curve9767_inner_gf_inv, .-curve9767_inner_gf_inv

@ =======================================================================
@ uint32_t gf_sqrt_inner(uint32_t *c, const uint32_t *a)
@
@ This computes a square root of 'a' into 'c'. This assumes that 'a' is
@ in Montgomery representation, and returns the value in Montgomery
@ representation as well. If 'a' is not a square, then '-a' is a square
@ and this function writes a square root of '-a' into 'c'.
@
@ Returned value is 1 if the input 'a' is a square (including if 'a' is
@ zero), 0 otherwise.
@
@ If c == NULL, then the square root is not computed and the function is
@ faster. It still returns the correct quadratic residue status of 'a'.
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function.
@
@ Cost: 26962 (9341 if fast exit)
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_sqrt_inner, %function
gf_sqrt_inner:
	push	{ lr }
	sub	sp, #132

	str	r0, [sp, #120]   @ Output buffer pointer
	str	r1, [sp, #124]   @ Input buffer pointer

	@ Since p^19 = 9767^19 = 3 mod 4, a square root is obtained by
	@ raising 'a' to the power (p^19+1)/4. We can write that value
	@ as:
	@   (p^19+1)/4 = ((p+1)/4)*(2*e-r)
	@ with:
	@   d =               1 + p^2 + p^4 + ... + p^14 + p^16
	@   e = 1 + (p^2)*d = 1 + p^2 + p^4 + ... + p^14 + p^16 + p^18
	@   f = p*d         = p + p^3 + p^5 + ... + p^15 + p^17
	@   r = e + f       = 1 + p + p^2 + p^3 + ... + p^17 + p^18
	@ Therefore:
	@   sqrt(a) = (((a^e)^2)/(a^r))^((p+1)/4)
	@ Moreover, (p-1)*r = p^19-1; therefore, if a != 0, then a^r is a
	@ (p-1)-th root of unity, which implies that it is part of GF(p)
	@ (the base field), thus easy to invert. If a == 0, the inversion
	@ routine yields 0, which is still correct in that case.
	@
	@ We can thus compute:
	@   u0 = a^d  (with a few Frobenius + mul invocations)
	@   u1 = a*(u0^(p^2)) = a^e  (one more Frobenius + mul)
	@   u2 = u0^p = a^f  (one Frobenius application)
	@   u3 = u1^2 = a^(2*e)
	@   u4 = u1*u2 = a^r  (in GF(p))
	@   u5 = u3/u4 = a^(2*e-r)
	@   c = u5^((p+1)/4)

	@ We allocate three slots for field elements on the stack
	@ (indices counted in 32-bit words):
	@     0   t1
	@    10   t2
	@    20   t3
	@ At offsets 120 and 124, we save the output and input buffer
	@ pointers. At offset 128, there is a free slot which is used
	@ to hold a^(-r).

	@ a^(1+p^2) -> t1
	mov	r0, sp
	movs	r2, #1
	add	r3, sp, #40
	bl	gf_mulfrob_inner

	@ a^(1+p^2+p^4+p^6) -> t1
	mov	r0, sp
	mov	r1, sp
	movs	r2, #2
	add	r3, sp, #40
	bl	gf_mulfrob_inner

	@ a^(1+p^2+p^4+p^6+p^8+p^10+p^12+p^14) -> t1
	mov	r0, sp
	mov	r1, sp
	movs	r2, #3
	add	r3, sp, #40
	bl	gf_mulfrob_inner

	@ a^d = a^(1+p^2+p^4+p^6+p^8+p^10+p^12+p^14+p^16) -> t1
	add	r0, sp, #40
	mov	r1, sp
	movs	r2, #1
	bl	gf_frob_inner
	mov	r0, sp
	ldr	r1, [sp, #124]
	add	r2, sp, #40
	bl	gf_mul_inner

	@ a^f = (a^d)^p -> t2
	add	r0, sp, #40
	mov	r1, sp
	movs	r2, #0
	bl	gf_frob_inner

	@ a^e = a*((a^f)^p) -> t1
	mov	r0, sp
	add	r1, sp, #40
	movs	r2, #0
	bl	gf_frob_inner
	mov	r0, sp
	mov	r1, sp
	ldr	r2, [sp, #124]
	bl	gf_mul_inner

	@ Compute a^r = a^e*a^f. It is an element in the base field.
	mov	r0, sp
	add	r1, sp, #(40 + 32)
	bl	gf_mul_to_base_inner

	@ Save a^r into register r3, and compute (a^r)^((p-1)/2) into
	@ register r0. gf_mul_to_base_inner() already sets r6 and r7 to
	@ p and p1i, respectively.

	@ (a^r)^9 = ((a^r)^8)*(a^r)
	movs	r3, r0
	MONTYMUL  r0, r0, r6, r7
	MONTYMUL  r0, r0, r6, r7
	MONTYMUL  r0, r0, r6, r7
	MONTYMUL  r0, r3, r6, r7
	@ (a^r)^19 = (((a^r)^9)^2)*(a^r)
	MONTYMUL  r0, r0, r6, r7
	MONTYMUL  r0, r3, r6, r7
	movs	r1, r0
	@ (a^r)^4883 = (((a^r)^19)^256)*((a^r)^19)
	MONTYMUL  r0, r0, r6, r7
	MONTYMUL  r0, r0, r6, r7
	MONTYMUL  r0, r0, r6, r7
	MONTYMUL  r0, r0, r6, r7
	MONTYMUL  r0, r0, r6, r7
	MONTYMUL  r0, r0, r6, r7
	MONTYMUL  r0, r0, r6, r7
	MONTYMUL  r0, r0, r6, r7
	MONTYMUL  r0, r1, r6, r7
	@ Value (a^r)^((p-1)/2) can be only one of these values:
	@   9767   (Montgomery representation of 0; a is zero)
	@   7182   (Montgomery representation of 1; a is a non-zero QR)
	@   2585   (Montgomery representation of -1; a is not a QR)
	@ We want to return 1 in the first two cases, 0 otherwise.
	@ It so happens that bit 1 of 9767 and 7182 is 1, but is 0 for 2585.
	@ We store the value to return in a stack slot (overwriting the
	@ pointer to the source, we won't need it anymore).
	lsls	r0, #30
	lsrs	r0, #31

	@ If the output pointer is NULL, then we simply return r0.
	@ Otherwise, we save r0 into a stack slot, and proceed with
	@ the remaining of the square root computation.
	ldr	r2, [sp, #120]
	cmp	r2, #0
	beq	sqrt_exit
	str	r0, [sp, #124]

	@ Invert a^r into a^(-r), and save that value into a stack slot.
	@ Value a^r is in r3 at this point, and r6 and r7 contain p and
	@ p1i, respectively.
	movs	r0, r3
	bl	mp_inv_inner
	str	r0, [sp, #128]

	@ (a^e)^2 -> t1
	mov	r0, sp
	mov	r1, sp
	bl	gf_sqr_inner

	@ (a^e)^2*a^(-r) -> t2
	ldr	r6, const_sqrt_p
	ldr	r7, const_sqrt_p1i
	ldr	r0, [sp, #128]
	add	r1, sp, #40
	mov	r2, sp
	bl	gf_mulconst_move_inner

	@ Now all we have to do is to raise the value in t2 to power
	@ (p+1)/4 = 2442. In the comments below, we call 'x' the
	@ original value; it is in t2.

	@ t2^4 = x^4 -> t1
	mov	r0, sp
	add	r1, sp, #40
	bl	gf_sqr_inner
	mov	r0, sp
	mov	r1, sp
	bl	gf_sqr_inner

	@ t1*t2 = x^5 -> t3
	add	r0, sp, #80
	mov	r1, sp
	add	r2, sp, #40
	bl	gf_mul_inner

	@ t1*t3 = x^9 -> t1
	mov	r0, sp
	mov	r1, sp
	add	r2, sp, #80
	bl	gf_mul_inner

	@ t1^2 = x^18 -> t1
	mov	r0, sp
	mov	r1, sp
	bl	gf_sqr_inner

	@ t1*t2 = x^19 -> t1
	mov	r0, sp
	mov	r1, sp
	add	r2, sp, #40
	bl	gf_mul_inner

	@ t1^64 = x^1216 -> t1
	mov	r0, sp
	mov	r1, sp
	bl	gf_sqr_inner
	mov	r0, sp
	mov	r1, sp
	bl	gf_sqr_inner
	mov	r0, sp
	mov	r1, sp
	bl	gf_sqr_inner
	mov	r0, sp
	mov	r1, sp
	bl	gf_sqr_inner
	mov	r0, sp
	mov	r1, sp
	bl	gf_sqr_inner
	mov	r0, sp
	mov	r1, sp
	bl	gf_sqr_inner

	@ t1*t3 = x^1221 -> t1
	mov	r0, sp
	mov	r1, sp
	add	r2, sp, #80
	bl	gf_mul_inner

	@ t1^2 = x^2442 -> out
	ldr	r0, [sp, #120]
	mov	r1, sp
	bl	gf_sqr_inner

	@ Reload return value from the stack slot where it was saved.
	ldr	r0, [sp, #124]
sqrt_exit:
	add	sp, #132
	pop	{ pc }

	.align	2
const_sqrt_p:
	.long	9767
const_sqrt_p1i:
	.long	3635353193
	.size	gf_sqrt_inner, .-gf_sqrt_inner

@ =======================================================================
@ void curve9767_inner_gf_sqrt(uint16_t *c, const uint16_t *a)
@
@ This is a public wrapper over gf_sqrt_inner(), to be used for tests.
@ =======================================================================

	.align	1
	.global	curve9767_inner_gf_sqrt
	.thumb
	.thumb_func
	.type	curve9767_inner_gf_sqrt, %function
curve9767_inner_gf_sqrt:
	push	{ r4, r5, r6, r7, lr }
	mov	r4, r8
	mov	r5, r10
	mov	r6, r11
	push	{ r4, r5, r6 }

	bl	gf_sqrt_inner

	pop	{ r4, r5, r6 }
	mov	r8, r4
	mov	r10, r5
	mov	r11, r6
	pop	{ r4, r5, r6, r7, pc }
	.size	curve9767_inner_gf_sqrt, .-curve9767_inner_gf_sqrt

@ =======================================================================
@ void gf_cubert_inner(uint32_t *c, const uint32_t *a)
@
@ This computes the (unique) cube root of 'a' into 'c'.
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function.
@
@ Cost: 31163
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_cubert_inner, %function
gf_cubert_inner:
	push	{ lr }

	@ Since p^19 = 9767^19 = 2 mod 3, there is a unique cube root
	@ that is obtained by raising 'a' to the power (2*p^19-1)/3.
	@ We can write that exponent as:
	@   (2*p^19-1)/3 = ((2*p-1)/3)*e + ((p-2)/3)*f
	@ with:
	@   d =               1 + p^2 + p^4 + ... + p^14 + p^16
	@   e = 1 + (p^2)*d = 1 + p^2 + p^4 + ... + p^14 + p^16 + p^18
	@   f = p*d         = p + p^3 + p^5 + ... + p^15 + p^17
	@ Therefore:
	@   cubert(a) = (a^e)^((2*p-1)/3) * (a^f)^((p-2)/3)
	@ Note that (2*p-1)/3 and (p-2)/3 are integers, since p = 2 mod 3.
	@
	@ Computations of a^e and a^f are done with the same process as
	@ at the start of gf_sqrt_inner(). The two extra exponentiations
	@ use dedicated addition chains.

	@ Stack layout:
	@   index  off  name
	@      0     0   t1
	@     10    40   t2
	@     20    80   t3
	@     30   120   t4
	@     50   160   pointer to output (c[])
	@     51   164   pointer to input (a[])

	sub	sp, #172

	str	r0, [sp, #160]   @ Output buffer pointer
	str	r1, [sp, #164]   @ Input buffer pointer

	@ a^(1+p^2) -> t1
	mov	r0, sp
	movs	r2, #1
	add	r3, sp, #40
	bl	gf_mulfrob_inner

	@ a^(1+p^2+p^4+p^6) -> t1
	mov	r0, sp
	mov	r1, sp
	movs	r2, #2
	add	r3, sp, #40
	bl	gf_mulfrob_inner

	@ a^(1+p^2+p^4+p^6+p^8+p^10+p^12+p^14) -> t1
	mov	r0, sp
	mov	r1, sp
	movs	r2, #3
	add	r3, sp, #40
	bl	gf_mulfrob_inner

	@ a^d = a^(1+p^2+p^4+p^6+p^8+p^10+p^12+p^14+p^16) -> t1
	add	r0, sp, #40
	mov	r1, sp
	movs	r2, #1
	bl	gf_frob_inner
	mov	r0, sp
	ldr	r1, [sp, #164]
	add	r2, sp, #40
	bl	gf_mul_inner

	@ a^f = (a^d)^p -> t2
	add	r0, sp, #40
	mov	r1, sp
	movs	r2, #0
	bl	gf_frob_inner

	@ a^e = a*((a^f)^p) -> t1
	mov	r0, sp
	add	r1, sp, #40
	movs	r2, #0
	bl	gf_frob_inner
	mov	r0, sp
	mov	r1, sp
	ldr	r2, [sp, #164]
	bl	gf_mul_inner

	@ Compute (a^e)^((2*p-1)/3) * ((a^f)^((p-2)/3). Note that:
	@   (2*p-1)/3 = 2 * ((p-2)/3) + 1
	@ Thus, we can compute the value as:
	@   (a^e) * ((a^(2*e+f))^((p-2)/3)

	@ Let u = a^(2*e+f) -> t2
	add	r0, sp, #80
	mov	r1, sp
	bl	gf_sqr_inner
	add	r0, sp, #40
	add	r1, sp, #40
	add	r2, sp, #80
	bl	gf_mul_inner

	@ u^3 -> t3
	add	r0, sp, #80
	add	r1, sp, #40
	bl	gf_sqr_inner
	add	r0, sp, #80
	add	r1, sp, #40
	add	r2, sp, #80
	bl	gf_mul_inner

	@ u^12 -> t4
	add	r0, sp, #120
	add	r1, sp, #80
	bl	gf_sqr_inner
	add	r0, sp, #120
	add	r1, sp, #120
	bl	gf_sqr_inner

	@ u^13 -> t2
	add	r0, sp, #40
	add	r1, sp, #40
	add	r2, sp, #120
	bl	gf_mul_inner

	@ u^25 -> t4
	add	r0, sp, #120
	add	r1, sp, #40
	add	r2, sp, #120
	bl	gf_mul_inner

	@ u^813 -> t4
	add	r0, sp, #120
	add	r1, sp, #120
	bl	gf_sqr_inner
	add	r0, sp, #120
	add	r1, sp, #120
	bl	gf_sqr_inner
	add	r0, sp, #120
	add	r1, sp, #120
	bl	gf_sqr_inner
	add	r0, sp, #120
	add	r1, sp, #120
	bl	gf_sqr_inner
	add	r0, sp, #120
	add	r1, sp, #120
	bl	gf_sqr_inner
	add	r0, sp, #120
	add	r1, sp, #40
	add	r2, sp, #120
	bl	gf_mul_inner

	@ u^3255 -> t4
	add	r0, sp, #120
	add	r1, sp, #120
	bl	gf_sqr_inner
	add	r0, sp, #120
	add	r1, sp, #120
	bl	gf_sqr_inner
	add	r0, sp, #120
	add	r1, sp, #80
	add	r2, sp, #120
	bl	gf_mul_inner

	@ Final product by a^e (still in t1).
	ldr	r0, [sp, #160]
	mov	r1, sp
	add	r2, sp, #120
	bl	gf_mul_inner

	add	sp, #172
	pop	{ pc }

	.size	gf_cubert_inner, .-gf_cubert_inner

@ =======================================================================
@ void curve9767_inner_gf_cubert(uint16_t *c, const uint16_t *a)
@
@ This is a public wrapper over gf_cubert_inner().
@ =======================================================================

	.align	1
	.global	curve9767_inner_gf_cubert
	.thumb
	.thumb_func
	.type	curve9767_inner_gf_cubert, %function
curve9767_inner_gf_cubert:
	push	{ r4, r5, r6, r7, lr }
	mov	r4, r8
	mov	r5, r10
	mov	r6, r11
	push	{ r4, r5, r6 }

	bl	gf_cubert_inner

	pop	{ r4, r5, r6 }
	mov	r8, r4
	mov	r10, r5
	mov	r11, r6
	pop	{ r4, r5, r6, r7, pc }
	.size	curve9767_inner_gf_cubert, .-curve9767_inner_gf_cubert

@ =======================================================================
@ uint32_t gf_eq_inner(const uint16_t *a, const uint16_t *b)
@
@ This function returns -1 if the two field elements are equal, 0
@ otherwise. The arrays MUST be 32-bit aligned.
@
@ This function does not comply with the ABI:
@    input parameters are r0 (a) and r1 (b)
@    output is in r2
@    r0 is incremented by 40
@    r1 is incremented by 40
@    r3..r7 are consumed
@    high registers are unmodified
@
@ Cost: 52
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_eq_inner, %function
gf_eq_inner:
	@ Load a0..a5 and b0..b5; temporary value stored in r2.
	ldm	r0!, { r2, r3, r4 }
	ldm	r1!, { r5, r6, r7 }
	eors	r2, r5
	eors	r3, r6
	eors	r4, r7
	orrs	r2, r3
	orrs	r2, r4

	@ Load a6..a11 and b6..b9; a10,a11 -> r5
	ldm	r0!, { r3, r4, r5 }
	ldm	r1!, { r6, r7 }
	eors	r3, r6
	eors	r4, r7
	orrs	r2, r3
	orrs	r2, r4

	@ Load a12..a17 and b10..b15; a16,a17 -> r5
	ldm	r1!, { r4, r6, r7 }
	eors	r4, r5
	orrs	r2, r4
	ldm	r0!, { r3, r4, r5 }
	eors	r3, r6
	eors	r4, r7
	orrs	r2, r3
	orrs	r2, r4

	@ Load a18 and b16..b18.
	ldm	r1!, { r6, r7 }
	eors	r6, r5
	orrs	r2, r6
	ldm	r0!, { r3 }
	eors	r3, r7
	uxth	r3, r3
	orrs	r2, r3

	@ Values are equal if and only if r2 is zero at this point.
	@ Since all coefficients are in 1..p, value r2 always fits
	@ on 30 bits (hence nonnegative).
	subs	r2, #1
	asrs	r2, #31
	bx	lr
	.size	gf_eq_inner, .-gf_eq_inner

@ =======================================================================
@ uint32_t curve9767_inner_gf_eq(const uint16_t *a, const uint16_t *b)
@
@ This function returns 1 if the two field elements are equal, 0
@ otherwise. This function complies with the ABI.
@ =======================================================================

	.align	1
	.global	curve9767_inner_gf_eq
	.thumb
	.thumb_func
	.type	curve9767_inner_gf_eq, %function
curve9767_inner_gf_eq:
	push	{ r4, r5, r6, r7, lr }
	bl	gf_eq_inner
	lsrs	r0, r2, #31
	pop	{ r4, r5, r6, r7, pc }
	.size	curve9767_inner_gf_eq, .-curve9767_inner_gf_eq

@ =======================================================================
@ void curve9767_point_add(curve9767_point *Q3,
@                          const curve9767_point *Q1,
@                          const curve9767_point *Q2)
@
@ This is a global function; it follows the ABI.
@
@ Cost: 16316
@ =======================================================================

	.align	1
	.global	curve9767_point_add
	.thumb
	.thumb_func
	.type	curve9767_point_add, %function
curve9767_point_add:
	push	{ r4, r5, r6, r7, lr }
	mov	r4, r8
	mov	r5, r10
	mov	r6, r11
	push	{ r4, r5, r6 }

	@ Stack layout (indices in 32-bit words):
	@   0    t1
	@   10   t2
	@   20   t3
	@   31   pointer to Q3 (output)
	@   32   pointer to Q1 (input 1)
	@   33   pointer to Q2 (input 2)
	@   34   ex = -1 if x1 == x2, 0 otherwise
	@   35   ey = -1 if y1 == y2, 0 otherwise
	sub	sp, #144
	str	r0, [sp, #124]
	str	r1, [sp, #128]
	str	r2, [sp, #132]

	@ Compute ex and ey.
	@ Note that x and y are at offsets 4 and 44 in curve9767_point,
	@ respectively.
	adds	r0, r1, #4
	adds	r1, r2, #4
	bl	gf_eq_inner
	str	r2, [sp, #136]
	bl	gf_eq_inner
	str	r2, [sp, #140]

	@ General case:
	@   lambda = (y2-y1)/(x2-x1)
	@ Special cases:
	@   Q1 == 0 and Q2 == 0: Q3 = 0
	@   Q1 == 0 and Q2 != 0: Q3 = Q2
	@   Q1 != 0 and Q2 == 0: Q3 = Q1
	@   Q1 == -Q2: Q3 = 0
	@   Q1 == Q2: lambda = (3*x1^2+a)/(2*y1)

	@ We compute lambda in t1. If x1 == x2, we just assume that
	@ this is a double (Q1 == Q2).

	@ If x1 == x2, set t1 to 2*y1; otherwise, set t1 to x2-x1.
	ldr	r1, [sp, #128]
	ldr	r2, [sp, #132]
	ldr	r5, [sp, #136]
	ldr	r6, const_curve_add1_p
	ldr	r7, const_curve_add1_p2i
	lsls	r3, r6, #16
	adds	r3, r6
	mov	r8, r3
	STEP_LAMBDA_D_X2  0,  0
	STEP_LAMBDA_D_X2  0,  2
	STEP_LAMBDA_D_X2  0,  4
	STEP_LAMBDA_D_X2  0,  6
	STEP_LAMBDA_D_X2  0,  8
	STEP_LAMBDA_D_X2  0, 10
	STEP_LAMBDA_D_X2  0, 12
	STEP_LAMBDA_D_X2  0, 14
	STEP_LAMBDA_D_X2  0, 16
	ldrh	r3, [r1, #(4 + 2 * 18)]
	ldrh	r4, [r2, #(4 + 2 * 18)]
	subs	r4, r3
	adds	r4, r6
	ldr	r0, [r1, #(44 + 2 * 18)]
	uxth	r0, r0
	lsls	r0, #1
	ands	r0, r5
	bics	r4, r5
	orrs	r0, r4
	BASERED  r0, r6, r7
	str	r0, [sp, #(4 * 0 + 2 * 18)]

	@ x1^2 -> t2
	add	r0, sp, #40
	adds	r1, #4
	bl	gf_sqr_inner

	@ If x1 == x2, set t2 to 3*x1^2+a; otherwise, set t2 to y2-y1.
	ldr	r1, [sp, #128]
	ldr	r2, [sp, #132]
	ldr	r5, [sp, #136]
	ldr	r6, const_curve_add1_p
	ldr	r7, const_curve_add1_p2i
	lsls	r3, r6, #16
	adds	r3, r6
	mov	r8, r3
	STEP_LAMBDA_N_X2  10, 10,  0
	STEP_LAMBDA_N_X2  10, 10,  2
	STEP_LAMBDA_N_X2  10, 10,  4
	STEP_LAMBDA_N_X2  10, 10,  6
	STEP_LAMBDA_N_X2  10, 10,  8
	STEP_LAMBDA_N_X2  10, 10, 10
	STEP_LAMBDA_N_X2  10, 10, 12
	STEP_LAMBDA_N_X2  10, 10, 14
	STEP_LAMBDA_N_X2  10, 10, 16
	ldr	r3, [r1, #(44 + 2 * 18)]
	ldr	r4, [r2, #(44 + 2 * 18)]
	subs	r4, r3
	add	r4, r8
	ldr	r0, [sp, #(4 * 10 + 2 * 18)]
	lsls	r3, r0, #1
	adds	r0, r3
	ands	r0, r5
	bics	r4, r5
	orrs	r0, r4
	uxth	r0, r0
	BASERED  r0, r6, r7
	str	r0, [sp, #(4 * 10 + 2 * 18)]

	@ Jump over the constants (included here to keep offsets in the
	@ supported range).
	b	curve9767_point_add_next

	.align	2
const_curve_add1_p:
	.long	9767
const_curve_add1_p2i:
	.long	439742
const_curve_add1_a:
	.long	7755

curve9767_point_add_next:
	@ Compute lambda = t2/t1 (in t1).
	mov	r0, sp
	mov	r1, sp
	bl	gf_inv_inner
	mov	r0, sp
	mov	r1, sp
	add	r2, sp, #40
	bl	gf_mul_inner

	@ Compute x3 = lambda^2 - x1 - x2 (in t2)
	add	r0, sp, #40
	mov	r1, sp
	bl	gf_sqr_inner
	ldr	r1, [sp, #128]
	ldr	r2, [sp, #132]
	ldr	r6, const_curve_add2_p
	ldr	r7, const_curve_add2_p2i
	lsls	r0, r6, #17
	adds	r0, r6
	adds	r0, r6
	mov	r8, r0
	STEP_SUB2_X2  10, 10,  0
	STEP_SUB2_X2  10, 10,  2
	STEP_SUB2_X2  10, 10,  4
	STEP_SUB2_X2  10, 10,  6
	STEP_SUB2_X2  10, 10,  8
	STEP_SUB2_X2  10, 10, 10
	STEP_SUB2_X2  10, 10, 12
	STEP_SUB2_X2  10, 10, 14
	STEP_SUB2_X2  10, 10, 16
	ldr	r3, [sp, #(4 * 10 + 2 * 18)]
	add	r3, r8
	uxth	r3, r3
	ldrh	r4, [r1, #(4 + 2 * 18)]
	subs	r3, r4
	ldrh	r4, [r2, #(4 + 2 * 18)]
	subs	r3, r4
	BASERED  r3, r6, r7
	str	r3, [sp, #(4 * 10 + 2 * 18)]

	@ Compute y3 = lambda*(x1 - x3) - y1 (in t3)
	add	r0, sp, #80
	adds	r1, #4
	add	r2, sp, #40
	bl	gf_sub_inner
	add	r0, sp, #80
	mov	r1, sp
	add	r2, sp, #80
	bl	gf_mul_inner
	add	r0, sp, #80
	add	r1, sp, #80
	ldr	r2, [sp, #128]
	adds	r2, #44
	bl	gf_sub_inner

	@ At this point, if Q1 != 0, Q2 != 0, and Q1 != -Q2, then the
	@ result is a non-zero point, and its coordinates are in (t2,t3).
	@
	@ If either Q1 or Q2 is zero, then we use the coordinates from
	@ the other one. If neither is zero, then we use the values
	@ computed in t2 and t3. Note that t2 and t3 are consecutive,
	@ with a 2-byte gap, just like the x and y coordinates in the
	@ curve9767_point structure, so we can process them in a single
	@ loop.

	ldr	r0, [sp, #124]
	adds	r0, #4
	ldr	r1, [sp, #128]
	ldr	r3, [r1]
	rsbs	r3, r3, #0
	mov	r8, r3
	adds	r1, #4
	ldr	r2, [sp, #132]
	ldr	r7, [r2]
	rsbs	r7, r7, #0
	adds	r2, #4
	mvns	r3, r3
	bics	r3, r7
	mov	r10, r3
	STEP_SELECT3_STACK_X4  10,  0
	STEP_SELECT3_STACK_X4  10,  4
	STEP_SELECT3_STACK_X4  10,  8
	STEP_SELECT3_STACK_X4  10, 12
	STEP_SELECT3_STACK_X4  10, 16
	STEP_SELECT3_STACK_X4  10, 20
	STEP_SELECT3_STACK_X4  10, 24
	STEP_SELECT3_STACK_X4  10, 28
	STEP_SELECT3_STACK_X4  10, 32
	STEP_SELECT3_STACK_X4  10, 36

	@ We handled all the cases that yield a non-zero value:
	@   Q1 != 0 and Q2 != 0 and Q1 != Q2 and Q1 != -Q2
	@   Q1 != 0 and Q1 == Q2
	@   Q1 == 0 and Q2 != 0
	@   Q1 != 0 and Q2 == 0
	@ (Note that 2*Q1 cannot be zero if Q1 != 0, since the curve has
	@ an odd order.)
	@ Remaining cases are those that yield a 0:
	@   Q1 != 0 and Q1 == -Q2
	@   Q1 == 0 and Q2 == 0
	subs	r1, #84
	subs	r2, #84
	ldr	r3, [r1]        @ Q1->neutral
	ldr	r4, [r2]        @ Q2->neutral
	ldr	r5, [sp, #136]  @ ex
	ldr	r6, [sp, #140]  @ ey
	movs	r7, #1
	subs	r1, r7, r3
	subs	r2, r7, r4
	ands	r1, r2
	ands	r1, r5
	bics	r1, r6
	ands	r3, r4
	orrs	r1, r3
	ldr	r0, [sp, #124]
	str	r1, [r0]

	add	sp, #144
	pop	{ r4, r5, r6 }
	mov	r8, r4
	mov	r10, r5
	mov	r11, r6
	pop	{ r4, r5, r6, r7, pc }

	.align	2
const_curve_add2_p:
	.long	9767
const_curve_add2_p2i:
	.long	439742

	.size	curve9767_point_add, .-curve9767_point_add

@ =======================================================================
@ void curve9767_point_mul2k(curve9767_point *Q3,
@                            const curve9767_point *Q1, unsigned k)
@
@ This is a global function; it follows the ABI.
@
@ Cost:
@    k == 0    66
@    k == 1    16337
@    k >= 2    18976 + (k-1)*11392
@ =======================================================================

	.align	1
	.global	curve9767_point_mul2k
	.thumb
	.thumb_func
	.type	curve9767_point_mul2k, %function
curve9767_point_mul2k:
	@ If k > 1, then we jump to the general case with Jacobian coordinates.
	cmp	r2, #1
	bhi	curve_mul2k_generic

	@ If k == 1, then we tail-call the addition function; otherwise,
	@ this is a simple copy.
	bne	curve_mul2k_copy
	movs	r2, r1
	b	curve9767_point_add
curve_mul2k_copy:
	push	{ r4, r5, r6, r7 }
	ldm	r1!, { r2, r3, r4, r5, r6, r7 }
	stm	r0!, { r2, r3, r4, r5, r6, r7 }
	ldm	r1!, { r2, r3, r4, r5, r6, r7 }
	stm	r0!, { r2, r3, r4, r5, r6, r7 }
	ldm	r1!, { r2, r3, r4, r5, r6, r7 }
	stm	r0!, { r2, r3, r4, r5, r6, r7 }
	ldm	r1!, { r2, r3, r4 }
	stm	r0!, { r2, r3, r4 }
	pop	{ r4, r5, r6, r7 }
	bx	lr

curve_mul2k_generic:
	@ Generic code. We now create the stack frame.
	@   index   offset
	@     0        0      X
	@    10       40      Y
	@    20       80      Z
	@    30      120      XX
	@    40      160      S (also XXXX and ZZZZ)
	@    50      200      ZZ (also x*(z^9))
	@    60      240      YY
	@    70      280      YYYY
	@    80      320      M (also (y^2)*(z^9))
	@    90      360      pointer to output (Q3)
	@    91      364      pointer to input (Q1)
	@    92      368      remaining iteration count
	@
	@ Note that XX, XXXX and x*(z^9) are consecutive and in that order;
	@ this allows reading from all three with a single moving pointer.
	@ Similarly, YYYY and (y^2)*(z^9) are consecutive.

	push	{ r4, r5, r6, r7, lr }
	mov	r4, r8
	mov	r5, r10
	mov	r6, r11
	push	{ r4, r5, r6 }
	sub	sp, #376

	str	r0, [sp, #360]
	str	r1, [sp, #364]
	subs	r2, #1
	str	r2, [sp, #368]

	@ Q3 = infinity if and only if Q1 = infinity, so we just copy the
	@ neutral flag from Q1 to Q3.
	ldr	r3, [r1]
	str	r3, [r0]

	@ First double can use optimized equations with four squarings;
	@ idea is from: https://eprint.iacr.org/2011/039.pdf
	@  X = x^4 - 2*a*x^2 + a^2 - 8*b*x
	@  Y = y^4 + 18*b*y^2 + 3*a*x^4 - 6*a^2*x^2 - 24*a*b*x - 27*b^2 - a^3
	@  Z = 2y

	@ x^2 -> XX
	add	r0, sp, #120
	adds	r1, #4
	bl	gf_sqr_inner

	@ x^4 -> XXXX
	add	r0, sp, #160
	add	r1, sp, #120
	bl	gf_sqr_inner

	@ y^2 -> YY
	add	r0, sp, #240
	ldr	r1, [sp, #364]
	adds	r1, #44
	bl	gf_sqr_inner

	@ y^4 -> YYYY
	add	r0, sp, #280
	add	r1, sp, #240
	bl	gf_sqr_inner

	@ x*(z^9) -> ZZ
	add	r0, sp, #200
	ldr	r1, [sp, #364]
	adds	r1, #4
	bl	gf_mulz9_inner

	@ (y^2)*(z^9) -> M
	add	r0, sp, #320
	add	r1, sp, #240
	bl	gf_mulz9_inner

	@ Assemble coordinates:
	@   X = x^4 - 2*a*x^2 - 8*b*x + a^2
	@   Y = y^4 + 18*b*y^2 + 3*a*(x^4 - 2*a*x^2 - 8*b*x) - 27*b^2 - a^3
	@   r0    pointer to x^2 || x^4 || x*(z^9)
	@   r1    pointer to y^4 || (y^2)*(z^9)
	@   r14   iteration counter
	@
	@ Constants:
	@   r5    low: R = 7182, high: -2*a*R = 4024
	@   r6    low: -8*b*R = 2928, high: 18*b*R = 3179
	@   r7    p = 9767
	add	r0, sp, #120
	add	r1, sp, #280
	movs	r2, #10
	mov	r14, r2
	ldr	r5, const_mul2k_cc1
	ldr	r6, const_mul2k_cc2
	ldr	r7, const_mul2k_p

mul2k_loop1:
	@ Compute x^4 - 2*a*x^2 - 8*b*x (two words, into r8:r10)

	@ 2 words from x^4  (multiplier is R)
	ldr	r2, [r0, #40]
	lsrs	r3, r2, #16
	uxth	r2, r2
	uxth	r4, r5
	muls	r2, r4
	muls	r3, r4
	mov	r8, r2
	mov	r10, r3

	@ 2 words from x^2  (multiplier is -2*a*R)
	ldr	r2, [r0]
	lsrs	r3, r2, #16
	uxth	r2, r2
	lsrs	r4, r5, #16
	muls	r2, r4
	muls	r3, r4
	add	r8, r2
	add	r10, r3

	@ 2 words from x*(z^9)  (multiplier is -8*b*R)
	ldr	r2, [r0, #80]
	lsrs	r3, r2, #16
	uxth	r2, r2
	uxth	r4, r6
	muls	r2, r4
	muls	r3, r4
	add	r2, r8
	add	r3, r10
	mov	r8, r2
	mov	r10, r3

	@ The two words are both in r2:r3 and r8:r10.

	@ Compute x^4 + 18*b*y^2 + 3*a*(x^4 - 2*a*x^2 - 8*b*x)
	@ (two words, into r11:r12)

	@ Multiply r2 and r3 by 3*a = -9. On input, maximum value for r2
	@ and r3 is 166644554, so multiplying by 9 makes it at most
	@ 1499800986. We can subtract 9*r2 and 9*r3 from 16*(9767^2),
	@ which is greater (1526308624), and thus obtain a positive result.
	movs	r4, #9
	rsbs	r4, r4, #0
	muls	r2, r4
	muls	r3, r4
	movs	r4, r7
	muls	r4, r4
	lsls	r4, #4
	adds	r2, r4
	adds	r3, r4
	mov	r11, r2
	mov	r12, r3

	@ 2 words from (y^2)*(z^9)  (multiplier is 18*b*R)
	ldr	r2, [r1, #40]
	lsrs	r3, r2, #16
	uxth	r2, r2
	lsrs	r4, r6, #16
	muls	r2, r4
	muls	r3, r4
	add	r11, r2
	add	r12, r3

	@ 2 words from y^4  (multiplier is R)
	ldm	r1!, { r2 }
	lsrs	r3, r2, #16
	uxth	r2, r2
	uxth	r4, r5
	muls	r2, r4
	muls	r3, r4
	add	r2, r11
	add	r3, r12

	@ Apply Montgomery reduction and write out words in X and Y
	ldr	r4, const_mul2k_p1i
	subs	r0, #120
	MONTYRED  r2, r7, r4
	MONTYRED  r3, r7, r4
	lsls	r3, #16
	orrs	r2, r3
	str	r2, [r0, #40]
	mov	r2, r8
	mov	r3, r10
	MONTYRED  r2, r7, r4
	MONTYRED  r3, r7, r4
	lsls	r3, #16
	orrs	r2, r3
	str	r2, [r0]
	adds	r0, #124

	@ Loop 10 times
	mov	r4, r14
	subs	r4, #1
	mov	r14, r4
	bne	mul2k_loop1

	@ Add a^2 to X, and -27*b^2-a^3 to Y
	mov	r0, sp
	ldrh	r1, [r0]
	ldr	r2, const_mul2k_9
	adds	r1, r2
	MPMOD_AFTER_ADD  r1, r2, r7
	strh	r1, [r0]
	add	r0, sp, #40
	ldrh	r1, [r0]
	ldr	r2, const_mul2k_27
	adds	r1, r2
	MPMOD_AFTER_ADD  r1, r2, r7
	strh	r1, [r0]
	ldrh	r1, [r0, #36]
	ldr	r2, const_mul2k_m27b2
	adds	r1, r2
	MPMOD_AFTER_ADD  r1, r2, r7
	strh	r1, [r0, #36]

	@ Store Z <- 2*y, ZZ <- 4*(y^2), ZZZZ <- 16*(y^4)
	add	r0, sp, #80
	ldr	r1, [sp, #364]
	adds	r1, #44
	bl	gf_mul2_move_inner
	add	r0, sp, #200
	add	r1, sp, #240
	bl	gf_mul4_move_inner
	add	r0, sp, #160
	add	r1, sp, #280
	bl	gf_mul16_move_inner

	@ Jump into the middle of the loop below, because Z^2 and Z^4 have
	@ already been computed.
	b	mul2k_loop2_alt

	@ Main loop, for doublings in Jacobian coordinates.
mul2k_loop2:

	@ ZZ = Z1^2
	add	r0, sp, #200
	add	r1, sp, #80
	bl	gf_sqr_inner

	@ ZZZZ = Z1^4
	add	r0, sp, #160
	add	r1, sp, #200
	bl	gf_sqr_inner

mul2k_loop2_alt:

	@ XX = X1^2
	add	r0, sp, #120
	mov	r1, sp
	bl	gf_sqr_inner

	@ M = 3*XX + a*ZZZZ  (with a = -3)
	add	r0, sp, #320
	add	r1, sp, #120
	add	r2, sp, #160
	bl	gf_sub_mul3_move_inner

	@ YY = Y1^2
	add	r0, sp, #240
	add	r1, sp, #40
	bl	gf_sqr_inner

	@ YYYY = YY^2
	add	r0, sp, #280
	add	r1, sp, #240
	bl	gf_sqr_inner

	@ S = 2*((X1+YY)^2-XX-YYYY)
	add	r0, sp, #160
	mov	r1, sp
	add	r2, sp, #240
	bl	gf_add_inner
	add	r0, sp, #160
	movs	r1, r0
	bl	gf_sqr_inner
	add	r0, sp, #160
	add	r1, sp, #120
	add	r2, sp, #280
	bl	gf_sub_sub_mul2_inner

	@ XX = Y1+Z1
	add	r0, sp, #120
	add	r1, sp, #40
	add	r2, sp, #80
	bl	gf_add_inner

	@ X3 = M^2-2*S
	mov	r0, sp
	add	r1, sp, #320
	bl	gf_sqr_inner
	mov	r0, sp
	add	r1, sp, #160
	bl	gf_sub2_inner

	@ Y3 = M*(S-X3) - 8*YYYY
	add	r0, sp, #160
	movs	r1, r0
	mov	r2, sp
	bl	gf_sub_inner
	add	r0, sp, #40
	add	r1, sp, #320
	add	r2, sp, #160
	bl	gf_mul_inner
	add	r0, sp, #40
	add	r1, sp, #280
	bl	gf_sub8_inner

	@ Z3 = (Y1+Z1)^2-YY-ZZ  (Y1+Z1 has been computed into XX)
	add	r0, sp, #80
	add	r1, sp, #120
	bl	gf_sqr_inner
	add	r0, sp, #80
	add	r1, sp, #240
	add	r2, sp, #200
	bl	gf_sub_sub_inner

	@ Loop until all doublings have been computed.
	ldr	r0, [sp, #368]
	subs	r0, #1
	str	r0, [sp, #368]
	bne	mul2k_loop2

	@ Convert back to affine coordinates.
	add	r0, sp, #200
	add	r1, sp, #80
	bl	gf_inv_inner
	add	r0, sp, #160
	add	r1, sp, #200
	bl	gf_sqr_inner
	add	r0, sp, #200
	add	r1, sp, #200
	add	r2, sp, #160
	bl	gf_mul_inner
	ldr	r0, [sp, #360]
	adds	r0, #4
	mov	r1, sp
	add	r2, sp, #160
	bl	gf_mul_inner
	ldr	r0, [sp, #360]
	adds	r0, #44
	add	r1, sp, #40
	add	r2, sp, #200
	bl	gf_mul_inner

	@ Q3->neutral was already set (copy of Q1->neutral), so we're all
	@ set here.

	add	sp, #376
	pop	{ r4, r5, r6 }
	mov	r8, r4
	mov	r10, r5
	mov	r11, r6
	pop	{ r4, r5, r6, r7, pc }

	.align	2
const_mul2k_cc1:
	.long	263724046
const_mul2k_cc2:
	.long	208341872
const_mul2k_p:
	.long	9767
const_mul2k_p1i:
	.long	3635353193
const_mul2k_9:
	.long	6036
const_mul2k_27:
	.long	8341
const_mul2k_m27b2:
	.long	1112

	.size	curve9767_point_mul2k, .-curve9767_point_mul2k

@ =======================================================================
@ void curve9767_inner_window_lookup(curve9767_point *Q,
@                                    curve window_point8 *window, uint32_t k)
@
@ This is a global function; it follows the ABI.
@
@ Cost: 777
@ =======================================================================

	.align	1
	.global	curve9767_inner_window_lookup
	.thumb
	.thumb_func
	.type	curve9767_inner_window_lookup, %function
curve9767_inner_window_lookup:
	push	{ r4, r5, r6, r7, lr }
	mov	r4, r8
	mov	r5, r10
	mov	r6, r11
	push	{ r4, r5, r6 }

	@ We compute and store the eight masks into the following places:
	@   m0  r5
	@   m1  r6
	@   m2  r7
	@   m3  r8
	@   m4  r10
	@   m5  r11
	@   m6  r12
	@   m7  r14

	@ m0
	subs	r2, r2, #1
	asrs	r5, r2, #31

	@ m1
	eors	r6, r6
	subs	r2, #1
	sbcs	r6, r6

	@ m2
	eors	r7, r7
	subs	r2, #1
	sbcs	r7, r7

	@ m3
	eors	r3, r3
	subs	r2, #1
	sbcs	r3, r3
	mov	r8, r3

	@ m4
	eors	r3, r3
	subs	r2, #1
	sbcs	r3, r3
	mov	r10, r3

	@ m5
	eors	r3, r3
	subs	r2, #1
	sbcs	r3, r3
	mov	r11, r3

	@ m6
	eors	r3, r3
	subs	r2, #1
	sbcs	r3, r3
	mov	r12, r3

	@ m7
	eors	r3, r3
	subs	r2, #1
	sbcs	r3, r3
	mov	r14, r3

	@ Set r0 to point to the start of the X coordinate of the destination.
	adds	r0, #4

	@ Thanks to the window in-memory layout, we can simply process
	@ words with 20 iteration of a macro.
	STEP_WINDOW_LOOKUP
	STEP_WINDOW_LOOKUP
	STEP_WINDOW_LOOKUP
	STEP_WINDOW_LOOKUP
	STEP_WINDOW_LOOKUP
	STEP_WINDOW_LOOKUP
	STEP_WINDOW_LOOKUP
	STEP_WINDOW_LOOKUP
	STEP_WINDOW_LOOKUP
	STEP_WINDOW_LOOKUP
	STEP_WINDOW_LOOKUP
	STEP_WINDOW_LOOKUP
	STEP_WINDOW_LOOKUP
	STEP_WINDOW_LOOKUP
	STEP_WINDOW_LOOKUP
	STEP_WINDOW_LOOKUP
	STEP_WINDOW_LOOKUP
	STEP_WINDOW_LOOKUP
	STEP_WINDOW_LOOKUP
	STEP_WINDOW_LOOKUP

	pop	{ r4, r5, r6 }
	mov	r8, r4
	mov	r10, r5
	mov	r11, r6
	pop	{ r4, r5, r6, r7, pc }
	.size	curve9767_inner_window_lookup, .-curve9767_inner_window_lookup

@ =======================================================================
@ const window_point8 curve9767_inner_window_G;
@ =======================================================================

	.align	2
	.global	curve9767_inner_window_G
curve9767_inner_window_G:
	@ X[0], X[1]
	.long	640099879
	.long	640099879
	.long	640099879
	.long	129505983
	.long	445974690
	.long	482547483
	.long	102701113
	.long	279380687
	@ X[2], X[3]
	.long	640099879
	.long	640099879
	.long	640099879
	.long	420226296
	.long	428417540
	.long	83501527
	.long	322112075
	.long	608641644
	@ X[4], X[5]
	.long	640099879
	.long	640099879
	.long	640099879
	.long	332341225
	.long	521801516
	.long	59908282
	.long	359014060
	.long	329974812
	@ X[6], X[7]
	.long	640099879
	.long	640099879
	.long	640099879
	.long	215818783
	.long	325193920
	.long	349706224
	.long	266010713
	.long	321853599
	@ X[8], X[9]
	.long	640099879
	.long	640099879
	.long	63963963
	.long	244195071
	.long	67307115
	.long	380044923
	.long	366944190
	.long	350291124
	@ X[10], X[11]
	.long	640099879
	.long	640095561
	.long	640099879
	.long	373429032
	.long	494600329
	.long	30614193
	.long	339612025
	.long	108070618
	@ X[12], X[13]
	.long	640099879
	.long	640099879
	.long	640099879
	.long	616047889
	.long	41486349
	.long	320078855
	.long	92478813
	.long	432154059
	@ X[14], X[15]
	.long	640099879
	.long	640099879
	.long	640099879
	.long	568725562
	.long	272834099
	.long	15799511
	.long	254411922
	.long	362747351
	@ X[16], X[17]
	.long	640099879
	.long	640099879
	.long	640099879
	.long	605428543
	.long	231019653
	.long	389809606
	.long	460922422
	.long	498340403
	@ X[18]
	.long	9767
	.long	9767
	.long	9767
	.long	441
	.long	7420
	.long	3000
	.long	6451
	.long	8865
	@ Y[0], Y[1]
	.long	640099879
	.long	640099879
	.long	640099879
	.long	186780972
	.long	413143489
	.long	403509943
	.long	258154435
	.long	321262204
	@ Y[2], Y[3]
	.long	640099879
	.long	640099879
	.long	640099879
	.long	293869225
	.long	168562821
	.long	506798437
	.long	74845467
	.long	409474541
	@ Y[4], Y[5]
	.long	640099879
	.long	640099879
	.long	640099879
	.long	182191571
	.long	631704200
	.long	263592921
	.long	482738833
	.long	311369043
	@ Y[6], Y[7]
	.long	640099879
	.long	640099879
	.long	640099879
	.long	8590142
	.long	41026166
	.long	565313693
	.long	255731105
	.long	303172917
	@ Y[8], Y[9]
	.long	640099879
	.long	640099879
	.long	640099879
	.long	463800021
	.long	230169193
	.long	371857893
	.long	173280734
	.long	336267259
	@ Y[10], Y[11]
	.long	640099879
	.long	640099879
	.long	640099879
	.long	311166585
	.long	438445431
	.long	53283205
	.long	475210650
	.long	219549847
	@ Y[12], Y[13]
	.long	640099879
	.long	640099879
	.long	261229695
	.long	151650528
	.long	70586537
	.long	100011026
	.long	249634384
	.long	612696251
	@ Y[14], Y[15]
	.long	640095295
	.long	423432680
	.long	640095894
	.long	425330145
	.long	462162769
	.long	591597608
	.long	118826203
	.long	168168634
	@ Y[16], Y[17]
	.long	640099879
	.long	640099879
	.long	640099879
	.long	80155398
	.long	461120593
	.long	407837149
	.long	228855597
	.long	152568901
	@ Y[18]
	.long	9767
	.long	9767
	.long	9767
	.long	9118
	.long	4309
	.long	2711
	.long	3881
	.long	4392
	.size	curve9767_inner_window_G, .-curve9767_inner_window_G

@ =======================================================================
@ const window_point8 curve9767_inner_window_G64;
@ =======================================================================

	.align	2
	.global	curve9767_inner_window_G64
curve9767_inner_window_G64:
	@ X[0], X[1]
	.long	564794650
	.long	499328123
	.long	579928072
	.long	398072132
	.long	375394389
	.long	612900434
	.long	59974088
	.long	211687693
	@ X[2], X[3]
	.long	378012590
	.long	356452050
	.long	393806193
	.long	380045812
	.long	290595840
	.long	100997488
	.long	259001571
	.long	520297350
	@ X[4], X[5]
	.long	255000589
	.long	540608532
	.long	528287842
	.long	605292680
	.long	123280186
	.long	37033706
	.long	253109520
	.long	187833964
	@ X[6], X[7]
	.long	609099095
	.long	50602459
	.long	245373465
	.long	537011071
	.long	33687968
	.long	212731657
	.long	330242361
	.long	181605063
	@ X[8], X[9]
	.long	550638948
	.long	169091509
	.long	104732866
	.long	293734477
	.long	209913569
	.long	332924577
	.long	268247222
	.long	240517551
	@ X[10], X[11]
	.long	537924078
	.long	98110927
	.long	171710846
	.long	303629884
	.long	577636929
	.long	285150519
	.long	522978648
	.long	338890155
	@ X[12], X[13]
	.long	383329668
	.long	61742571
	.long	578486774
	.long	113247427
	.long	321264006
	.long	447152502
	.long	575481098
	.long	560537748
	@ X[14], X[15]
	.long	413534787
	.long	277610914
	.long	82445260
	.long	322052491
	.long	28902504
	.long	293212827
	.long	303635651
	.long	540414768
	@ X[16], X[17]
	.long	254611966
	.long	400167360
	.long	622985379
	.long	497162909
	.long	566759386
	.long	605621061
	.long	274993311
	.long	308743464
	@ X[18]
	.long	4662
	.long	3463
	.long	1760
	.long	9632
	.long	4520
	.long	1300
	.long	2136
	.long	1096
	@ Y[0], Y[1]
	.long	267199578
	.long	134092987
	.long	629084772
	.long	401873782
	.long	520228356
	.long	460134945
	.long	551554445
	.long	480186441
	@ Y[2], Y[3]
	.long	5443450
	.long	106565639
	.long	263397482
	.long	285745277
	.long	170135295
	.long	260575576
	.long	86910432
	.long	30278080
	@ Y[4], Y[5]
	.long	607853356
	.long	461571284
	.long	226233801
	.long	570167903
	.long	129958316
	.long	515968371
	.long	390210409
	.long	393090480
	@ Y[6], Y[7]
	.long	166202540
	.long	16254070
	.long	133568081
	.long	205917586
	.long	191367790
	.long	223747091
	.long	324997524
	.long	473237552
	@ Y[8], Y[9]
	.long	634329658
	.long	488117021
	.long	560401162
	.long	398664546
	.long	620962933
	.long	578225293
	.long	363860211
	.long	163389892
	@ Y[10], Y[11]
	.long	596905363
	.long	54592392
	.long	321267061
	.long	60301785
	.long	369501487
	.long	616366857
	.long	125634470
	.long	288755908
	@ Y[12], Y[13]
	.long	289612193
	.long	300877615
	.long	599261971
	.long	46536248
	.long	378281012
	.long	386793526
	.long	393220947
	.long	476912091
	@ Y[14], Y[15]
	.long	312283719
	.long	289149927
	.long	34219315
	.long	618209882
	.long	104793070
	.long	295901152
	.long	390792206
	.long	579406044
	@ Y[16], Y[17]
	.long	146935000
	.long	263590293
	.long	427493990
	.long	154410838
	.long	140447232
	.long	569580679
	.long	270667696
	.long	461377824
	@ Y[18]
	.long	7206
	.long	1789
	.long	9544
	.long	5285
	.long	4050
	.long	2848
	.long	8603
	.long	7167
	.size	curve9767_inner_window_G64, .-curve9767_inner_window_G64

@ =======================================================================
@ const window_point8 curve9767_inner_window_G128;
@ =======================================================================

	.align	2
	.global	curve9767_inner_window_G128
curve9767_inner_window_G128:
	@ X[0], X[1]
	.long	17236348
	.long	189466057
	.long	373168638
	.long	203688053
	.long	177999509
	.long	348919455
	.long	114820281
	.long	472584188
	@ X[2], X[3]
	.long	268505751
	.long	32968884
	.long	96735803
	.long	242099419
	.long	16129590
	.long	102957701
	.long	137306197
	.long	195823414
	@ X[4], X[5]
	.long	12386485
	.long	447026436
	.long	352588363
	.long	593366288
	.long	321258108
	.long	380768604
	.long	223218477
	.long	230754359
	@ X[6], X[7]
	.long	302125966
	.long	496574757
	.long	238950921
	.long	127672895
	.long	391059821
	.long	238294639
	.long	483992361
	.long	15863394
	@ X[8], X[9]
	.long	418063398
	.long	305469001
	.long	417992791
	.long	386406891
	.long	25365912
	.long	172556503
	.long	565314750
	.long	219155541
	@ X[10], X[11]
	.long	383064192
	.long	620303171
	.long	217646860
	.long	498213303
	.long	489816095
	.long	389418663
	.long	552994336
	.long	101450142
	@ X[12], X[13]
	.long	199697607
	.long	326115749
	.long	164829361
	.long	251794064
	.long	538319495
	.long	157552476
	.long	141829348
	.long	46276053
	@ X[14], X[15]
	.long	582026812
	.long	593496502
	.long	30346371
	.long	40110129
	.long	625869533
	.long	30015913
	.long	360644717
	.long	267325668
	@ X[16], X[17]
	.long	355408911
	.long	513545776
	.long	486090116
	.long	5379569
	.long	61669389
	.long	320479883
	.long	357368688
	.long	63184501
	@ X[18]
	.long	2235
	.long	3053
	.long	4611
	.long	684
	.long	897
	.long	6863
	.long	5670
	.long	5944
	@ Y[0], Y[1]
	.long	618076179
	.long	394003904
	.long	604441824
	.long	260969392
	.long	308812805
	.long	591208291
	.long	127074449
	.long	364316602
	@ Y[2], Y[3]
	.long	634130693
	.long	1184555
	.long	539696128
	.long	500376274
	.long	529079470
	.long	92152652
	.long	278464111
	.long	283380207
	@ Y[4], Y[5]
	.long	364057191
	.long	504766690
	.long	167776056
	.long	317071964
	.long	207361890
	.long	305989684
	.long	540410256
	.long	211688561
	@ Y[6], Y[7]
	.long	367464014
	.long	310382014
	.long	520162213
	.long	10100416
	.long	585178006
	.long	427626890
	.long	446571282
	.long	114558076
	@ Y[8], Y[9]
	.long	522521495
	.long	498212163
	.long	355668382
	.long	596320369
	.long	111613303
	.long	414518485
	.long	533600747
	.long	171253448
	@ Y[10], Y[11]
	.long	300946172
	.long	545202743
	.long	195756140
	.long	178000809
	.long	309395554
	.long	94833003
	.long	251464729
	.long	106371290
	@ Y[12], Y[13]
	.long	468916655
	.long	547693534
	.long	137959792
	.long	577374096
	.long	361567100
	.long	407964940
	.long	566367020
	.long	102236575
	@ Y[14], Y[15]
	.long	312870034
	.long	609485171
	.long	178263745
	.long	204866856
	.long	567738977
	.long	570892956
	.long	180683175
	.long	135860596
	@ Y[16], Y[17]
	.long	342558582
	.long	295570080
	.long	114166076
	.long	260252831
	.long	278926932
	.long	107288021
	.long	161618117
	.long	230826864
	@ Y[18]
	.long	2442
	.long	9284
	.long	7087
	.long	4511
	.long	2286
	.long	3160
	.long	2473
	.long	875
	.size	curve9767_inner_window_G128, .-curve9767_inner_window_G128

@ =======================================================================
@ const window_point8 curve9767_inner_window_G192;
@ =======================================================================

	.align	2
	.global	curve9767_inner_window_G192
curve9767_inner_window_G192:
	@ X[0], X[1]
	.long	487923927
	.long	296097422
	.long	277741868
	.long	524095475
	.long	444406125
	.long	72224004
	.long	629808174
	.long	105252896
	@ X[2], X[3]
	.long	119410636
	.long	99620159
	.long	83364517
	.long	487786922
	.long	449447056
	.long	96799945
	.long	266019388
	.long	114892221
	@ X[4], X[5]
	.long	539690686
	.long	620044956
	.long	184878873
	.long	242030144
	.long	12196620
	.long	423564164
	.long	177474310
	.long	153551621
	@ X[6], X[7]
	.long	383459067
	.long	526394565
	.long	15204405
	.long	408621790
	.long	172951206
	.long	418192239
	.long	352720382
	.long	595005709
	@ X[8], X[9]
	.long	27663764
	.long	194581007
	.long	192355792
	.long	577117276
	.long	414122966
	.long	101386110
	.long	577180993
	.long	347803074
	@ X[10], X[11]
	.long	164171441
	.long	118889445
	.long	80355891
	.long	226558974
	.long	509477267
	.long	417994969
	.long	541267288
	.long	607722803
	@ X[12], X[13]
	.long	540940961
	.long	269353949
	.long	578756860
	.long	224329979
	.long	553984253
	.long	330440511
	.long	515375892
	.long	617878426
	@ X[14], X[15]
	.long	1573893
	.long	398397873
	.long	340922312
	.long	267717128
	.long	304420167
	.long	80290073
	.long	263981103
	.long	603200850
	@ X[16], X[17]
	.long	374545093
	.long	329651534
	.long	338102701
	.long	184949169
	.long	626266369
	.long	280499087
	.long	377097335
	.long	89987662
	@ X[18]
	.long	7561
	.long	555
	.long	7411
	.long	8893
	.long	7406
	.long	9752
	.long	8166
	.long	2019
	@ Y[0], Y[1]
	.long	174924226
	.long	34412123
	.long	548673923
	.long	37552930
	.long	114171090
	.long	67765003
	.long	582816345
	.long	162208741
	@ Y[2], Y[3]
	.long	372245287
	.long	182588341
	.long	597626521
	.long	532087636
	.long	412291380
	.long	515115153
	.long	432280542
	.long	464914032
	@ Y[4], Y[5]
	.long	478020586
	.long	134422371
	.long	383519018
	.long	327883826
	.long	600771897
	.long	237053014
	.long	384172816
	.long	478092287
	@ Y[6], Y[7]
	.long	22610123
	.long	378741262
	.long	135861383
	.long	66453629
	.long	420872692
	.long	186390885
	.long	415833750
	.long	383653433
	@ Y[8], Y[9]
	.long	265618919
	.long	306778668
	.long	5048527
	.long	510854488
	.long	398793159
	.long	125703217
	.long	535108408
	.long	123077266
	@ Y[10], Y[11]
	.long	357172651
	.long	298919071
	.long	179511805
	.long	463472675
	.long	618332368
	.long	7803767
	.long	566493500
	.long	8653476
	@ Y[12], Y[13]
	.long	284040917
	.long	89463035
	.long	457777588
	.long	318442283
	.long	234691351
	.long	83040130
	.long	20653394
	.long	607851921
	@ Y[14], Y[15]
	.long	263331430
	.long	111937912
	.long	41683056
	.long	379463053
	.long	145691703
	.long	51645126
	.long	48243833
	.long	497950408
	@ Y[16], Y[17]
	.long	622068705
	.long	555158892
	.long	117902408
	.long	444994411
	.long	142279855
	.long	91628577
	.long	293342507
	.long	436802868
	@ Y[18]
	.long	2107
	.long	4510
	.long	404
	.long	4277
	.long	6773
	.long	5795
	.long	7232
	.long	5452
	.size	curve9767_inner_window_G192, .-curve9767_inner_window_G192
