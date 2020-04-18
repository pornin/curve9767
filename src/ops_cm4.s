@ =======================================================================
@ WARNING: all "costs" indicated for macros and functions are based on
@ manual instruction counting, and the following assumptions, which hold
@ for the ARM Cortex-M0+, but not necessarily for the ARM Cortex-M4:
@  - Branches take two cycles when taken (three cycles for 'bl').
@  - Each load or store of N words takes N+1 cycles.
@
@ Reality is different on the ARM Cortex-M4:
@  - Pipeline is deeper, making branches slightly more expensive. However,
@    the CPU has some limited branch prediction abilities and may avoid
@    that extra cost in some cases.
@  - Some tests seem to indicate that loads of N words take N+2 cycles,
@    not N+1 cycles (but stores use N+1 cycles).
@  - A contrario, some code sequences seem to lead to instruction pairing,
@    allowing for instance a write to execute partially in the background.
@  - Data reads and writes may be stalled when competing with instruction
@    fetching. The CPU core has separate buses for data I/O and
@    instructions, but the core is integrated in a microcontroller that
@    uses a memory controller that may induce such stalling.
@
@ On a STM32F407 board, clocked at 24 MHz ("zero wait state" for Flash
@ access), actual function costs, when measured, seem slightly higher than
@ the values predicted through instruction counting; e.g. gf_mul_inner()
@ was predicted at 609 cycles, but uses 628 cycles in practice.
@ =======================================================================

	.syntax	unified
	.cpu	cortex-m4
	.file	"ops_cm4.s"
	.text

@ =======================================================================

@ Perform Montgomery reduction.
@   rx     value to reduce (input/output)
@   rt     scratch register
@   rp     contains p = 9767 (unmodified)
@   rp1i   contains p1i = 3635353193 (unmodified)
@ This function works for all input values (full unsigned 32-bit range).
@ Cost: 2
.macro MONTYRED  rx, rt, rp, rp1i
	mul	\rt, \rx, \rp1i
	umaal	\rp1i, \rx, \rt, \rp
.endm

@ Perform two Montgomery reductions with packed results.
@ Values in registers rx and ry are reduced, and the results packed into
@ rd (reduced rx is the low half). All registers must be distinct.
@   rd     destination register
@   rx     first source value (consumed)
@   ry     second source value (consumed)
@   rp     contains p = 9767 (unmodified)
@   rp1i   contains p1i = 3635353193 (unmodified)
@ This function works for all input values (full unsigned 32-bit range).
@ Cost: 5
.macro MONTYRED_X2  rd, rx, ry, rp, rp1i
	MONTYRED  \rx, \rd, \rp, \rp1i
	MONTYRED  \ry, \rd, \rp, \rp1i
	pkhbt	\rd, \rx, \ry, lsl #16
.endm

@ Perform two Montgomery reductions with packed results, alternate code.
@ This is slower than MONTYRED_X2, but it does not need an extra register
@ for the output; instead, the result is written over rx.
@   rx     first source value, and destination register.
@   ry     second source value (consumed)
@   rz     arbitrary value (unmodified)
@   rp     contains p = 9767 (unmodified)
@   rp1i   contains p1i = 3635353193 (unmodified)
@ Input values x and y must be between 1 and 3654952486 (inclusive).
@ Cost: 8
.macro MONTYRED_X2_ALT  rx, ry, rp, rp1i
	mul	\rx, \rx, \rp1i
	lsrs	\rx, #16
	mul	\rx, \rx, \rp
	mul	\ry, \ry, \rp1i
	lsrs	\ry, #16
	mul	\ry, \ry, \rp
	pkhtb	\rx, \ry, \rx, asr #16
	adds	\rx, \rx, #0x00010001
.endm

@ Perform normal reduction (not Montgomery).
@   rx     register to reduce (within r0..r7)
@   rt     scratch register
@   rp     contains p = 9767 (unmodified)
@   rp2i   contains p2i = 439742 (unmodified)
@ Input value x must be between 1 and 509232 (inclusive); output
@ value is equal to x mod 9767, and is between 1 and 9767 (inclusive).
@ Cost: 3
.macro BASERED  rx, rt, rp, rp2i
	mul	\rx, \rx, \rp2i
	umull	\rt, \rx, \rx, \rp
	adds	\rx, #1
.endm

@ Perform normal reduction on two elements (not Montgomery, packed).
@   rx     values to reduce
@   rt     scratch register
@   ru     scratch register
@   rp     contains p = 9767 (unmodified)
@   rp2i   contains p2i = 439742 (unmodified)
@ Cost: 8
.macro BASERED_X2  rx, rt, ru, rp, rp2i
	uxth	\rt, \rx
	lsrs	\ru, \rx, #16
	mul	\rt, \rt, \rp2i
	mul	\ru, \ru, \rp2i
	umull	\rx, \rt, \rt, \rp
	umull	\rx, \ru, \ru, \rp
	pkhbt	\rx, \rt, \ru, lsl #16
	adds	\rx, #0x00010001
.endm

@ Montgomery multiplication:
@   rx <- (rx*ry)/(2^32) mod p  (in the 1..p range)
@ Input:
@   rx     first operand, and output register
@   ry     second operand (unmodified)
@   rt     scratch register
@   rp     constant p = 9767
@   rp1i   constant p1i = 3635353193
@ Cost: 3
.macro MONTYMUL  rx, ry, rt, rp, rp1i
	muls	\rx, \ry
	MONTYRED  \rx, \rt, \rp, \rp1i
.endm

