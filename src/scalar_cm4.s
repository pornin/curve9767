@ =======================================================================
@ =======================================================================

	.syntax	unified
	.cpu	cortex-m4
	.file	"scalar_cm0.s"
	.text

@ =======================================================================
@ We represent integers over limbs of 32 bits each (little-endian
@ order).
@ =======================================================================

@ =======================================================================
@ int bitlength(const uint32_t *x, int len)
@
@ Compute the bit length of a signed integer (len words, with len > 0).
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	bitlength, %function
bitlength:
	@ Adjust r0 so that we can index limbs starting at 1.
	subs	r0, #4

	@ Get sign mask.
	ldr	r3, [r0, r1, lsl #2]
	asrs	r3, r3, #31

	@ Find the top word not equal to the sign mask.
L_bitlength__1:
	ldr	r2, [r0, r1, lsl #2]
	eors	r2, r3
	bne	L_bitlength__2
	subs	r1, #1
	bne	L_bitlength__1

	@ All limbs are equal to the sign mask; bit length is zero.
	movs	r0, #0
	bx	lr

L_bitlength__2:
	@ Word r2 (sign-adjusted) was found at offset r1 (in limbs).
	@ Offsets starts at 1 here.
	clz	r2, r2
	rsbs	r0, r2, r1, lsl #5
	bx	lr
	.size	bitlength, .-bitlength

@ =======================================================================
@ void add_lshift(uint32_t *a, const uint32_t *b, int len, int s)
@
@ 'len' must be at least 2, and a multiple of 2.
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	add_lshift, %function
add_lshift:
	cmp	r3, #31
	bhi	L_add_lshift__generic
	cmp	r3, #0
	beq	L_add_lshift__noshift

	@ Loop for 0 < s < 32.
	@ Each iteration processes two input words.
	@   r2    loop counter
	@   r3    shift count s
	@   r4    anti-shift count (32-s)
	@   r5    carry
	@   r6    delayed word chunk
	push	{ r4, r5, r6, r7, r8, lr }
	rsbs	r4, r3, #32
	movs	r5, #0
	movs	r6, #0
L_add_lshift__loop:
	ldm	r1!, { r7, r8 }
	lsls	r12, r7, r3
	lsrs	r7, r4
	orrs	r6, r12
	lsls	r12, r8, r3
	orrs	r7, r12
	ldrd	r12, r14, [r0]
	lsrs	r5, #1
	adcs	r6, r12
	adcs	r7, r14
	adcs	r5, r5
	stm	r0!, { r6, r7 }
	lsrs	r6, r8, r4
	subs	r2, #2
	bne	L_add_lshift__loop
	pop	{ r4, r5, r6, r7, r8, pc }

L_add_lshift__noshift:
	@ Specialized loop for s == 0.
	@ Two words are processed at once. r3 is the carry (0 or 1).
	push	{ r4, r5, r6, r7, lr }
L_add_lshift__noshift_loop:
	ldrd	r4, r5, [r0]
	ldm	r1!, { r6, r7 }
	lsrs	r3, #1      @ Load low bit of r3 into carry.
	adcs	r4, r6
	adcs	r5, r7
	adcs	r3, r3      @ r3 is 0 at this point; this retrieves the carry.
	stm	r0!, { r4, r5 }
	subs	r2, #2
	bne	L_add_lshift__noshift_loop
	pop	{ r4, r5, r6, r7, pc }

L_add_lshift__generic:
	@ Shift count is 32 or greater: we use the generic function.
	push	{ r0, lr }
	bl	curve9767_inner_add_lshift
	pop	{ r0, pc }
	.size	add_lshift, .-add_lshift

@ =======================================================================
@ void sub_lshift(uint32_t *a, const uint32_t *b, int len, int s)
@
@ 'len' must be at least 2, and a multiple of 2.
@ =======================================================================

	.align	1
	.thumb
	.thumb_func
	.type	sub_lshift, %function
sub_lshift:
	cmp	r3, #31
	bhi	L_sub_lshift__generic
	cmp	r3, #0
	beq	L_sub_lshift__noshift

	@ Loop for 0 < s < 32.
	@ Each iteration processes two input words.
	@   r2    loop counter
	@   r3    shift count s
	@   r4    anti-shift count (32-s)
	@   r5    carry
	@   r6    delayed word chunk
	push	{ r4, r5, r6, r7, r8, lr }
	rsbs	r4, r3, #32
	movs	r5, #1
	movs	r6, #0
L_sub_lshift__loop:
	ldm	r1!, { r7, r8 }
	lsls	r12, r7, r3
	lsrs	r7, r4
	orrs	r6, r12
	lsls	r12, r8, r3
	orrs	r7, r12
	ldrd	r12, r14, [r0]
	lsrs	r5, #1
	sbcs	r12, r6
	sbcs	r14, r7
	adcs	r5, r5
	stm	r0!, { r12, r14 }
	lsrs	r6, r8, r4
	subs	r2, #2
	bne	L_sub_lshift__loop
	pop	{ r4, r5, r6, r7, r8, pc }

L_sub_lshift__noshift:
	@ Specialized loop for s == 0.
	@ Two words are processed at once. r3 is the carry (0 or 1).
	push	{ r4, r5, r6, r7, lr }
	movs	r3, #1
L_sub_lshift__noshift_loop:
	ldrd	r4, r5, [r0]
	ldm	r1!, { r6, r7 }
	lsrs	r3, #1      @ Load low bit of r3 into carry.
	sbcs	r4, r6
	sbcs	r5, r7
	adcs	r3, r3      @ r3 is 0 at this point; this retrieves the carry.
	stm	r0!, { r4, r5 }
	subs	r2, #2
	bne	L_sub_lshift__noshift_loop
	pop	{ r4, r5, r6, r7, pc }

L_sub_lshift__generic:
	@ Shift count is 32 or greater: we use the generic function.
	push	{ r0, lr }
	bl	curve9767_inner_sub_lshift
	pop	{ r0, pc }
	.size	sub_lshift, .-sub_lshift

@ =======================================================================
@ void curve9767_inner_reduce_basis_vartime_core(uint32_t *tab)
@ =======================================================================

	.align	1
	.global	curve9767_inner_reduce_basis_vartime_core
	.thumb
	.thumb_func
	.type	curve9767_inner_reduce_basis_vartime_core, %function
curve9767_inner_reduce_basis_vartime_core:
	push	{ r4, r5, r6, r7, r8, lr }

	@ r4 = tab
	@ r5 = 0 or 32:
	@    if r5 == 0 then:
	@         r4+0     u0
	@         r4+16    u1
	@         r4+32    v0
	@         r4+48    v1
	@         r4+64    nu
	@         r4+128   nv
	@    else:
	@         r4+0     v0
	@         r4+16    v1
	@         r4+32    u0
	@         r4+48    u1
	@         r4+64    nv
	@         r4+128   nu
	@ Therefore:
	@    u0   is at address   r4+r5
	@    u1   is at address   r4+16+r5
	@    v0   is at address   r4+32-r5
	@    v1   is at address   r4+48-r5
	@    nu   is at address   r4+64+2*r5
	@    nv   is at address   r4+128-2*r5
	@    sp   is at address   r4+192
	@
	@ r6 = current length for nu, nv and sp, counted in 32-bit words.
	@ It starts at 16 and is always even.
	movs	r4, r0
	movs	r5, #0
	movs	r6, #16

L_reduce_core__loop:
	@ Compare nu with nv. If nu < nv, then we do a swap (i.e. replace
	@ r5 with 32-r5).
	adds	r1, r4, r5, lsl #1
	subs	r0, r4, r5, lsl #1
	adds	r1, #64
	adds	r0, #128
	subs	r7, r6, #1
L_reduce_core__cmp_nu_nv_1:
	ldr	r2, [r1, r7, lsl #2]
	ldr	r3, [r0, r7, lsl #2]
	cmp	r3, r2
	bne	L_reduce_core__cmp_nu_nv_2
	subs	r7, #1
	bpl	L_reduce_core__cmp_nu_nv_1
	cmp	r3, r2
L_reduce_core__cmp_nu_nv_2:
	bls	L_reduce_core__size
	movs	r2, r0
	movs	r0, r1
	movs	r1, r2
	rsbs	r5, r5, #32

L_reduce_core__size:
	@ Pointer to nu is in r1. We know that |sp| <= nu; thus, if
	@ the top two words of nu are redundant (accounting for the sign
	@ bit), then we can reduce the size by two words.
	adds	r2, r1, r6, lsl #2
	subs	r2, #12
	ldm	r2, { r2, r3, r7 }
	lsrs	r2, #31
	orrs	r2, r3
	orrs	r2, r7
	bne	L_reduce_core__blen_nv
	subs	r6, #2

L_reduce_core__blen_nv:
	@ Get the bit length of nv. At that point, pointer to nv is in r0.
	@ If the bit length is 253 or less, then we have reached the exit
	@ condition.
	movs	r1, r6
	bl	bitlength
	cmp	r0, #253
	bls	L_reduce_core__exit

	@ Get the bit length of sp.
	movs	r7, r0
	adds	r0, r4, #192
	movs	r1, r6
	bl	bitlength

	@ s <- max(len(sp) - len(nv), 0)  (into r7)
	subs	r7, r0, r7
	asrs	r1, r7, #31
	bics	r7, r1

	@ Get sign of sp and branch.
	adds	r0, r6, #47
	ldr	r0, [r4, r0, lsl #2]
	adds	r0, r0
	bcs	L_reduce_core__spneg

	@ u0 <- u0 - (v0 << s)
	@ u1 <- u1 - (v1 << s)
	@ nu <- nu + (nv << (2*s)) - (sp << (s+1))
	@ sp <- sp - (nv << s)
	adds	r0, r4, r5
	subs	r1, r4, r5
	adds	r1, #32
	movs	r2, #4
	movs	r3, r7
	bl	sub_lshift
	adds	r0, r4, r5
	adds	r0, #16
	subs	r1, r4, r5
	adds	r1, #48
	movs	r2, #4
	movs	r3, r7
	bl	sub_lshift
	subs	r1, r4, r5, lsl #1
	adds	r0, r4, r5, lsl #1
	adds	r0, #64
	adds	r1, #128
	movs	r2, r6
	lsls	r3, r7, #1
	bl	add_lshift
	adds	r0, r4, r5, lsl #1
	adds	r0, #64
	adds	r1, r4, #192
	movs	r2, r6
	adds	r3, r7, #1
	bl	sub_lshift
	adds	r0, r4, #192
	subs	r1, r4, r5, lsl #1
	adds	r1, #128
	movs	r2, r6
	movs	r3, r7
	bl	sub_lshift

	b	L_reduce_core__loop

L_reduce_core__spneg:
	@ u0 <- u0 + (v0 << s)
	@ u1 <- u1 + (v1 << s)
	@ nu <- nu + (nv << (2*s)) + (sp << (s+1))
	@ sp <- sp + (nv << s)
	adds	r0, r4, r5
	subs	r1, r4, r5
	adds	r1, #32
	movs	r2, #4
	movs	r3, r7
	bl	add_lshift
	adds	r0, r4, r5
	adds	r0, #16
	subs	r1, r4, r5
	adds	r1, #48
	movs	r2, #4
	movs	r3, r7
	bl	add_lshift
	subs	r1, r4, r5, lsl #1
	adds	r0, r4, r5, lsl #1
	adds	r0, #64
	adds	r1, #128
	movs	r2, r6
	lsls	r3, r7, #1
	bl	add_lshift
	adds	r0, r4, r5, lsl #1
	adds	r0, #64
	adds	r1, r4, #192
	movs	r2, r6
	adds	r3, r7, #1
	bl	add_lshift
	adds	r0, r4, #192
	subs	r1, r4, r5, lsl #1
	adds	r1, #128
	movs	r2, r6
	movs	r3, r7
	bl	add_lshift

	b	L_reduce_core__loop

L_reduce_core__exit:
	@ If we are in swapped state (r5 != 0), then v is in the wrong
	@ place and we must move it.
	cmp	r5, #0
	beq	L_reduce_core__exit_2
	ldm	r4!, { r0, r1, r2, r3, r5, r6, r7, r8 }
	stm	r4!, { r0, r1, r2, r3, r5, r6, r7, r8 }
L_reduce_core__exit_2:
	pop	{ r4, r5, r6, r7, r8, pc }
	.size	curve9767_inner_reduce_basis_vartime_core, .-curve9767_inner_reduce_basis_vartime_core

@ =======================================================================