@ Let A = a0..a9; pointer to a0 is in r0.
@ Let B = b0..b9; pointer to b0 is in r1.
@ This macro computes A*B, and stores it in the stack buffer at index kc.
@ kt is the index of a free stack slot.
@ If trunc is non-zero, then the top word is not computed or written, i.e.
@ only 18 words of output are produced.
@ Upon exit, r1 is unchanged; r0 is consumed.
@ Cost: 108 (105 if trunc)
.macro MUL_SET_10x10  kc, kt, trunc
	@ Load a0..a9 into registers r2, r3, r4, r5, r6
	ldm	r0!, { r2, r3, r4, r5, r6 }

	@ Load b0..b5 into registers r8, r10, r11, r12, r14
	ldm	r1, { r8, r10, r11, r12, r14 }

	@ a0*b0
	@ a0*b1+a1*b0
	smulbb	r0, r2, r8
	smuadx	r7, r2, r8
	strd	r0, r7, [sp, #(4 * (\kc))]

	@ a0*b3+a1*b2+a2*b1+a3*b0
	smuadx	r0, r2, r10
	smladx	r0, r3, r8, r0
	str	r0, [sp, #(4 * ((\kc) + 3))]

	@ a0*b5+a1*b4+a2*b3+a3*b2+a4*b1+a5*b0
	smuadx	r0, r2, r11
	smladx	r0, r3, r10, r0
	smladx	r0, r4, r8, r0
	str	r0, [sp, #(4 * ((\kc) + 5))]

	@ a0*b7+a1*b6+a2*b5+a3*b4+a4*b3+a5*b2+a6*b1+a7*b0
	smuadx	r0, r2, r12
	smladx	r0, r3, r11, r0
	smladx	r0, r4, r10, r0
	smladx	r0, r5, r8, r0
	str	r0, [sp, #(4 * ((\kc) + 7))]

	@ a0*b9+a1*b8+a2*b7+a3*b6+a4*b5+a5*b4+a6*b3+a7*b2+a8*b1+a9*b0
	smuadx	r0, r2, r14
	smladx	r0, r3, r12, r0
	smladx	r0, r4, r11, r0
	smladx	r0, r5, r10, r0
	smladx	r0, r6, r8, r0
	str	r0, [sp, #(4 * ((\kc) + 9))]

	@ a2*b9+a3*b8+a4*b7+a5*b6+a6*b5+a7*b4+a8*b3+a9*b2
	smuadx	r0, r3, r14
	smladx	r0, r4, r12, r0
	smladx	r0, r5, r11, r0
	smladx	r0, r6, r10, r0
	str	r0, [sp, #(4 * ((\kc) + 11))]

	@ a4*b9+a5*b8+a6*b7+a7*b6+a8*b5+a9*b4
	smuadx	r0, r4, r14
	smladx	r0, r5, r12, r0
	smladx	r0, r6, r11, r0
	str	r0, [sp, #(4 * ((\kc) + 13))]

	@ a6*b9+a7*b8+a8*b7+a9*b6
	smuadx	r0, r5, r14
	smladx	r0, r6, r12, r0
	str	r0, [sp, #(4 * ((\kc) + 15))]

	@ a8*b9+a9*b8
	smuadx	r0, r6, r14
	str	r0, [sp, #(4 * ((\kc) + 17))]

	@ Reorganize words of a.
	@   r2 = a0:a1  (unchanged)
	@   r3 = a2:a1
	@   r4 = a4:a3
	@   r5 = a6:a5
	@   r6 = a8:a9  (unchanged)
	@   r7 = a8:a7
	pkhbt	r7, r6, r5
	pkhbt	r5, r5, r4
	pkhbt	r4, r4, r3
	pkhbt	r3, r3, r2

	@ a0*b2+a1*b1+a2*b0
	smulbb	r0, r2, r10
	smlad	r0, r3, r8, r0
	str	r0, [sp, #(4 * ((\kc) + 2))]

	@ a0*b4+a1*b3+a2*b2+a3*b1+a4*b0
	smulbb	r0, r2, r11
	smlad	r0, r3, r10, r0
	smlad	r0, r4, r8, r0
	str	r0, [sp, #(4 * ((\kc) + 4))]

	@ a0*b6+a1*b5+a2*b4+a3*b3+a4*b2+a5*b1+a6*b0
	smulbb	r0, r2, r12
	smlad	r0, r3, r11, r0
	smlad	r0, r4, r10, r0
	smlad	r0, r5, r8, r0
	str	r0, [sp, #(4 * ((\kc) + 6))]

	@ a0*b8+a1*b7+a2*b6+a3*b5+a4*b4+a5*b3+a6*b2+a7*b1+a8*b0
	smulbb	r0, r2, r14
	smlad	r0, r3, r12, r0
	smlad	r0, r4, r11, r0
	smlad	r0, r5, r10, r0
	smlad	r0, r7, r8, r0
	str	r0, [sp, #(4 * ((\kc) + 8))]

	@ a1*b9+a2*b8+a3*b7+a4*b6+a5*b5+a6*b4+a7*b3+a8*b2+a9*b1
	smuad	r0, r3, r14
	smlad	r0, r4, r12, r0
	smlad	r0, r5, r11, r0
	smlad	r0, r7, r10, r0
	smlatt	r0, r6, r8, r0
	str	r0, [sp, #(4 * ((\kc) + 10))]

	@ a3*b9+a4*b8+a5*b7+a6*b6+a7*b5+a8*b4+a9*b3
	smuad	r0, r4, r14
	smlad	r0, r5, r12, r0
	smlad	r0, r7, r11, r0
	smlatt	r0, r6, r10, r0
	str	r0, [sp, #(4 * ((\kc) + 12))]

	@ a5*b9+a6*b8+a7*b7+a8*b6+a9*b5
	smuad	r0, r5, r14
	smlad	r0, r7, r12, r0
	smlatt	r0, r6, r11, r0
	str	r0, [sp, #(4 * ((\kc) + 14))]

	@ a7*b9+a8*b8+a9*b7
	smuad	r0, r7, r14
	smlatt	r0, r6, r12, r0
	str	r0, [sp, #(4 * ((\kc) + 16))]

	.if (\trunc) == 0
	@ a9*b9
	smultt	r0, r6, r14
	str	r0, [sp, #(4 * ((\kc) + 18))]
	.endif
.endm

@ Let A = a0..a8; pointer to a0 is in r0.
@ Let B = b0..b8; pointer to b0 is in r1.
@ This macro computes A*B, and stores it in the stack buffer at index kc.
@ kt is the index of a free stack slot.
@ Upon exit, r1 is unchanged; r0 is consumed.
@ Cost: 98
.macro MUL_SET_9x9  kc, kt
	@ Load a0..a8 into registers r2, r3, r4, r5, r6
	ldm	r0!, { r2, r3, r4, r5, r6 }

	@ Load b0..b8 into registers r8, r10, r11, r12, r14
	ldm	r1, { r8, r10, r11, r12, r14 }

	@ a0*b0
	@ a0*b1+a1*b0
	smulbb	r0, r2, r8
	smuadx	r7, r2, r8
	strd	r0, r7, [sp, #(4 * (\kc))]

	@ a0*b3+a1*b2+a2*b1+a3*b0
	smuadx	r0, r2, r10
	smladx	r0, r3, r8, r0
	str	r0, [sp, #(4 * ((\kc) + 3))]

	@ a0*b5+a1*b4+a2*b3+a3*b2+a4*b1+a5*b0
	smuadx	r0, r2, r11
	smladx	r0, r3, r10, r0
	smladx	r0, r4, r8, r0
	str	r0, [sp, #(4 * ((\kc) + 5))]

	@ a0*b7+a1*b6+a2*b5+a3*b4+a4*b3+a5*b2+a6*b1+a7*b0
	smuadx	r0, r2, r12
	smladx	r0, r3, r11, r0
	smladx	r0, r4, r10, r0
	smladx	r0, r5, r8, r0
	str	r0, [sp, #(4 * ((\kc) + 7))]

	@ a1*b8+a2*b7+a3*b6+a4*b5+a5*b4+a6*b3+a7*b2+a8*b1
	smultb	r0, r2, r14
	smladx	r0, r3, r12, r0
	smladx	r0, r4, r11, r0
	smladx	r0, r5, r10, r0
	smlabt	r0, r6, r8, r0
	str	r0, [sp, #(4 * ((\kc) + 9))]

	@ a3*b8+a4*b7+a5*b6+a6*b5+a7*b4+a8*b3
	smultb	r0, r3, r14
	smladx	r0, r4, r12, r0
	smladx	r0, r5, r11, r0
	smlabt	r0, r6, r10, r0
	str	r0, [sp, #(4 * ((\kc) + 11))]

	@ a5*b8+a6*b7+a7*b6+a8*b5
	smultb	r0, r4, r14
	smladx	r0, r5, r12, r0
	smlabt	r0, r6, r11, r0
	str	r0, [sp, #(4 * ((\kc) + 13))]

	@ a7*b8+a8*b7
	smultb	r0, r5, r14
	smlabt	r0, r6, r12, r0
	str	r0, [sp, #(4 * ((\kc) + 15))]

	@ Reorganize words of a.
	@   r2 = a0:a1  (unchanged)
	@   r3 = a2:a1
	@   r4 = a4:a3
	@   r5 = a6:a5
	@   r6 = a8:a7
	pkhbt	r6, r6, r5
	pkhbt	r5, r5, r4
	pkhbt	r4, r4, r3
	pkhbt	r3, r3, r2

	@ a0*b2+a1*b1+a2*b0
	smulbb	r0, r2, r10
	smlad	r0, r3, r8, r0
	str	r0, [sp, #(4 * ((\kc) + 2))]

	@ a0*b4+a1*b3+a2*b2+a3*b1+a4*b0
	smulbb	r0, r2, r11
	smlad	r0, r3, r10, r0
	smlad	r0, r4, r8, r0
	str	r0, [sp, #(4 * ((\kc) + 4))]

	@ a0*b6+a1*b5+a2*b4+a3*b3+a4*b2+a5*b1+a6*b0
	smulbb	r0, r2, r12
	smlad	r0, r3, r11, r0
	smlad	r0, r4, r10, r0
	smlad	r0, r5, r8, r0
	str	r0, [sp, #(4 * ((\kc) + 6))]

	@ a0*b8+a1*b7+a2*b6+a3*b5+a4*b4+a5*b3+a6*b2+a7*b1+a8*b0
	smulbb	r0, r2, r14
	smlad	r0, r3, r12, r0
	smlad	r0, r4, r11, r0
	smlad	r0, r5, r10, r0
	smlad	r0, r6, r8, r0
	str	r0, [sp, #(4 * ((\kc) + 8))]

	@ a2*b8+a3*b7+a4*b6+a5*b5+a6*b4+a7*b3+a8*b2
	smulbb	r0, r3, r14
	smlad	r0, r4, r12, r0
	smlad	r0, r5, r11, r0
	smlad	r0, r6, r10, r0
	str	r0, [sp, #(4 * ((\kc) + 10))]

	@ a4*b8+a5*b7+a6*b6+a7*b5+a8*b4
	smulbb	r0, r4, r14
	smlad	r0, r5, r12, r0
	smlad	r0, r6, r11, r0
	str	r0, [sp, #(4 * ((\kc) + 12))]

	@ a6*b8+a7*b7+a8*b6
	smulbb	r0, r5, r14
	smlad	r0, r6, r12, r0
	str	r0, [sp, #(4 * ((\kc) + 14))]

	@ a8*b8
	smulbb	r0, r6, r14
	str	r0, [sp, #(4 * ((\kc) + 16))]
.endm

@ Karatsuba fix up, producing output values at indices i, i+1, i-9 and i-8
@ (with 9 <= i <= 17, i = 1 mod 2; when i = 17, out[9] is not produced).
@ Expectations:
@   r0   pointer to the output buffer
@   r1   (A0*B0)[i - 10]  (ignored if i == 9)
@   r3   (A1*B1)[i - 1]
@   r6   constant p = 9767 (unmodified)
@   r7   constant p1i = 3635353193 (unmodified)
@   rk   input register for out[i-1] (unreduced, ignored if i == 9)
@   rn   output register for out[i+1] (unreduced, not produced if i == 17)
@   ka   index for alpha = A0*B0
@   kb   index for beta = A1*B1
@   kg   index for gamma = (A0+A1)*(B0+B1)
@ Registers rk and rn must be r8 and r11 (or r11 and r8).
@ On output, r1 contains (A0*B0)[i-8], r3 contains (A1*B1)[i+1], and r8
@ contains out[i+1].
@ Cost: depends on i:
@    9   39
@   11   48
@   13   48
@   15   48
@   17   38
.macro STEP_KFIX_X4  rk, rn, ka, kb, kg, i
	@ If we note the following (with indices in stack):
	@  ka   alpha  = A0*B0  (19 words)
	@  ka   alpha' = alpha without the last word  (18 words)
	@  kb   beta   = A1*B1  (17 words)
	@  kg   gamma  = (A0+A1)*(B0+B1)  (18 words)
	@ Then:
	@  out[i] = alpha[i] + gamma[i - 10] - alpha'[i - 10] - beta[i - 10]
	@         + 2*(beta[i - 1] + gamma[i + 9] - alpha'[i + 9] - beta[i + 9])
	@ With the following rule:
	@  xxx[i] is zero outside of the range for that array

	@ We assemble the four output values:
	@   i     r10
	@   i+1   \rn
	@   i-9   r12
	@   i-8   r14

	@ Load alpha[i] (r4) and alpha[i+1] (r5).
	ldrd	r4, r5, [sp, #(4 * ((\ka) + (\i)))]

	.if (\i) == 9

	@ Load gamma[i-9], then add it to alpha[i+1].
	ldr	\rn, [sp, #(4 * ((\kg) + (\i) - 9))]
	add	\rn, r5

	@ Load beta[i-9].
	ldr	r14, [sp, #(4 * ((\kb) + (\i) - 9))]

	@ Subtract beta[i-9].
	sub	\rn, r14

	@ Load gamma[i] and gamma[i+1].
	ldrd	r12, r1, [sp, #(4 * ((\kg) + (\i)))]
	add	r14, r1

	@ Subtract alpha[i] and alpha[i+1].
	sub	r12, r4
	sub	r14, r5

	@ Set out[i] = alpha[i] + 2*beta[i-1] (valid for i = 9).
	add	r10, r4, r3, lsl #1

	@ Load beta[i] and beta[i+1]. beta[i+1] goes to r3 (output).
	ldrd	r4, r3, [sp, #(4 * ((\kb) + (\i)))]

	@ Add 2*beta[i].
	add	\rn, \rn, r4, lsl #1

	@ Subtract beta[i] and beta[i + 1].
	sub	r12, r4
	sub	r14, r3

	@ Load alpha[i-9] and alpha[i-8]. alpha[i-8] goes to r1 (output).
	ldrd	r4, r1, [sp, #(4 * ((\ka) + (\i) - 9))]

	@ Subtract alpha[i-9].
	sub	\rn, r4

	@ Add alpha[i-9] and alpha[i-8].
	add	r12, r4, r12, lsl #1
	add	r14, r1, r14, lsl #1

	.elseif (\i) < 17

	@ Load gamma[i-10] and gamma[i-9], then add them to alpha[i]
	@ and alpha[i+1].
	ldrd	r10, \rn, [sp, #(4 * ((\kg) + (\i) - 10))]
	add	r10, r4
	add	\rn, r5

	@ Subtract alpha[i-10].
	sub	r10, r1

	@ Add 2*beta[i-1].
	add	r10, r10, r3, lsl #1

	@ Load beta[i-10] and beta[i-9].
	ldrd	r12, r14, [sp, #(4 * ((\kb) + (\i) - 10))]

	@ Subtract beta[i-10] and beta[i-9].
	sub	r10, r12
	sub	\rn, r14

	@ Subtract alpha[i] and alpha[i+1].
	sub	r12, r4
	sub	r14, r5

	@ Load gamma[i] and gamma[i+1] and add them.
	ldrd	r4, r5, [sp, #(4 * ((\kg) + (\i)))]
	add	r12, r4
	add	r14, r5

	@ Load beta[i] and beta[i+1]. beta[i+1] goes to r3 (output).
	ldrd	r4, r3, [sp, #(4 * ((\kb) + (\i)))]

	@ Add 2*beta[i].
	add	\rn, \rn, r4, lsl #1

	@ Subtract beta[i] and beta[i + 1].
	sub	r12, r4
	sub	r14, r3

	@ Load alpha[i-9] and alpha[i-8]. alpha[i-8] goes to r1 (output).
	ldrd	r4, r1, [sp, #(4 * ((\ka) + (\i) - 9))]

	@ Subtract alpha[i-9].
	sub	\rn, r4

	@ Add alpha[i-9] and alpha[i-8].
	add	r12, r4, r12, lsl #1
	add	r14, r1, r14, lsl #1

	.else

	@ Load gamma[i-10] and gamma[i-9], then add them to alpha[i]
	@ and alpha[i+1].
	ldrd	r10, \rn, [sp, #(4 * ((\kg) + (\i) - 10))]
	add	r10, r4
	add	\rn, r5

	@ Subtract alpha[i-10].
	sub	r10, r1

	@ Add 2*beta[i-1].
	add	r10, r10, r3, lsl #1

	@ Load beta[i-10] and beta[i-9].
	ldrd	r12, r14, [sp, #(4 * ((\kb) + (\i) - 10))]

	@ Subtract beta[i-10] and beta[i-9].
	sub	r10, r12
	sub	\rn, r14

	@ Subtract alpha[i] and alpha[i+1].
	sub	r12, r4

	@ Load gamma[i] and gamma[i+1] and add them.
	ldr	r4, [sp, #(4 * ((\kg) + (\i)))]
	add	r12, r4

	@ Load alpha[i-9].
	ldr	r4, [sp, #(4 * ((\ka) + (\i) - 9))]

	@ Subtract alpha[i-9].
	sub	\rn, r4

	@ Add alpha[i-9] and alpha[i-8].
	add	r12, r4, r12, lsl #1

	.endif

	@ Apply Montgomery reduction on all output words.

	@ Since i is odd, r10 and \rn (out[i] and out[i+1]) must be
	@ written as two separate 16-bit values, to avoid an unaligned
	@ access. We get out[i-1] from \rk (except if i == 9).
	.if (\i) == 9
	MONTYRED  r10, r4, r6, r7
	strh	r10, [r0, #(2 * ((\i) + 0))]
	.else
	MONTYRED_X2  r4, \rk, r10, r6, r7
	str	r4, [r0, #(2 * ((\i) - 1))]
	.endif

	@ If i == 17, then we are at the end of the array, and we must
	@ reduce the last half-word alone.
	.if (\i) == 17
	MONTYRED  \rn, r4, r6, r7
	strh	\rn, [r0, #(2 * ((\i) + 1))]
	.endif

	@ r12 and r14 are for out[i-9] and out[i-8]. This is 32-bit
	@ aligned (because i is odd); but when i = 17, r14 is missing.
	.if (\i) < 17
	MONTYRED_X2  r4, r12, r14, r6, r7
	str	r4, [r0, #(2 * ((\i) - 9))]
	.else
	MONTYRED  r12, r4, r6, r7
	strh	r12, [r0, #(2 * ((\i) - 9))]
	.endif
.endm

@ Perform Karastuba fix-up and Montgomery reduction.
@   ka   index for stack buffer for alpha = A0*B0
@   kb   index for stack buffer for beta = A1*B1
@   kg   index for stack buffer for gamma = (A0+A1)*(B0+B1)
@ Expectations:
@   r0   pointer to output buffer (unmodified)
@   r6   constant p = 9767 (unmodified)
@   r7   constant p1i = 3635353193 (unmodified)
@ Cost: 223
.macro KFIX  ka, kb, kg
	ldr	r3, [sp, #(4 * ((\kb) + 8))]
	STEP_KFIX_X4  r8, r11, (\ka), (\kb), (\kg),  9
	STEP_KFIX_X4  r11, r8, (\ka), (\kb), (\kg), 11
	STEP_KFIX_X4  r8, r11, (\ka), (\kb), (\kg), 13
	STEP_KFIX_X4  r11, r8, (\ka), (\kb), (\kg), 15
	STEP_KFIX_X4  r8, r11, (\ka), (\kb), (\kg), 17
.endm

@ Perform reduction after addition.
@   rx   value to reduce, receives the result
@   rt   scratch register
@   rp   register containing p = 9767 (unmodified)
@ Cost: 2
.macro MPMOD_AFTER_ADD  rx, rt, rp
	subs	\rt, \rp, \rx
	smlatb	\rx, \rt, \rp, \rx
.endm

@ Processing six elements of each operand in gf_add_inner()
@   r0    pointer to output array (updated)
@   r1    pointer to first input array (updated)
@   r2    pointer to second input array (updated)
@   r10   constant 0x00010001 (unmodified)
@   r11   constant 9767 + (9767 << 16) (unmodified)
@   r12   constant 9767 (unmodified)
@ Register r14 is unmodified.
@ Cost: 24
.macro STEP_ADD_X6
	ldm	r1!, { r3, r4, r5 }
	ldm	r2!, { r6, r7, r8 }
	adds	r3, r6
	adds	r4, r7
	adds	r5, r8
	ssub16	r6, r11, r3
	ssub16	r7, r11, r4
	ssub16	r8, r11, r5
	ands	r6, r10, r6, lsr #15
	ands	r7, r10, r7, lsr #15
	ands	r8, r10, r8, lsr #15
	mls	r3, r6, r12, r3
	mls	r4, r7, r12, r4
	mls	r5, r8, r12, r5
	stm	r0!, { r3, r4, r5 }
.endm

@ Perform reduction after subtraction.
@   rx    value to reduce, receives the result
@   rt    scratch register
@   rmp   register containing -p = -9767 (unmodified)
@ Cost: 2
.macro MPMOD_AFTER_SUB  rx, rt, rp
	subs	\rt, \rx, #1
	smlatb	\rx, \rt, \rp, \rx
.endm

@ Processing six elements of each operand in gf_sub_inner()
@   r0    pointer to output array (updated)
@   r1    pointer to first input array (updated)
@   r2    pointer to second input array (updated)
@   r10   constant 0x00010001 (unmodified)
@   r12   constant 9767 (unmodified)
@ Register r14 is unmodified.
@ Cost: 24
.macro STEP_SUB_X6
	ldm	r1!, { r3, r4, r5 }
	ldm	r2!, { r6, r7, r8 }

	subs	r11, r3, r6
	ssub16	r6, r6, r3
	bics	r6, r10, r6, lsr #15
	mla	r3, r6, r12, r11

	subs	r11, r4, r7
	ssub16	r7, r7, r4
	bics	r7, r10, r7, lsr #15
	mla	r4, r7, r12, r11

	subs	r11, r5, r8
	ssub16	r8, r8, r5
	bics	r8, r10, r8, lsr #15
	mla	r5, r8, r12, r11

	stm	r0!, { r3, r4, r5 }
.endm

@ Negate two elements (packed).
@   rx   input elements
@   ry   output elements
@   rt   scratch register
@   rp   constant 9767 + (9767 << 16) (unmodified)
@ If rp == 0, then rx is copied into ry (no negation).
@ IMPORTANT: all registers must be distinct.
@ Cost: 3
.macro STEP_NEG_X2  ry, rx, rt, rp
	usub16	\rt, \rx, \rp
	sub	\ry, \rp, \rx
	sel	\ry, \rx, \ry
.endm

@ Process six elements of x1, x2 and y1 in computation of lambda for
@ curve point addition (denominator).
@   r0    pointer to output (updated)
@   r1    pointer to x1 (updated)
@   r2    pointer to x2 (updated)
@   r3    pointer to y1 (updated)
@   r11   value -2*ex (-2 if x1 == x2, 0 otherwise) (unmodified)
@   r12   constant 0x00010001 (unmodified)
@   r14   constant p + (p << 16) (unmodified)
@ Cost: 38
.macro STEP_LAMBDA_D_X6
	ldm	r2!, { r4, r5, r6 }
	ldm	r1!, { r7, r8, r10 }
	subs	r4, r7
	subs	r5, r8
	subs	r6, r10
	adds	r4, r14
	adds	r5, r14
	adds	r6, r14
	bics	r4, r4, r11, asr #1
	bics	r5, r5, r11, asr #1
	bics	r6, r6, r11, asr #1
	ldm	r3!, { r7, r8, r10 }
	mls	r4, r7, r11, r4
	mls	r5, r8, r11, r5
	mls	r6, r10, r11, r6
	uxth	r10, r14
	ssub16	r7, r14, r4
	ands	r7, r12, r7, lsr #15
	mls	r4, r7, r10, r4
	ssub16	r7, r14, r5
	ands	r7, r12, r7, lsr #15
	mls	r5, r7, r10, r5
	ssub16	r7, r14, r6
	ands	r7, r12, r7, lsr #15
	mls	r6, r7, r10, r6
	stm	r0!, { r4, r5, r6 }
.endm

@ Process six elements of y1, y2 and x1^2 in computation of lambda for
@ curve point addition (numerator).
@   r0    pointer to output (updated)
@   r1    pointer to y1 (updated)
@   r2    pointer to y2 (updated)
@   r3    pointer to x1^2 (updated)
@   r11   value -ex (-1 if x1 == x2, 0 otherwise) (unmodified)
@   r12   constant p2i = 439742 (unmodified)
@   r14   constant p + (p << 16) (unmodified)
@   i     current index (0, 6 or 12)
@ Cost: 56 (58 if i == 0)
.macro STEP_LAMBDA_N_X6  i
	ldm	r2!, { r4, r5, r6 }
	ldm	r1!, { r7, r8, r10 }
	subs	r4, r7
	subs	r5, r8
	subs	r6, r10
	adds	r4, r14
	adds	r5, r14
	adds	r6, r14
	ldm	r3!, { r7, r8, r10 }
	adds	r7, r7, r7, lsl #1
	adds	r8, r8, r8, lsl #1
	adds	r10, r10, r10, lsl #1
	.if (\i) == 0
	adds	r7, #0x4B
	adds	r7, #0x1E00
	.endif
	bics	r4, r4, r11
	bics	r5, r5, r11
	bics	r6, r6, r11
	mls	r4, r7, r11, r4
	mls	r5, r8, r11, r5
	mls	r6, r10, r11, r6
	uxth	r10, r14
	BASERED_X2  r4, r7, r8, r10, r12
	BASERED_X2  r5, r7, r8, r10, r12
	BASERED_X2  r6, r7, r8, r10, r12
	stm	r0!, { r4, r5, r6 }
.endm

@ Assemble a value (six 16-bit elements) from three sources.
@   r0    pointer to output (updated)
@   r1    pointer to first value (updated)
@   r2    pointer to second value (updated)
@   r3    pointer to third value (updated)
@   r11   -1 if first value is kept, 0 otherwise
@   r12   -1 if second value is kept, 0 otherwise
@   r14   -1 if third value is kept, 0 otherwise
@ If nw1 is 1, then the high half of the first output word is not written
@ out (this is the 2-byte gap between the coordinates).
@ Cost: 25 (26 if nw1 != 0)
.macro STEP_SELECT3_X6  nw1
	ldm	r1!, { r4, r5, r6 }
	ands	r4, r4, r11
	ands	r5, r5, r11
	ands	r6, r6, r11
	ldm	r2!, { r7, r8, r10 }
	mls	r4, r7, r12, r4
	mls	r5, r8, r12, r5
	mls	r6, r10, r12, r6
	ldm	r3!, { r7, r8, r10 }
	mls	r4, r7, r14, r4
	mls	r5, r8, r14, r5
	mls	r6, r10, r14, r6
	.if (\nw1) != 0
	strh	r4, [r0]
	strd	r5, r6, [r0, #4]
	adds	r0, #12
	.else
	stm	r0!, { r4, r5, r6 }
	.endif
.endm

@ Double two elements (packed) with modular reduction.
@   rx    value to double
@   rt    scratch register
@   rm    constant 0x00010001 (unmodified)
@   rp    constant 9767 (unmodified)
@   rpp   constant 9767 + (9767 << 16) (unmodified)
@ Cost: 4
.macro MP_MUL2_X2  rx, rt, rm, rp, rpp
	lsls	\rt, \rx, #1
	ssub16	\rx, \rpp, \rt
	ands	\rx, \rm, \rx, lsr #15
	mls	\rx, \rp, \rx, \rt
.endm

@ Compute rx - 8 * ry, result in rx (reduced, packed).
@   rx     first operand, and output register
@   ry     second operand (unmodified)
@   rt     scratch register
@   ru     scratch register
@   rp     constant p = 9767 (unmodified)
@   rp2i   constant p2i = 439742 (unmodified)
@ Cost: 15
.macro MP_SUB8_X2  rx, ry, rt, ru, rp, rp2i
	lsrs	\rt, \rx, #16
	lsrs	\ru, \ry, #16
	subs	\rt, \rt, \ru, lsl #3
	adds	\rt, \rt, \rp, lsl #3
	BASERED  \rt, \ru, \rp, \rp2i
	uxth	\rx, \rx
	uxth	\ru, \ry
	subs	\rx, \rx, \ru, lsl #3
	adds	\rx, \rx, \rp, lsl #3
	BASERED  \rx, \ru, \rp, \rp2i
	pkhbt	\rx, \rx, \rt, lsl #16
.endm

@ One iteration of a window lookup. This assembles two elements of a
@ coordinate of the result point.
@   r0   output pointer (updated)
@   r1   input pointer (updated)
@ The eight masks are in r5, r6, r7, r8, r10, r11, r12 and r14.
@ Cost: 22
.macro STEP_WINDOW_LOOKUP
	ldm	r1!, { r2, r3, r4 }
	ands	r2, r5
	mls	r2, r3, r6, r2
	mls	r2, r4, r7, r2
	ldm	r1!, { r3, r4 }
	mls	r2, r3, r8, r2
	mls	r2, r4, r10, r2
	ldm	r1!, { r3, r4 }
	mls	r2, r3, r11, r2
	mls	r2, r4, r12, r2
	ldm	r1!, { r3 }
	mls	r2, r3, r14, r2
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
@ the ABI, but it is usable as a non-global function.
@
@ Cost: 86
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_add_inner, %function
gf_add_inner:
	mov	r10, #0x00010001
	movw	r12, #9767
	orrs	r11, r12, r12, lsl #16
	STEP_ADD_X6
	STEP_ADD_X6
	STEP_ADD_X6
	ldrh	r3, [r1]
	ldrh	r4, [r2]
	adds	r3, r4
	MPMOD_AFTER_ADD  r3, r5, r12
	strh	r3, [r0]
	bx	lr
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
	push	{ r4, r5, r6, r7, r8, r10, r11, lr }
	bl	gf_add_inner
	pop	{ r4, r5, r6, r7, r8, r10, r11, pc }
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
@ Cost: 86
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_sub_inner, %function
gf_sub_inner:
	mov	r10, #0x00010001
	movw	r12, #9767
	STEP_SUB_X6
	STEP_SUB_X6
	STEP_SUB_X6
	ldrh	r3, [r1]
	ldrh	r4, [r2]
	subs	r3, r4
	rsbs	r12, r12, #0
	MPMOD_AFTER_SUB  r3, r5, r12
	strh	r3, [r0]
	bx	lr
	.size	gf_sub_inner, .-gf_sub_inner

@ =======================================================================
@ void curve9767_inner_gf_sub(uint16_t *c,
@                             const uint16_t *a, const uint16_t *b)
@
@ Public wrapper for gf_sub_inner() (conforms to the ABI).
@ =======================================================================

	.align	1
	.global	curve9767_inner_gf_sub
	.thumb
	.thumb_func
	.type	curve9767_inner_gf_sub, %function
curve9767_inner_gf_sub:
	push	{ r4, r5, r6, r7, r8, r10, r11, lr }
	bl	gf_sub_inner
	pop	{ r4, r5, r6, r7, r8, r10, r11, pc }
	.size	curve9767_inner_gf_sub, .-curve9767_inner_gf_sub

@ =======================================================================
@ void curve9767_inner_gf_neg(uint16_t *c, const uint16_t *a)
@
@ Source/destination arrays are guaranteed to be 32-bit aligned.
@
@ Cost: 71
@ =======================================================================

	.align	1
	.global	curve9767_inner_gf_neg
	.thumb
	.thumb_func
	.type	curve9767_inner_gf_neg, %function
curve9767_inner_gf_neg:
	push	{ r4, r5, r6, r7, r8 }
	movw	r12, #9767
	orrs	r12, r12, r12, lsl #16
	ldm	r1!, { r2, r3, r4, r5, r6 }
	STEP_NEG_X2  r7, r6, r8, r12
	STEP_NEG_X2  r6, r5, r8, r12
	STEP_NEG_X2  r5, r4, r8, r12
	STEP_NEG_X2  r4, r3, r8, r12
	STEP_NEG_X2  r3, r2, r8, r12
	stm	r0!, { r3, r4, r5, r6, r7 }
	ldm	r1!, { r2, r3, r4, r5, r6 }
	STEP_NEG_X2  r7, r6, r8, r12
	STEP_NEG_X2  r6, r5, r8, r12
	STEP_NEG_X2  r5, r4, r8, r12
	STEP_NEG_X2  r4, r3, r8, r12
	STEP_NEG_X2  r3, r2, r8, r12
	stm	r0!, { r3, r4, r5, r6 }
	strh	r7, [r0]
	pop	{ r4, r5, r6, r7, r8 }
	bx	lr
	.size	curve9767_inner_gf_neg, .-curve9767_inner_gf_neg

@ =======================================================================
@ void curve9767_inner_gf_condneg(uint16_t *c, uint32_t ctl)
@
@ Source/destination arrays are guaranteed to be 32-bit aligned.
@
@ Cost: 73
@ =======================================================================

	.align	1
	.global	curve9767_inner_gf_condneg
	.thumb
	.thumb_func
	.type	curve9767_inner_gf_condneg, %function
curve9767_inner_gf_condneg:
	push	{ r4, r5, r6, r7, r8 }
	movw	r12, #9767
	orrs	r12, r12, r12, lsl #16
	mul	r12, r12, r1
	movs	r1, r0
	ldm	r1!, { r2, r3, r4, r5, r6 }
	STEP_NEG_X2  r7, r6, r8, r12
	STEP_NEG_X2  r6, r5, r8, r12
	STEP_NEG_X2  r5, r4, r8, r12
	STEP_NEG_X2  r4, r3, r8, r12
	STEP_NEG_X2  r3, r2, r8, r12
	stm	r0!, { r3, r4, r5, r6, r7 }
	ldm	r1!, { r2, r3, r4, r5, r6 }
	STEP_NEG_X2  r7, r6, r8, r12
	STEP_NEG_X2  r6, r5, r8, r12
	STEP_NEG_X2  r5, r4, r8, r12
	STEP_NEG_X2  r4, r3, r8, r12
	STEP_NEG_X2  r3, r2, r8, r12
	stm	r0!, { r3, r4, r5, r6 }
	strh	r7, [r0]
	pop	{ r4, r5, r6, r7, r8 }
	bx	lr
	.size	curve9767_inner_gf_condneg, .-curve9767_inner_gf_condneg

@ =======================================================================
@ void gf_mul2_move_inner(uint16_t *d, const uint16_t *a);
@
@ Compute: d <- 2*a
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function.
@
@ Cost: 70
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_mul2_move_inner, %function
gf_mul2_move_inner:
	mov	r10, #0x00010001
	movw	r11, #9767
	orrs	r12, r11, r11, lsl #16

	ldm	r1!, { r2, r3, r4, r5, r6 }
	MP_MUL2_X2  r2, r7, r10, r11, r12
	MP_MUL2_X2  r3, r7, r10, r11, r12
	MP_MUL2_X2  r4, r7, r10, r11, r12
	MP_MUL2_X2  r5, r7, r10, r11, r12
	MP_MUL2_X2  r6, r7, r10, r11, r12
	stm	r0!, { r2, r3, r4, r5, r6 }

	ldm	r1!, { r2, r3, r4, r5, r6 }
	MP_MUL2_X2  r2, r7, r10, r11, r12
	MP_MUL2_X2  r3, r7, r10, r11, r12
	MP_MUL2_X2  r4, r7, r10, r11, r12
	MP_MUL2_X2  r5, r7, r10, r11, r12
	MP_MUL2_X2  r6, r7, r10, r11, r12
	stm	r0!, { r2, r3, r4, r5 }
	strh	r6, [r0]

	bx	lr
	.size	gf_mul2_move_inner, .-gf_mul2_move_inner

@ =======================================================================
@ void gf_sub2_inner(uint16_t *d, const uint16_t *a);
@
@ Compute: d <- d-2*a
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function. High registers are
@ preserved.
@
@ Cost: 144
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_sub2_inner, %function
gf_sub2_inner:
	movw	r11, #9767
	movw	r12, #0xB5BE
	movt	r12, #0x0006

	ldm	r0, { r2, r3, r4, r5 }
	ldm	r1!, { r6, r7, r8, r10 }
	subs	r2, r2, r6, lsl #1
	subs	r3, r3, r7, lsl #1
	subs	r4, r4, r8, lsl #1
	subs	r5, r5, r10, lsl #1
	orrs	r6, r11, r11, lsl #16
	adds	r2, r2, r6, lsl #1
	adds	r3, r3, r6, lsl #1
	adds	r4, r4, r6, lsl #1
	adds	r5, r5, r6, lsl #1
	BASERED_X2  r2, r6, r7, r11, r12
	BASERED_X2  r3, r6, r7, r11, r12
	BASERED_X2  r4, r6, r7, r11, r12
	BASERED_X2  r5, r6, r7, r11, r12
	stm	r0!, { r2, r3, r4, r5 }

	ldm	r0, { r2, r3, r4, r5 }
	ldm	r1!, { r6, r7, r8, r10 }
	subs	r2, r2, r6, lsl #1
	subs	r3, r3, r7, lsl #1
	subs	r4, r4, r8, lsl #1
	subs	r5, r5, r10, lsl #1
	orrs	r6, r11, r11, lsl #16
	adds	r2, r2, r6, lsl #1
	adds	r3, r3, r6, lsl #1
	adds	r4, r4, r6, lsl #1
	adds	r5, r5, r6, lsl #1
	BASERED_X2  r2, r6, r7, r11, r12
	BASERED_X2  r3, r6, r7, r11, r12
	BASERED_X2  r4, r6, r7, r11, r12
	BASERED_X2  r5, r6, r7, r11, r12
	stm	r0!, { r2, r3, r4, r5 }

	ldm	r0, { r2, r3 }
	ldm	r1!, { r6, r7 }
	subs	r2, r2, r6, lsl #1
	subs	r3, r3, r7, lsl #1
	orrs	r6, r11, r11, lsl #16
	adds	r2, r2, r6, lsl #1
	adds	r3, r3, r6, lsl #1
	BASERED_X2  r2, r6, r7, r11, r12
	uxth	r3, r3
	BASERED  r3, r6, r11, r12
	str	r2, [r0]
	strh	r3, [r0, #4]

	bx	lr
	.size	gf_sub2_inner, .-gf_sub2_inner

@ =======================================================================
@ void gf_sub_sub_inner(uint16_t *d, const uint16_t *a, const uint16_t *b);
@
@ Compute: d <- d-a-b
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function.
@
@ Cost: 172
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_sub_sub_inner, %function
gf_sub_sub_inner:
	push	{ lr }
	movw	r14, #0x4C4E
	movt	r14, #0x4C4E
	movw	r11, #9767
	movw	r12, #0xB5BE
	movt	r12, #0x0006

	ldm	r0, { r3, r4, r5 }
	ldm	r1!, { r6, r7, r8 }
	sub	r3, r6
	sub	r4, r7
	sub	r5, r8
	ldm	r2!, { r6, r7, r8 }
	sub	r3, r6
	sub	r4, r7
	sub	r5, r8
	add	r3, r14
	add	r4, r14
	add	r5, r14
	BASERED_X2  r3, r6, r7, r11, r12
	BASERED_X2  r4, r6, r7, r11, r12
	BASERED_X2  r5, r6, r7, r11, r12
	stm	r0!, { r3, r4, r5 }

	ldm	r0, { r3, r4, r5 }
	ldm	r1!, { r6, r7, r8 }
	sub	r3, r6
	sub	r4, r7
	sub	r5, r8
	ldm	r2!, { r6, r7, r8 }
	sub	r3, r6
	sub	r4, r7
	sub	r5, r8
	add	r3, r14
	add	r4, r14
	add	r5, r14
	BASERED_X2  r3, r6, r7, r11, r12
	BASERED_X2  r4, r6, r7, r11, r12
	BASERED_X2  r5, r6, r7, r11, r12
	stm	r0!, { r3, r4, r5 }

	ldm	r0, { r3, r4, r5 }
	ldm	r1!, { r6, r7, r8 }
	sub	r3, r6
	sub	r4, r7
	sub	r5, r8
	ldm	r2!, { r6, r7, r8 }
	sub	r3, r6
	sub	r4, r7
	sub	r5, r8
	add	r3, r14
	add	r4, r14
	add	r5, r14
	BASERED_X2  r3, r6, r7, r11, r12
	BASERED_X2  r4, r6, r7, r11, r12
	BASERED_X2  r5, r6, r7, r11, r12
	stm	r0!, { r3, r4, r5 }

	ldrh	r3, [r0]
	ldrh	r6, [r1]
	sub	r3, r6
	ldrh	r6, [r2]
	sub	r3, r6
	adds	r3, r3, r11, lsl #1
	BASERED  r3, r6, r11, r12
	strh	r3, [r0]

	pop	{ pc }
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
@ Cost: 173
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_sub_sub_mul2_inner, %function
gf_sub_sub_mul2_inner:
	push	{ lr }
	movw	r14, #0x989C
	movt	r14, #0x989C
	movw	r11, #9767
	movw	r12, #0xB5BE
	movt	r12, #0x0006

	ldm	r0, { r3, r4, r5 }
	ldm	r1!, { r6, r7, r8 }
	sub	r3, r6
	sub	r4, r7
	sub	r5, r8
	ldm	r2!, { r6, r7, r8 }
	sub	r3, r6
	sub	r4, r7
	sub	r5, r8
	adds	r3, r14, r3, lsl #1
	adds	r4, r14, r4, lsl #1
	adds	r5, r14, r5, lsl #1
	BASERED_X2  r3, r6, r7, r11, r12
	BASERED_X2  r4, r6, r7, r11, r12
	BASERED_X2  r5, r6, r7, r11, r12
	stm	r0!, { r3, r4, r5 }

	ldm	r0, { r3, r4, r5 }
	ldm	r1!, { r6, r7, r8 }
	sub	r3, r6
	sub	r4, r7
	sub	r5, r8
	ldm	r2!, { r6, r7, r8 }
	sub	r3, r6
	sub	r4, r7
	sub	r5, r8
	adds	r3, r14, r3, lsl #1
	adds	r4, r14, r4, lsl #1
	adds	r5, r14, r5, lsl #1
	BASERED_X2  r3, r6, r7, r11, r12
	BASERED_X2  r4, r6, r7, r11, r12
	BASERED_X2  r5, r6, r7, r11, r12
	stm	r0!, { r3, r4, r5 }

	ldm	r0, { r3, r4, r5 }
	ldm	r1!, { r6, r7, r8 }
	sub	r3, r6
	sub	r4, r7
	sub	r5, r8
	ldm	r2!, { r6, r7, r8 }
	sub	r3, r6
	sub	r4, r7
	sub	r5, r8
	adds	r3, r14, r3, lsl #1
	adds	r4, r14, r4, lsl #1
	adds	r5, r14, r5, lsl #1
	BASERED_X2  r3, r6, r7, r11, r12
	BASERED_X2  r4, r6, r7, r11, r12
	BASERED_X2  r5, r6, r7, r11, r12
	stm	r0!, { r3, r4, r5 }

	ldrh	r3, [r0]
	ldrh	r6, [r1]
	sub	r3, r6
	ldrh	r6, [r2]
	sub	r3, r6
	lsls	r3, #1
	adds	r3, r3, r11, lsl #2
	BASERED  r3, r6, r11, r12
	strh	r3, [r0]

	pop	{ pc }
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
@ Cost: 153
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_sub_mul3_move_inner, %function
gf_sub_mul3_move_inner:
	movw	r11, #9767
	movw	r12, #0xB5BE
	movt	r12, #0x0006
	orrs	r10, r11, r11, lsl #16

	ldm	r1!, { r3, r4, r5 }
	ldm	r2!, { r6, r7, r8 }
	subs	r3, r6
	subs	r4, r7
	subs	r5, r8
	add	r3, r10
	add	r4, r10
	add	r5, r10
	adds	r3, r3, r3, lsl #1
	adds	r4, r4, r4, lsl #1
	adds	r5, r5, r5, lsl #1
	BASERED_X2  r3, r6, r7, r11, r12
	BASERED_X2  r4, r6, r7, r11, r12
	BASERED_X2  r5, r6, r7, r11, r12
	stm	r0!, { r3, r4, r5 }

	ldm	r1!, { r3, r4, r5 }
	ldm	r2!, { r6, r7, r8 }
	subs	r3, r6
	subs	r4, r7
	subs	r5, r8
	add	r3, r10
	add	r4, r10
	add	r5, r10
	adds	r3, r3, r3, lsl #1
	adds	r4, r4, r4, lsl #1
	adds	r5, r5, r5, lsl #1
	BASERED_X2  r3, r6, r7, r11, r12
	BASERED_X2  r4, r6, r7, r11, r12
	BASERED_X2  r5, r6, r7, r11, r12
	stm	r0!, { r3, r4, r5 }

	ldm	r1!, { r3, r4, r5 }
	ldm	r2!, { r6, r7, r8 }
	subs	r3, r6
	subs	r4, r7
	subs	r5, r8
	add	r3, r10
	add	r4, r10
	add	r5, r10
	adds	r3, r3, r3, lsl #1
	adds	r4, r4, r4, lsl #1
	adds	r5, r5, r5, lsl #1
	BASERED_X2  r3, r6, r7, r11, r12
	BASERED_X2  r4, r6, r7, r11, r12
	BASERED_X2  r5, r6, r7, r11, r12
	stm	r0!, { r3, r4, r5 }

	ldrh	r3, [r1]
	ldrh	r6, [r2]
	subs	r3, r6
	add	r3, r11
	adds	r3, r3, r3, lsl #1
	BASERED  r3, r6, r11, r12
	strh	r3, [r0]

	bx	lr
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
@ the ABI, but it is usable as a non-global function.
@
@ Cost: 43
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_mulz9_inner, %function
gf_mulz9_inner:
	@ Source elements 0..9 go to destination elements 9..18.
	ldm	r1!, { r2, r3, r4, r5, r6 }
	lsrs	r7, r6, #16
	pkhbt	r6, r6, r5
	ror	r6, r6, #16
	pkhbt	r5, r5, r4
	ror	r5, r5, #16
	pkhbt	r4, r4, r3
	ror	r4, r4, #16
	pkhbt	r3, r3, r2
	ror	r3, r3, #16
	adds	r0, #20
	stm	r0!, { r3, r4, r5, r6, r7 }

	@ Source elements 10..18 go to destination elements 0..8, but
	@ doubled (for reduction modulo z^19-2).
	ldm	r1!, { r3, r4, r5, r6, r7 }
	lsls	r3, #1
	lsls	r4, #1
	lsls	r5, #1
	lsls	r6, #1
	lsls	r7, #1
	pkhbt	r7, r7, r2, lsl #16
	subs	r0, #40
	stm	r0!, { r3, r4, r5, r6, r7 }

	bx	lr
	.size	gf_mulz9_inner, .-gf_mulz9_inner

@ =======================================================================
@ void gf_sub8_inner(uint16_t *d, const uint16_t *a);
@
@ Compute: d <- d-8*a
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function.
@
@ Cost: 187
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_sub8_inner, %function
gf_sub8_inner:
	movw	r11, #9767
	movw	r12, #0xB5BE
	movt	r12, #0x0006

	ldm	r0, { r2, r3, r4 }
	ldm	r1!, { r5, r6, r7 }
	MP_SUB8_X2  r2, r5, r8, r10, r11, r12
	MP_SUB8_X2  r3, r6, r8, r10, r11, r12
	MP_SUB8_X2  r4, r7, r8, r10, r11, r12
	stm	r0!, { r2, r3, r4 }

	ldm	r0, { r2, r3, r4 }
	ldm	r1!, { r5, r6, r7 }
	MP_SUB8_X2  r2, r5, r8, r10, r11, r12
	MP_SUB8_X2  r3, r6, r8, r10, r11, r12
	MP_SUB8_X2  r4, r7, r8, r10, r11, r12
	stm	r0!, { r2, r3, r4 }

	ldm	r0, { r2, r3, r4 }
	ldm	r1!, { r5, r6, r7 }
	MP_SUB8_X2  r2, r5, r8, r10, r11, r12
	MP_SUB8_X2  r3, r6, r8, r10, r11, r12
	MP_SUB8_X2  r4, r7, r8, r10, r11, r12
	stm	r0!, { r2, r3, r4 }

	ldrh	r2, [r0]
	ldrh	r5, [r1]
	subs	r2, r2, r5, lsl #3
	adds	r2, r2, r11, lsl #3
	BASERED  r2, r8, r11, r12
	strh	r2, [r0]

	bx	lr
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
@ Cost: 728
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

	@ Get Frobenius coefficients and load reduction constant.
	bl	gf_get_frob_constants
	movw	r12, #9767
	movw	r14, #0x1669
	movt	r14, #0xD8AF

	@ Register contents at this point:
	@   r1    pointer to a0
	@   r2    pointer to first Frobenius coefficient
	@   r3    pointer to temporary area where frob(a) should be written
	@   r12   constant 9767
	@   r14   constant p1i = 3635353193

	@ Load a0..a3 in r4:r5, and a10..a13 in r6:r7
	ldrd	r4, r5, [r1, #0]	
	ldrd	r6, r7, [r1, #20]	

	@ Store a0+a10..a3+a13 into stack buffer.
	adds	r8, r4, r6
	adds	r10, r5, r7
	strd	r8, r10, [sp, #(4 * (37 + 0))]

	@ Apply Frobenius operator on a0..a3 (coefficient for a0 is 1).
	ldrd	r8, r10, [r2, #0]
	smultt	r8, r4, r8
	MONTYRED  r8, r11, r12, r14
	pkhbt	r4, r4, r8, lsl #16
	smulbb	r0, r5, r10
	smultt	r11, r5, r10
	MONTYRED_X2  r5, r0, r11, r12, r14
	strd	r4, r5, [r3, #0]

	@ Apply Frobenius operator on a10..a13.
	ldrd	r8, r10, [r2, #20]
	smulbb	r0, r6, r8
	smultt	r11, r6, r8
	MONTYRED_X2  r6, r0, r11, r12, r14
	smulbb	r0, r7, r10
	smultt	r11, r7, r10
	MONTYRED_X2  r7, r0, r11, r12, r14
	strd	r6, r7, [r3, #20]

	@ Compute b0+b10..b3+b13 into stack buffer.
	adds	r4, r6
	adds	r5, r7
	strd	r4, r5, [sp, #(4 * (42 + 0))]

	@ Load a4..a7 in r4:r5, and a14..a17 in r6:r7
	ldrd	r4, r5, [r1, #8]	
	ldrd	r6, r7, [r1, #28]	

	@ Store a4+a14..a7+a17 into stack buffer.
	adds	r8, r4, r6
	adds	r10, r5, r7
	strd	r8, r10, [sp, #(4 * (37 + 2))]

	@ Apply Frobenius operator on a4..a7.
	ldrd	r8, r10, [r2, #8]
	smulbb	r0, r4, r8
	smultt	r11, r4, r8
	MONTYRED_X2  r4, r0, r11, r12, r14
	smulbb	r0, r5, r10
	smultt	r11, r5, r10
	MONTYRED_X2  r5, r0, r11, r12, r14
	strd	r4, r5, [r3, #8]

	@ Apply Frobenius operator on a14..a17.
	ldrd	r8, r10, [r2, #28]
	smulbb	r0, r6, r8
	smultt	r11, r6, r8
	MONTYRED_X2  r6, r0, r11, r12, r14
	smulbb	r0, r7, r10
	smultt	r11, r7, r10
	MONTYRED_X2  r7, r0, r11, r12, r14
	strd	r6, r7, [r3, #28]

	@ Compute b0+b10..b3+b13 into stack buffer.
	adds	r4, r6
	adds	r5, r7
	strd	r4, r5, [sp, #(4 * (42 + 2))]

	@ Load a8:a9 in r4 and a18 in r6
	ldr	r4, [r1, #16]	
	ldrh	r6, [r1, #36]	

	@ Store a8+a18 and a9 into stack buffer.
	adds	r5, r4, r6
	str	r5, [sp, #(4 * (37 + 4))]

	@ Apply Frobenius operator on a8:a9.
	ldr	r8, [r2, #16]
	smulbb	r0, r4, r8
	smultt	r11, r4, r8
	MONTYRED_X2  r4, r0, r11, r12, r14
	str	r4, [r3, #16]

	@ Apply Frobenius operator on a18.
	ldrh	r7, [r2, #36]
	muls	r6, r7
	MONTYRED  r6, r11, r12, r14
	strh	r6, [r3, #36]

	@ Store b8+b18 and b9 into stack buffer.
	adds	r4, r6
	str	r4, [sp, #(4 * (42 + 4))]

	@ Finish multiplication by jumping into gf_mul_inner().
	movs	r1, r3
	b	gf_mul_inner_alternate_entry

	.size	gf_mulfrob_inner, .-gf_mulfrob_inner

@ =======================================================================
@ void gf_mul_inner(uint16_t *c, const uint16_t *a, const uint16_t *b)
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function.
@
@ Source/destination arrays are guaranteed to be 32-bit aligned.
@
@ Cost: 609
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
	ldm	r1!, { r3, r4, r5, r6, r7, r8, r10, r11, r12, r14 }
	uxth	r14, r14
	add	r3, r8
	add	r4, r10
	add	r5, r11
	add	r6, r12
	add	r7, r14
	stm	r0!, { r3, r4, r5, r6, r7 }

	@ Compute and store B0+B1.
	@ r0 is already at the right value.
	ldm	r2!, { r3, r4, r5, r6, r7, r8, r10, r11, r12, r14 }
	uxth	r14, r14
	add	r3, r8
	add	r4, r10
	add	r5, r11
	add	r6, r12
	add	r7, r14
	stm	r0!, { r3, r4, r5, r6, r7 }

	@ Set r1 to point to B[0].
	subs	r1, r2, #40

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
	movw	r6, #9767
	movw	r7, #0x1669
	movt	r7, #0xD8AF
	KFIX  0, 20, 47

	add	sp, #284
	pop	{ pc }
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
	push	{ r4, r5, r6, r7, r8, r10, r11, lr }
	bl	gf_mul_inner
	pop	{ r4, r5, r6, r7, r8, r10, r11, pc }
	.size	curve9767_inner_gf_mul, .-curve9767_inner_gf_mul

@ =======================================================================
@ void gf_sqr_inner(uint16_t *c, const uint16_t *a)
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function.
@
@ Source/destination arrays are guaranteed to be 32-bit aligned.
@
@ Cost: 351
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_sqr_inner, %function
gf_sqr_inner:
	push	{ lr }

	@ Stack elements (indices for 4-byte words):
	@   0..36    S = A*A (not Montgomery-reduced)
	@   37       pointer to output

	sub	sp, #156
	str	r0, [sp, #148]   @ Output buffer address.

	@ Load all input words in r2..r12
	@   r2   a0:a1
	@   r3   a2:a3
	@   r4   a4:a5
	@   r5   a6:a7
	@   r6   a8:a9
	@   r7   a10:a11
	@   r8   a12:a13
	@   r10  a14:a15
	@   r11  a16:a17
	@   r12  a18
	ldm	r1!, { r2, r3, r4, r5, r6, r7, r8, r10, r11, r12 }

	@ a0*a0 + 4*(a1*a18+a2*a17+a3*a16+...+a9*a10) -> s0
	smultb	r0, r2, r12
	smladx	r0, r3, r11, r0
	smladx	r0, r4, r10, r0
	smladx	r0, r5, r8, r0
	smladx	r0, r6, r7, r0
	smulbb	r1, r2, r2
	adds	r0, r1, r0, lsl #2

	@ 2*a10*a10 + 2*(a0*a1) -> s1
	smulbb	r1, r7, r7
	lsls	r1, #1
	smladx	r1, r2, r2, r1

	strd	r0, r1, [sp, #(4 * 0)]

	@ a1*a1 + 2*(a0*a2) + 4*(a3*a18+a4*a17+a5*a16+...+a10*a11) -> s2
	smultb	r0, r3, r12
	smladx	r0, r4, r11, r0
	smladx	r0, r5, r10, r0
	smladx	r0, r6, r8, r0
	smlabt	r0, r7, r7, r0
	smulbb	r1, r2, r3
	adds	r0, r1, r0, lsl #1
	smultt	r1, r2, r2
	adds	r0, r1, r0, lsl #1

	@ 2*a11*a11 + 2*(a0*a3+a1*a2) -> s3
	smuadx	r1, r2, r3
	smlatt	r1, r7, r7, r1
	lsls	r1, #1

	strd	r0, r1, [sp, #(4 * 2)]

	@ a2*a2 + 2*(a0*a4+a1*a3) + 4*(a5*a18+a6*a17+a7*a16+...+a11*a12) -> s4
	smultb	r0, r4, r12
	smladx	r0, r5, r11, r0
	smladx	r0, r6, r10, r0
	smladx	r0, r7, r8, r0
	lsls	r0, #2
	smlabb	r0, r3, r3, r0
	smulbb	r14, r2, r4
	smlatt	r14, r2, r3, r14
	adds	r0, r0, r14, lsl #1

	@ 2*a12*a12 + 2*(a0*a5+a1*a4+a2*a3) -> s5
	smuadx	r1, r2, r4
	smlabt	r1, r3, r3, r1
	smlabb	r1, r8, r8, r1
	lsls	r1, #1

	strd	r0, r1, [sp, #(4 * 4)]

	@ a3*a3 + 2*(a0*a6+a1*a5+a2*a4) + 4*(a7*a18+a8*a17+...+a12*a13) -> s6
	smultb	r0, r5, r12
	smladx	r0, r6, r11, r0
	smladx	r0, r7, r10, r0
	smlabt	r0, r8, r8, r0
	lsls	r0, #2
	smlatt	r0, r3, r3, r0
	smulbb	r14, r2, r5
	smlatt	r14, r2, r4, r14
	smlabb	r14, r3, r4, r14
	adds	r0, r0, r14, lsl #1

	@ 2*a13*a13 + 2*(a0*a7+a1*a6+a2*a5+a3*a4) -> s7
	smuadx	r1, r2, r5
	smladx	r1, r3, r4, r1
	smlatt	r1, r8, r8, r1
	lsls	r1, #1

	strd	r0, r1, [sp, #(4 * 6)]

	@ a4*a4 + 4*(a9*a18+a10*a17+a11*a16+a12*a15+a13*a14) -> s8
	smultb	r0, r6, r12
	smladx	r0, r7, r11, r0
	smladx	r0, r8, r10, r0
	lsls	r0, #2
	smlabb	r0, r4, r4, r0

	@ 2*a14*a14 + 2*(a0*a9+a1*a8+a2*a7+a3*a6+a4*a5) -> s9
	smuadx	r1, r2, r6
	smladx	r1, r3, r5, r1
	smlabt	r1, r4, r4, r1
	smlabb	r1, r10, r10, r1
	lsls	r1, #1

	strd	r0, r1, [sp, #(4 * 8)]

	@ a5*a5 + 4*(a11*a18+a12*a17+a13*a16+a14*a15) -> s10
	smultb	r0, r7, r12
	smladx	r0, r8, r11, r0
	smlabt	r0, r10, r10, r0
	lsls	r0, #2
	smlatt	r0, r4, r4, r0

	@ 2*a15*a15 + 2*(a0*a11+a1*a10+a2*a9+...+a5*a6) -> s11
	smuadx	r1, r2, r7
	smladx	r1, r3, r6, r1
	smladx	r1, r4, r5, r1
	smlatt	r1, r10, r10, r1
	lsls	r1, #1

	strd	r0, r1, [sp, #(4 * 10)]

	@ a6*a6 + 4*(a13*a18+a14*a7+a15*a16) -> s12
	smultb	r0, r8, r12
	smladx	r0, r10, r11, r0
	lsls	r0, #2
	smlabb	r0, r5, r5, r0

	@ 2*a16*a16 + 2*(a0*a13+a1*a12+a2*a11+...+a6*a7) -> s13
	smuadx	r1, r2, r8
	smladx	r1, r3, r7, r1
	smladx	r1, r4, r6, r1
	smlabt	r1, r5, r5, r1
	smlabb	r1, r11, r11, r1
	lsls	r1, #1

	strd	r0, r1, [sp, #(4 * 12)]

	@ a7*a7 + 4*(a15*a18+a16*a17) -> s14
	smultb	r0, r10, r12
	smlabt	r0, r11, r11, r0
	lsls	r0, #2
	smlatt	r0, r5, r5, r0

	@ 2*a17*a17 + 2*(a0*a15+a1*a14+a2*a13+...+a7*a8) -> s15
	smuadx	r1, r2, r10
	smladx	r1, r3, r8, r1
	smladx	r1, r4, r7, r1
	smladx	r1, r5, r6, r1
	smlatt	r1, r11, r11, r1
	lsls	r1, #1

	strd	r0, r1, [sp, #(4 * 14)]

	@ a8*a8 + 4*(a17*a18) -> s16
	smultb	r0, r11, r12
	lsls	r0, #2
	smlabb	r0, r6, r6, r0

	@ Store s16 alone; s17 will go with s18.
	str	r0, [sp, #(4 * 16)]

	@ 2*a18*a18 + 2*(a0*a17+a1*a16+a2*a15+...+a8*a9) -> s17
	smuadx	r1, r2, r11
	smladx	r1, r3, r10, r1
	smladx	r1, r4, r8, r1
	smladx	r1, r5, r7, r1
	smlabt	r1, r6, r6, r1
	smlabb	r1, r12, r12, r1
	lsls	r1, #1

	@ Data reorganization
	@   r2   a0:a1
	@   r3   a2:a3
	@   r4   a4:a5
	@   r5   a6:a7
	@   r6   a8:a9
	@   r7   a10:a11
	@   r8   a12:a11
	@   r10  a14:a13
	@   r11  a16:a15
	@   r12  a18:a17
	pkhbt	r12, r12, r11
	pkhbt	r11, r11, r10
	pkhbt	r10, r10, r8
	pkhbt	r8, r8, r7

	@ a9*a9 + 2*(a0*a18+a1*a17+a2*a16+...+a8*a10) -> s18
	smuad	r0, r2, r12
	smlad	r0, r3, r11, r0
	smlad	r0, r4, r10, r0
	smlad	r0, r5, r8, r0
	smlabb	r0, r6, r7, r0
	lsls	r0, #1
	smlatt	r0, r6, r6, r0

	strd	r1, r0, [sp, #(4 * 17)]

	@ Data reorganization
	@   r2   a0:a1
	@   r3   a2:a3
	@   r4   a4:a5
	@   r5   a6:a7
	@   r6   a8:a9
	@   r7   a10:a9
	@   r8   a12:a11
	@   r10  a14:a13
	@   r11  a16:a15
	@   r12  a18:a17
	pkhbt	r7, r7, r6

	ldrd	r1, r0, [sp, #(4 * 15)]

	@ s16 + 2*(a0*a16+a1*a15+a2*a14+...+a7*a9) -> s16
	smuad	r14, r2, r11
	smlad	r14, r3, r10, r14
	smlad	r14, r4, r8, r14
	smlad	r14, r5, r7, r14
	adds	r0, r0, r14, lsl #1

	@ s15 + 4*(a16*a18) -> s15
	smulbb	r14, r11, r12
	adds	r1, r1, r14, lsl #2

	strd	r1, r0, [sp, #(4 * 15)]

	ldrd	r1, r0, [sp, #(4 * 13)]

	@ s14 + 2*(a0*a14+a1*a13+a2*a12+...+a6*a8) -> s14
	smuad	r14, r2, r10
	smlad	r14, r3, r8, r14
	smlad	r14, r4, r7, r14
	smlabb	r14, r5, r6, r14
	adds	r0, r0, r14, lsl #1

	@ s13 + 4*(a14*a18+a15*a17) -> s13
	pkhbt	r14, r10, r11
	smuad	r14, r12, r14
	adds	r1, r1, r14, lsl #2

	strd	r1, r0, [sp, #(4 * 13)]

	@ Data reorganization
	@   r2   a0:a1
	@   r3   a2:a3
	@   r4   a4:a5
	@   r5   a6:a7
	@   r6   a8:a7
	@   r7   a10:a9
	@   r8   a12:a11
	@   r10  a14:a13
	@   r11  a16:a15
	@   r12  a18:a17
	pkhbt	r6, r6, r5

	ldrd	r1, r0, [sp, #(4 * 11)]

	@ s12 + 2*(a0*a12+a1*a11+a2*a10+...+a5*a7) -> s12
	smuad	r14, r2, r8
	smlad	r14, r3, r7, r14
	smlad	r14, r4, r6, r14
	adds	r0, r0, r14, lsl #1

	@ s11 + 4*(a12*a18+a13*a17+a14*16) -> s11
	pkhbt	r14, r8, r10
	smuad	r14, r14, r12
	smlabb	r14, r10, r11, r14
	adds	r1, r1, r14, lsl #2

	strd	r1, r0, [sp, #(4 * 11)]

	ldrd	r1, r0, [sp, #(4 * 9)]

	@ s10 + 2*(a0*a10+a1*a9+a2*a8+a3*a7+a4*a6) -> s10
	smuad	r14, r2, r7
	smlad	r14, r3, r6, r14
	smlabb	r14, r4, r5, r14
	adds	r0, r0, r14, lsl #1

	@ Data reorganization
	@   r2   a0:a1
	@   r3   a2:a3
	@   r4   a4:a9
	@   r5   a6:a5
	@   r6   a8:a7
	@   r7   a10:a11
	@   r8   a12:a11
	@   r10  a14:a13
	@   r11  a16:a15
	@   r12  a18:a17
	pkhbt	r5, r5, r4
	pkhbt	r4, r4, r7
	pkhbt	r7, r7, r8

	@ s9 + 4*(a10*a18+a11*a17+a12*a16+a13*a15) -> s9
	pkhbt	r14, r8, r10
	smuad	r14, r14, r11
	smlad	r14, r7, r12, r14
	adds	r1, r1, r14, lsl #2

	strd	r1, r0, [sp, #(4 * 9)]

	ldrd	r1, r0, [sp, #(4 * 7)]

	@ s8 + 2*(a0*a8+a1*a7+a2*a6+a3*a5) -> s8
	smuad	r14, r2, r6
	smlad	r14, r3, r5, r14
	adds	r0, r0, r14, lsl #1

	@ Data reorganization
	@   r2   a0:a1
	@   r3   a2:a3
	@   r4   a4:a5
	@   r5   a6:a7
	@   r6   (scratch)
	@   r7   a10:a11
	@   r8   a12:a11
	@   r10  a14:a13
	@   r11  a16:a15
	@   r12  a18:a17
	@   r14  a8:a9
	pkhbt	r14, r6, r4
	pkhbt	r4, r4, r5
	pkhbt	r5, r5, r6

	@ s7 + 4*(a8*a18+a9*a17+a10*a16+a11*a15+a12*a14) -> s7
	smuad	r6, r12, r14
	smlad	r6, r7, r11, r6
	smlabb	r6, r8, r10, r6
	adds	r1, r1, r6, lsl #2

	strd	r1, r0, [sp, #(4 * 7)]

	@ s6 is complete.

	ldr	r1, [sp, #(4 * 5)]

	@ s5 + 4*(a6*a18+a7*a17+a8*a16+...+a11*a13) -> s5
	smuad	r6, r5, r12
	smlad	r6, r14, r11, r6
	smlad	r6, r7, r10, r6
	adds	r1, r1, r6, lsl #2

	str	r1, [sp, #(4 * 5)]

	@ s4 is complete.

	ldr	r0, [sp, #(4 * 3)]

	@ s3 + 4*(a4*a18+a5*a17+a6*a16+...+a10*a12) -> s3
	smuad	r6, r4, r12
	smlad	r6, r5, r11, r6
	smlad	r6, r14, r10, r6
	smlabb	r6, r7, r8, r6
	adds	r0, r0, r6, lsl #2

	str	r0, [sp, #(4 * 3)]

	@ s2 is complete.

	ldr	r1, [sp, #(4 * 1)]

	@ s1 + 4*(a2*a18+a3*a17+a4*a16+...+a9*a11) -> s1
	smuad	r6, r3, r12
	smlad	r6, r4, r11, r6
	smlad	r6, r5, r10, r6
	smlad	r6, r14, r8, r6
	adds	r1, r1, r6, lsl #2

	str	r1, [sp, #(4 * 1)]

	@ s0 is complete.

	@ All 19 intermediate values have been assembled; only remain
	@ Montgomery reductions.
	ldr	r0, [sp, #148]
	mov	r1, sp
	movw	r12, #9767
	movw	r14, #0x1669
	movt	r14, #0xD8AF

	ldm	r1!, { r3, r4, r5, r6, r7, r8, r10 }
	MONTYRED_X2  r2, r3, r4, r12, r14
	MONTYRED_X2  r3, r5, r6, r12, r14
	MONTYRED_X2  r4, r7, r8, r12, r14
	stm	r0!, { r2, r3, r4 }
	ldm	r1!, { r3, r4, r5, r6, r7, r8 }
	MONTYRED_X2  r2, r10, r3, r12, r14
	MONTYRED_X2  r3, r4, r5, r12, r14
	MONTYRED_X2  r4, r6, r7, r12, r14
	stm	r0!, { r2, r3, r4 }
	ldm	r1!, { r3, r4, r5, r6, r7, r10 }
	MONTYRED_X2  r2, r8, r3, r12, r14
	MONTYRED_X2  r3, r4, r5, r12, r14
	MONTYRED_X2  r4, r6, r7, r12, r14
	stm	r0!, { r2, r3, r4 }
	MONTYRED  r10, r5, r12, r14
	strh	r10, [r0]

	add	sp, #156
	pop	{ pc }
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
	push	{ r4, r5, r6, r7, r8, r10, r11, lr }
	bl	gf_sqr_inner
	pop	{ r4, r5, r6, r7, r8, r10, r11, pc }
	.size	curve9767_inner_gf_sqr, .-curve9767_inner_gf_sqr

@ =======================================================================
@ uint32_t *gf_get_frob_constants(int knum)
@
@ This function does conform to the ABI: it expects its parameter in r2,
@ and writes the return value in r2. Register r7 is used as scratch
@ register. Other registers (except r14 = lr) are untouched.
@
@ This function returns the Frobenius coefficients for the operator that
@ raises an input element to the power p^k. Coefficients are in Montgomery
@ representation and packed. Coefficient for degree 0 is always 1 (i.e.
@ 7182, in Montgomery representation).
@
@ Parameter knum (in r2) specifies k with the following correspondance:
@   k   knum
@   1    0
@   2    1
@   4    2
@   8    3
@   9    4
@
@ Cost: 5
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_get_frob_constants, %function
gf_get_frob_constants:
	@ Each array is 10 words = 40 bytes
	adds	r2, r2, r2, lsl #2
	adr	r7, const_frob1_cc
	adds	r2, r7, r2, lsl #3
	bx	lr
	.align	2
const_frob1_cc:
	.long	214113294
	.long	159913769
	.long	314180033
	.long	431562175
	.long	399315202
	.long	121904762
	.long	564794794
	.long	355213887
	.long	152242006
	.long	     6748
const_frob2_cc:
	.long	388570126
	.long	499057089
	.long	511316226
	.long	574559658
	.long	442238806
	.long	159911107
	.long	431559354
	.long	121903053
	.long	355213738
	.long	     2323
const_frob4_cc:
	.long	 29432846
	.long	363467010
	.long	214107990
	.long	399315642
	.long	152248746
	.long	499062569
	.long	574561914
	.long	159914588
	.long	121903545
	.long	     5420
const_frob8_cc:
	.long	285350926
	.long	314181462
	.long	388571562
	.long	442244730
	.long	355211705
	.long	363463105
	.long	399314115
	.long	499058963
	.long	159916607
	.long	     1860
const_frob9_cc:
	.long	399318030
	.long	285350492
	.long	431556883
	.long	499058518
	.long	314185004
	.long	 29434431
	.long	159916458
	.long	388568490
	.long	214107972
	.long	     7802
	.size	gf_get_frob_constants, .-gf_get_frob_constants

@ =======================================================================
@ void gf_frob_inner(uint16_t *c, const uint16_t *a, int knum)
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function.
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
@ Cost: 118
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_frob_inner, %function
gf_frob_inner:
	push	{ lr }
	bl	gf_get_frob_constants
	movw	r12, #9767
	movw	r14, #0x1669
	movt	r14, #0xD8AF

	@ Read a0..a7
	ldm	r1!, { r3, r4, r5, r6 }

	@ Read Frobenius constants 0..3
	@ (coefficient 0 is always 1, we can skip it)
	ldm	r2!, { r7, r8 }
	smultt	r10, r3, r7
	MONTYRED  r10, r11, r12, r14
	pkhbt	r3, r3, r10, lsl #16
	smulbb	r10, r4, r8
	smultt	r11, r4, r8
	MONTYRED_X2  r4, r10, r11, r12, r14

	@ Read Frobenius constants 4..7
	ldm	r2!, { r7, r8 }
	smulbb	r10, r5, r7
	smultt	r11, r5, r7
	MONTYRED_X2  r5, r10, r11, r12, r14
	smulbb	r10, r6, r8
	smultt	r11, r6, r8
	MONTYRED_X2  r6, r10, r11, r12, r14

	@ Write output elements 0..7
	stm	r0!, { r3, r4, r5, r6 }

	@ Read a8..a15
	ldm	r1!, { r3, r4, r5, r6 }

	@ Read Frobenius constants 8..11
	ldm	r2!, { r7, r8 }
	smulbb	r10, r3, r7
	smultt	r11, r3, r7
	MONTYRED_X2  r3, r10, r11, r12, r14
	smulbb	r10, r4, r8
	smultt	r11, r4, r8
	MONTYRED_X2  r4, r10, r11, r12, r14

	@ Read Frobenius constants 12..15
	ldm	r2!, { r7, r8 }
	smulbb	r10, r5, r7
	smultt	r11, r5, r7
	MONTYRED_X2  r5, r10, r11, r12, r14
	smulbb	r10, r6, r8
	smultt	r11, r6, r8
	MONTYRED_X2  r6, r10, r11, r12, r14

	@ Write output elements 8..15
	stm	r0!, { r3, r4, r5, r6 }

	@ Read a16..a18
	ldm	r1!, { r3, r4 }

	@ Read Frobenius constants 16..18
	ldm	r2!, { r7, r8 }
	smulbb	r10, r3, r7
	smultt	r11, r3, r7
	MONTYRED_X2  r3, r10, r11, r12, r14
	smulbb	r4, r4, r8
	MONTYRED  r4, r11, r12, r14

	@ Write output elements 16..18
	str	r3, [r0]
	strh	r4, [r0, #4]

	pop	{ pc }
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
@     r1   pointer to element of index 20 in b (i.e. 40 + address of b0).
@   Output:
@     r0   result (in GF(p))
@     r6   value p = 9767
@     r7   value p1i = 3635353193
@ Registers are not preserved.
@
@ Cost: 43
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_mul_to_base_inner, %function
gf_mul_to_base_inner:
	@ Load a0..a9.
	ldm	r0!, { r2, r3, r4, r5, r6 }

	@ Load b10..b18.
	ldmdb	r1!, { r7, r8, r10, r11, r12 }

	@ a1*b18+a2*b17+a3*b16+...+a8*b11+a9*b10 -> r12
	smultb	r12, r2, r12
	smladx	r12, r3, r11, r12
	smladx	r12, r4, r10, r12
	smladx	r12, r5, r8, r12
	smladx	r12, r6, r7, r12

	@ Load a10..a18.
	ldm	r0, { r0, r3, r4, r5, r6 }

	@ Load b0..b9.
	ldmdb	r1, { r1, r7, r8, r10, r11 }

	@ r12 + a10*b9+a11*b8+...+a17*b2+a18*b1 -> r12
	smladx	r12, r0, r11, r12
	smladx	r12, r3, r10, r12
	smladx	r12, r4, r8, r12
	smladx	r12, r5, r7, r12
	smlabt	r12, r6, r1, r12

	@ a0*b0 + 2*r12 -> r0
	smulbb	r2, r2, r1
	adds	r0, r2, r12, lsl #1

	@ Apply Montgomery reduction.
	movw	r6, #9767
	movw	r7, #0x1669
	movt	r7, #0xD8AF
	MONTYRED  r0, r1, r6, r7
	bx	lr
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
@ Registers r1..r5 are not preserved; however, r6, r7 and high registers
@ are preserved.
@
@ Cost: 56
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	mp_inv_inner, %function
mp_inv_inner:
	@ The source value is in r0; we raise it to power p-2 = 9765.

	@ Set r1 = x^8
	movs	r1, r0
	MONTYMUL  r1, r1, r5, r6, r7
	MONTYMUL  r1, r1, r5, r6, r7
	MONTYMUL  r1, r1, r5, r6, r7
	@ Set r1 = x^9
	movs	r2, r1
	MONTYMUL  r2, r0, r5, r6, r7
	@ Set r3 = x^(9*16+8)
	movs	r3, r2
	MONTYMUL  r3, r3, r5, r6, r7
	MONTYMUL  r3, r3, r5, r6, r7
	MONTYMUL  r3, r3, r5, r6, r7
	MONTYMUL  r3, r3, r5, r6, r7
	MONTYMUL  r3, r1, r5, r6, r7
	@ Set r3 = x^((9*16+8)*16+9)
	MONTYMUL  r3, r3, r5, r6, r7
	MONTYMUL  r3, r3, r5, r6, r7
	MONTYMUL  r3, r3, r5, r6, r7
	MONTYMUL  r3, r3, r5, r6, r7
	MONTYMUL  r3, r2, r5, r6, r7
	@ Set r0 = x^(((9*16+8)*16+9)*4+1)
	MONTYMUL  r3, r3, r5, r6, r7
	MONTYMUL  r3, r3, r5, r6, r7
	MONTYMUL  r0, r3, r5, r6, r7

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
@ Registers r0, r6 and r7 are preserved; other registers are not.
@
@ Cost: 93
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_mulconst_move_inner, %function
gf_mulconst_move_inner:
	ldm	r2!, { r3, r4, r5, r8, r10 }
	smulbb	r11, r0, r3
	smulbt	r12, r0, r3
	MONTYRED_X2  r3, r11, r12, r6, r7
	smulbb	r11, r0, r4
	smulbt	r12, r0, r4
	MONTYRED_X2  r4, r11, r12, r6, r7
	smulbb	r11, r0, r5
	smulbt	r12, r0, r5
	MONTYRED_X2  r5, r11, r12, r6, r7
	smulbb	r11, r0, r8
	smulbt	r12, r0, r8
	MONTYRED_X2  r8, r11, r12, r6, r7
	smulbb	r11, r0, r10
	smulbt	r12, r0, r10
	MONTYRED_X2  r10, r11, r12, r6, r7
	stm	r1!, { r3, r4, r5, r8, r10 }

	ldm	r2!, { r3, r4, r5, r8, r10 }
	smulbb	r11, r0, r3
	smulbt	r12, r0, r3
	MONTYRED_X2  r3, r11, r12, r6, r7
	smulbb	r11, r0, r4
	smulbt	r12, r0, r4
	MONTYRED_X2  r4, r11, r12, r6, r7
	smulbb	r11, r0, r5
	smulbt	r12, r0, r5
	MONTYRED_X2  r5, r11, r12, r6, r7
	smulbb	r11, r0, r8
	smulbt	r12, r0, r8
	MONTYRED_X2  r8, r11, r12, r6, r7
	smulbb	r11, r0, r10
	MONTYRED  r11, r12, r6, r7
	stm	r1!, { r3, r4, r5, r8 }
	strh	r11, [r1]
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
@ Cost: 4062
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
	add	r1, sp, #40
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
	push	{ r4, r5, r6, r7, r8, r10, r11, lr }
	bl	gf_inv_inner
	pop	{ r4, r5, r6, r7, r8, r10, r11, pc }
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
@ Cost: 10539 (3983 if fast exit)
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
	add	r1, sp, #(40 + 40)
	bl	gf_mul_to_base_inner

	@ Save a^r into register r3, and compute (a^r)^((p-1)/2) into
	@ register r0. gf_mul_to_base_inner() already sets r6 and r7 to
	@ p and p1i, respectively.

	@ (a^r)^9 = ((a^r)^8)*(a^r)
	movs	r3, r0
	MONTYMUL  r0, r0, r5, r6, r7
	MONTYMUL  r0, r0, r5, r6, r7
	MONTYMUL  r0, r0, r5, r6, r7
	MONTYMUL  r0, r3, r5, r6, r7
	@ (a^r)^19 = (((a^r)^9)^2)*(a^r)
	MONTYMUL  r0, r0, r5, r6, r7
	MONTYMUL  r0, r3, r5, r6, r7
	movs	r1, r0
	@ (a^r)^4883 = (((a^r)^19)^256)*((a^r)^19)
	MONTYMUL  r0, r0, r5, r6, r7
	MONTYMUL  r0, r0, r5, r6, r7
	MONTYMUL  r0, r0, r5, r6, r7
	MONTYMUL  r0, r0, r5, r6, r7
	MONTYMUL  r0, r0, r5, r6, r7
	MONTYMUL  r0, r0, r5, r6, r7
	MONTYMUL  r0, r0, r5, r6, r7
	MONTYMUL  r0, r0, r5, r6, r7
	MONTYMUL  r0, r1, r5, r6, r7
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
	movw	r6, #9767
	movw	r7, #0x1669
	movt	r7, #0xD8AF
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
	push	{ r4, r5, r6, r7, r8, r10, r11, lr }
	bl	gf_sqrt_inner
	pop	{ r4, r5, r6, r7, r8, r10, r11, pc }
	.size	curve9767_inner_gf_sqrt, .-curve9767_inner_gf_sqrt

@ =======================================================================
@ void gf_cubert_inner(uint32_t *c, const uint32_t *a)
@
@ This computes the (unique) cube root of 'a' into 'c'.
@
@ This function does not preserve registers; this does not comply with
@ the ABI, but it is usable as a non-global function.
@
@ Cost: 12113
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
	push	{ r4, r5, r6, r7, r8, r10, r11, lr }
	bl	gf_cubert_inner
	pop	{ r4, r5, r6, r7, r8, r10, r11, pc }
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
@    no register preserved
@
@ Cost: 49
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	gf_eq_inner, %function
gf_eq_inner:
	ldm	r0!, { r2, r3, r4, r5, r6 }
	ldm	r1!, { r7, r8, r10, r11, r12 }
	eors	r2, r7
	eors	r3, r8
	eors	r4, r10
	eors	r5, r11
	eors	r6, r12
	orrs	r2, r3
	orrs	r2, r4
	orrs	r2, r5
	orrs	r2, r6

	ldm	r0!, { r3, r4, r5, r6 }
	ldm	r1!, { r7, r8, r10, r11 }
	eors	r3, r7
	eors	r4, r8
	eors	r5, r10
	eors	r6, r11
	orrs	r2, r3
	orrs	r2, r4
	orrs	r2, r5
	orrs	r2, r6

	ldrh	r3, [r0]
	ldrh	r4, [r1]
	eors	r3, r4
	orrs	r2, r3
	adds	r0, #4
	adds	r1, #4

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
	push	{ r4, r5, r6, r7, r8, r10, r11, lr }
	bl	gf_eq_inner
	lsrs	r0, r2, #31
	pop	{ r4, r5, r6, r7, r8, r10, r11, pc }
	.size	curve9767_inner_gf_eq, .-curve9767_inner_gf_eq

@ =======================================================================
@ void curve9767_point_add(curve9767_point *Q3,
@                          const curve9767_point *Q1,
@                          const curve9767_point *Q2)
@
@ This is a global function; it follows the ABI.
@
@ Cost: 7073
@ =======================================================================

	.align	1
	.global	curve9767_point_add
	.thumb
	.thumb_func
	.type	curve9767_point_add, %function
curve9767_point_add:
	push	{ r4, r5, r6, r7, r8, r10, r11, lr }

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
	mov	r0, sp
	ldr	r1, [sp, #128]
	adds	r1, #4
	ldr	r2, [sp, #132]
	adds	r2, #4
	adds	r3, r1, #40
	ldr	r5, [sp, #136]
	lsl	r11, r5, #1
	movs	r12, #0x00010001
	movw	r14, #9767
	adds	r14, r14, r14, lsl #16
	STEP_LAMBDA_D_X6
	STEP_LAMBDA_D_X6
	STEP_LAMBDA_D_X6
	ldrh	r4, [r2]
	ldrh	r7, [r1]
	subs	r4, r7
	adds	r4, r14
	bics	r4, r4, r11, asr #1
	ldrh	r7, [r3]
	mls	r4, r7, r11, r4
	ssub16	r7, r14, r4
	ands	r7, r12, r7, lsr #15
	mls	r4, r7, r14, r4
	strh	r4, [r0]

	@ x1^2 -> t2
	add	r0, sp, #40
	subs	r1, #36
	bl	gf_sqr_inner

	@ If x1 == x2, set t2 to 3*x1^2+a; otherwise, set t2 to y2-y1.
	add	r0, sp, #40
	ldr	r1, [sp, #128]
	adds	r1, #44
	ldr	r2, [sp, #132]
	adds	r2, #44
	add	r3, sp, #40
	ldr	r11, [sp, #136]
	movw	r12, #0xB5BE
	movt	r12, #0x0006
	movw	r14, #9767
	adds	r14, r14, r14, lsl #16
	STEP_LAMBDA_N_X6  0
	STEP_LAMBDA_N_X6  6
	STEP_LAMBDA_N_X6  12
	uxth	r10, r14
	ldrh	r4, [r2]
	ldrh	r7, [r1]
	subs	r4, r7
	adds	r4, r10
	ldrh	r7, [r3]
	adds	r7, r7, r7, lsl #1
	bics	r4, r4, r11
	mls	r4, r7, r11, r4
	BASERED  r4, r7, r10, r12
	strh	r4, [r0]

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
	add	r0, sp, #40
	ldr	r1, [sp, #128]
	adds	r1, #4
	ldr	r2, [sp, #132]
	adds	r2, #4
	bl	gf_sub_sub_inner

	@ Compute y3 = lambda*(x1 - x3) - y1 (in t3)
	add	r0, sp, #80
	ldr	r1, [sp, #128]
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
	ldr	r4, [r1]
	adds	r1, #4
	ldr	r2, [sp, #132]
	ldr	r5, [r2]
	adds	r2, #4
	add	r3, sp, #40
	rsbs	r11, r5, #0
	rsbs	r12, r4, #0
	bics	r12, r12, r11
	orrs	r14, r11, r12
	mvns	r14, r14
	STEP_SELECT3_X6  0
	STEP_SELECT3_X6  0
	STEP_SELECT3_X6  0
	STEP_SELECT3_X6  1
	STEP_SELECT3_X6  0
	STEP_SELECT3_X6  0
	ldm	r1!, { r4, r5 }
	ands	r4, r4, r11
	ands	r5, r5, r11
	ldm	r2!, { r7, r8 }
	mls	r4, r7, r12, r4
	mls	r5, r8, r12, r5
	ldm	r3!, { r7, r8 }
	mls	r4, r7, r14, r4
	mls	r5, r8, r14, r5
	str	r4, [r0]
	strh	r5, [r0, #4]

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
	rsbs	r1, r3, #1
	rsbs	r2, r4, #1
	ands	r1, r2
	ands	r1, r5
	bics	r1, r6
	ands	r3, r4
	orrs	r1, r3
	ldr	r0, [sp, #124]
	str	r1, [r0]

	add	sp, #144
	pop	{ r4, r5, r6, r7, r8, r10, r11, pc }
	.size	curve9767_point_add, .-curve9767_point_add

@ =======================================================================
@ void curve9767_point_mul2k(curve9767_point *Q3,
@                            const curve9767_point *Q1, unsigned k)
@
@ This is a global function; it follows the ABI.
@
@ Cost:
@    k == 0    66
@    k == 1    7079
@    k >= 2    3251 + k*4611
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
	@    40      160      S (also ZZZZ)
	@    50      200      ZZ
	@    60      240      YY (also XXXX)
	@    70      280      YYYY (also x*(z^9))
	@    80      320      M (also (y^2)*(z^9))
	@    90      360      pointer to output (Q3)
	@    91      364      pointer to input (Q1)
	@    92      368      remaining iteration count
	@
	@ Note that XX, XXXX and x*(z^9) are consecutive and in that order;
	@ this allows reading from all three with a single moving pointer.
	@ Similarly, YYYY and (y^2)*(z^9) are consecutive.

	push	{ r4, r5, r6, r7, r8, r10, r11, lr }
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
	@ We use Z^2 = 4*y^2 and Z^4 = 16*y^4 directly in the equations,
	@ because we'll need Z^2 and Z^4 for the next doubling; this avoids
	@ the multiplications by 4 and 16.

	@ x^2 -> XX
	add	r0, sp, #120
	adds	r1, #4
	bl	gf_sqr_inner

	@ x^4 -> XXXX
	add	r0, sp, #240
	add	r1, sp, #120
	bl	gf_sqr_inner

	@ 2*y -> Z
	add	r0, sp, #80
	ldr	r1, [sp, #364]
	adds	r1, #44
	bl	gf_mul2_move_inner

	@ 4*y^2 -> ZZ
	add	r0, sp, #200
	add	r1, sp, #80
	bl	gf_sqr_inner

	@ 16*y^4 -> ZZZZ
	add	r0, sp, #160
	add	r1, sp, #200
	bl	gf_sqr_inner

	@ x*(z^9) -> YYYY
	add	r0, sp, #280
	ldr	r1, [sp, #364]
	adds	r1, #4
	bl	gf_mulz9_inner

	@ 4*(y^2)*(z^9) -> M
	add	r0, sp, #320
	add	r1, sp, #200
	bl	gf_mulz9_inner

	@ Assemble coordinates:
	@   X = x^4 - 2*a*x^2 - 8*b*x + a^2
	@   Y = y^4 + 18*b*y^2 + 3*a*(x^4 - 2*a*x^2 - 8*b*x) - 27*b^2 - a^3
	@   r0    pointer into X
	@
	@ Constants:
	@   r11    low: R = 7182, high: -2*a*R = 4024
	@   r12    low: -8*b*R = 2928, high: 18*b*R/4 = 8120
	@   r14    p1i = 3635353193
	mov	r0, sp
	movw	r11, #0x1C0E
	movt	r11, #0x0FB8
	movw	r12, #0x0B70
	movt	r12, #0x1FB8
	movw	r14, #0x1669
	movt	r14, #0xD8AF

mul2k_loop1:
	@ Compute x^4 - 2*a*x^2 - 8*b*x (four words, into r1..r4)
	ldrd	r5, r6, [r0, #240]
	smulbb	r1, r5, r11
	smultb	r2, r5, r11
	smulbb	r3, r6, r11
	smultb	r4, r6, r11
	ldrd	r5, r6, [r0, #120]
	smlabt	r1, r5, r11, r1
	smlatt	r2, r5, r11, r2
	smlabt	r3, r6, r11, r3
	smlatt	r4, r6, r11, r4
	ldrd	r5, r6, [r0, #280]
	smlabb	r1, r5, r12, r1
	smlatb	r2, r5, r12, r2
	smlabb	r3, r6, r12, r3
	smlatb	r4, r6, r12, r4

	@ Compute y^4 + 18*b*y^2 + 3*a*(x^4 - 2*a*x^2 - 8*b*x)
	@ (four words, into r5..r8)
	@ We have the unreduced (x^4-2*a*x^2-8*b*x) in r1..r4, with
	@ maximum value 166644554. Multiplying by -3*a = 9 yields a
	@ value which is no more than 1499800986; we add 16*(9767^2)
	@ (i.e. 1526308624) to get a positive value.
	ldrd	r5, r6, [r0, #320]
	smultt	r8, r6, r12
	smulbt	r7, r6, r12
	smultt	r6, r5, r12
	smulbt	r5, r5, r12
	movw	r10, #0x9F10
	movt	r10, #0x5AF9
	add	r5, r10
	add	r6, r10
	add	r7, r10
	add	r8, r10
	movs	r10, #9
	mls	r5, r1, r10, r5
	mls	r6, r2, r10, r6
	mls	r7, r3, r10, r7
	mls	r8, r4, r10, r8

	@ We temporarily reuse the top 16 bits of r11 for R/16 mod p
	@ (we want R*y^4 but we have Z^4 = 16*y^4).
	movt	r11, #0x2323
	ldr	r10, [r0, #160]
	smlabt	r5, r10, r11, r5
	smlatt	r6, r10, r11, r6
	ldr	r10, [r0, #164]
	smlabt	r7, r10, r11, r7
	smlatt	r8, r10, r11, r8
	movt	r11, #0x0FB8

	@ Apply Montgomery reductions.
	movw	r10, #9767
	MONTYRED_X2_ALT  r1, r2, r10, r14
	MONTYRED_X2      r2, r3, r4, r10, r14
	MONTYRED_X2      r3, r5, r6, r10, r14
	MONTYRED_X2      r4, r7, r8, r10, r14

	@ Write resulting words.
	strd	r1, r2, [r0]
	strd	r3, r4, [r0, #40]

	@ Increment r0 and loop. There are five iterations; we exit when
	@ r0 reaches sp+40.
	adds	r0, #8
	sub	r1, sp, r0
	adds	r1, #40
	bne	mul2k_loop1

	@ Add a^2 to X, and -27*b^2-a^3 to Y
	ldrh	r1, [sp]
	movw	r2, #6036
	adds	r1, r2
	MPMOD_AFTER_ADD  r1, r2, r10
	strh	r1, [sp]
	ldrh	r1, [sp, #40]
	movw	r2, #8341
	adds	r1, r2
	MPMOD_AFTER_ADD  r1, r2, r10
	strh	r1, [sp, #40]
	ldrh	r1, [sp, #76]
	adds	r1, #1112
	MPMOD_AFTER_ADD  r1, r2, r10
	strh	r1, [sp, #76]

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
	pop	{ r4, r5, r6, r7, r8, r10, r11, pc }
	.size	curve9767_point_mul2k, .-curve9767_point_mul2k

@ =======================================================================
@ void curve9767_inner_window_lookup(curve9767_point *Q,
@                                    curve window_point8 *window, uint32_t k)
@
@ This is a global function; it follows the ABI.
@
@ Cost: 479
@ =======================================================================

	.align	1
	.global	curve9767_inner_window_lookup
	.thumb
	.thumb_func
	.type	curve9767_inner_window_lookup, %function
curve9767_inner_window_lookup:
	push	{ r4, r5, r6, r7, r8, r10, r11, lr }

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
	subs	r2, #1
	sbcs	r6, r6

	@ m2
	subs	r2, #1
	sbcs	r7, r7

	@ m3
	subs	r2, #1
	sbcs	r8, r8

	@ m4
	subs	r2, #1
	sbcs	r10, r10

	@ m5
	subs	r2, #1
	sbcs	r11, r11

	@ m6
	subs	r2, #1
	sbcs	r12, r12

	@ m7
	subs	r2, #1
	sbcs	r14, r14

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

	pop	{ r4, r5, r6, r7, r8, r10, r11, pc }
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

@ =======================================================================
@ The two functions below are used for benchmark calibration:
@  - curve9767_inner_dummy(): returns immediately
@  - curve9767_inner_dummy_wrapper(): saves all registers mandated by the
@    ABI, then calls curve9767_inner_dummy(), and restores the registers.
@ =======================================================================

	.align	1
	.global	curve9767_inner_dummy
	.thumb
	.thumb_func
	.type	curve9767_inner_dummy, %function
curve9767_inner_dummy:
	bx	lr
	.size	curve9767_inner_dummy, .-curve9767_inner_dummy

	.align	1
	.global	curve9767_inner_dummy_wrapper
	.thumb
	.thumb_func
	.type	curve9767_inner_dummy_wrapper, %function
curve9767_inner_dummy_wrapper:
	push	{ r4, r5, r6, r7, r8, r10, r11, lr }
	bl	curve9767_inner_dummy
	pop	{ r4, r5, r6, r7, r8, r10, r11, pc }
	.size	curve9767_inner_dummy_wrapper, .-curve9767_inner_dummy_wrapper
