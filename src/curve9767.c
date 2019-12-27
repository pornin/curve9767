#include "inner.h"

/* ====================================================================== */

/* see curve9767.h */
int
curve9767_point_encode(void *dst, const curve9767_point *Q)
{
	int i;
	uint8_t *buf;
	uint8_t m;

	curve9767_inner_gf_encode(dst, Q->x);
	buf = dst;
	buf[31] |= curve9767_inner_gf_is_neg(Q->y) << 6;
	m = (uint8_t)-Q->neutral;
	for (i = 0; i < 31; i ++) {
		buf[i] |= m;
	}
	buf[31] |= m & 0x7F;
	return 1 - Q->neutral;
}

/* see curve9767.h */
int
curve9767_point_encode_X(void *dst, const curve9767_point *Q)
{
	int i;
	uint8_t *buf;
	uint8_t m;

	curve9767_inner_gf_encode(dst, Q->x);
	buf = dst;
	m = (uint8_t)-Q->neutral;
	for (i = 0; i < 31; i ++) {
		buf[i] |= m;
	}
	buf[31] |= m & 0x3F;
	return 1 - Q->neutral;
}

/* see curve9767.h */
int
curve9767_point_decode(curve9767_point *Q, const void *src)
{
	uint32_t tb, r;

	/*
	 * Check that the top bit of the top byte is 0.
	 */
	tb = ((const uint8_t *)src)[31];
	r = 1 - (tb >> 7);

	/*
	 * Decode the X coordinate.
	 */
	r &= curve9767_inner_gf_decode(Q->x, src);

	/*
	 * Obtain the Y coordinate.
	 */
	r &= curve9767_inner_make_y(Q->y, Q->x, (tb >> 6) & 0x01);

	/*
	 * If one of the step failed, then the value is turned into the
	 * point-at-infinity.
	 */
	Q->neutral = 1 - r;
	return r;
}

/* see curve9767.h */
void
curve9767_point_neg(curve9767_point *Q2, const curve9767_point *Q1)
{
	if (Q2 != Q1) {
		Q2->neutral = Q1->neutral;
		memcpy(Q2->x, Q1->x, sizeof Q1->x);
	}
	curve9767_inner_gf_neg(Q2->y, Q1->y);
}

/* see curve9767.h */
void
curve9767_point_sub(curve9767_point *Q3,
	const curve9767_point *Q1, const curve9767_point *Q2)
{
	curve9767_point T;

	curve9767_point_neg(&T, Q2);
	curve9767_point_add(Q3, Q1, &T);
}

/*
 * A constant scalar value, used in point multiplication algorithms.
 */
static const uint8_t scalar_win4_off[] = {
	0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
	0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
	0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
	0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x08
};

/*
 * Perform a lookup in a window, based on the value of four bits, with
 * the extra processing for the conditional negation. The value 'e' MUST
 * be in the 0..15 range. T is set to (e-8)*B, where B is the base point
 * of the window (the window contains the coordinates of i*B for i in
 * 1..8).
 *
 * This function sets the neutral flag of T under the assumption that the
 * base point for the window is not the point-at-infinity (i.e. the function
 * sets T->neutral to 1 if e == 8, to 0 otherwise). It is up to the caller
 * to adjust the flag afterwards, depending on the source parameters.
 */
static inline void
do_lookup(curve9767_point *T, const window_point8 *win, uint32_t e)
{
	uint32_t e8, index, r;

	/*
	 * Set e8 to 1 if e == 8, to 0 otherwise.
	 */
	e8 = (e & -e) >> 3;

	/*
	 * If e >= 9, lookup index must be e - 9.
	 * If e <= 7, lookup index must be 7 - e.
	 * If e == 8, lookup index is unimportant, as long as it is in
	 * the proper range (0..7). Code below sets index to 0 in that case.
	 *
	 * We set r to 1 if e <= 8, 0 otherwise. This conditions the
	 * final negation.
	 */
	index = e - 9;
	r = index >> 31;
	index = (index ^ -r) - r;
	index &= (e8 - 1);

	curve9767_inner_window_lookup(T, win, index);
	T->neutral = e8;
	curve9767_inner_gf_condneg(T->y, r);
}

/* see curve9767.h */
void
curve9767_point_mul(curve9767_point *Q3, const curve9767_point *Q1,
	const curve9767_scalar *s)
{
	/*
	 * Algorithm:
	 *
	 *  - We use a window optimization: we precompute small multiples
	 *    of Q1, and add one such small multiple every four doublings.
	 *
	 *  - We store only 1*Q1, 2*Q1,... 8*Q1. The point to add will
	 *    be either one of these points, or the opposite of one of
	 *    these points. This "shifts" the window, and must be
	 *    counterbalanced by a constant offset applied to the scalar.
	 *
	 * Therefore:
	 *
	 *  1. Add 0x8888...888 to the scalar s, and normalize it to 0..n-1
	 *     (we do this by encoding the scalar to bytes).
	 *  2. Compute the window: j*Q1 (for j = 1..8).
	 *  3. Start with point Q3 = 0.
	 *  4. For i in 0..62:
	 *      - Compute Q3 <- 16*Q3
	 *      - Let e = bits[(248-4*i)..(251-4*i)] of s
	 *      - Let: T = -(8-e)*Q1  if 0 <= e <= 7
	 *             T = 0          if e == 8
	 *             T = (e-8)*Q1   if 9 <= e <= 15
	 *      - Compute Q3 <- Q3 + T
	 *
	 * All lookups should be done in constant-time, as well as additions
	 * and conditional negation of T.
	 *
	 * For the first iteration (i == 0), since Q3 is still 0 at that
	 * point, we can omit the multiplication by 16 and the addition,
	 * and simply set Q3 to T.
	 */
	curve9767_scalar ss;
	uint8_t sb[32];
	curve9767_point T;
	window_point8 window;
	int i;
	uint32_t qz;

	/*
	 * Apply offset on the scalar and encode it into bytes. This
	 * involves normalization to 0..n-1.
	 */
	curve9767_scalar_decode_strict(&ss,
		scalar_win4_off, sizeof scalar_win4_off);
	curve9767_scalar_add(&ss, &ss, s);
	curve9767_scalar_encode(sb, &ss);

	/*
	 * Create window contents.
	 */
	T = *Q1;
	for (i = 1; i <= 8; i ++) {
		if (i != 1) {
			curve9767_point_add(&T, &T, Q1);
		}
		curve9767_inner_window_put(&window, &T, i - 1);
	}

	/*
	 * Perform the chunk-by-chunk computation.
	 */
	qz = Q1->neutral;
	for (i = 0; i < 63; i ++) {
		uint32_t e;

		/*
		 * Extract exponent bits.
		 */
		e = (sb[(62 - i) >> 1] >> (((62 - i) & 1) << 2)) & 0x0F;

		/*
		 * Window lookup. Don't forget to adjust the neutral flag
		 * to account for the case of Q1 = infinity.
		 */
		do_lookup(&T, &window, e);
		T.neutral |= qz;

		/*
		 * Q3 <- 16*Q3 + T.
		 *
		 * If i == 0, then we know that Q3 is (conceptually) 0,
		 * and we can simply set Q3 to T.
		 */
		if (i == 0) {
			*Q3 = T;
		} else {
			curve9767_point_mul2k(Q3, Q3, 4);
			curve9767_point_add(Q3, Q3, &T);
		}
	}
}

/* see curve9767.h */
void
curve9767_point_mulgen(curve9767_point *Q3, const curve9767_scalar *s)
{
	/*
	 * We apply the same algorithm as curve9767_point_mul(), but
	 * with four 64-bit scalars instead of one 252-bit scalar,
	 * thus mutualizing the point doublings. This requires four
	 * precomputed windows (each has size 640 bytes, hence these
	 * tables account for 2560 bytes of ROM/Flash, which is
	 * tolerable).
	 */
	curve9767_scalar ss;
	uint8_t sb[32];
	curve9767_point T;
	int i;

	/*
	 * Apply offset on the scalar and encode it into bytes. This
	 * involves normalization to 0..n-1.
	 */
	curve9767_scalar_decode_strict(&ss,
		scalar_win4_off, sizeof scalar_win4_off);
	curve9767_scalar_add(&ss, &ss, s);
	curve9767_scalar_encode(sb, &ss);

	/*
	 * Perform the chunk-by-chunk computation. For each iteration,
	 * we use 4 chunks of 4 bits from the scalar, to make 4 lookups
	 * in the relevant precomputed windows.
	 *
	 * Since the source scalar is 252 bits, not 256 bits, the first
	 * loop iteration will not make a lookup in the highest window
	 * (the lookup bits are statically known to be 0). We specialize
	 * that first iteration out of the loop.
	 */
	do_lookup(Q3, &curve9767_inner_window_G, sb[7] >> 4);
	do_lookup(&T, &curve9767_inner_window_G64, sb[15] >> 4);
	curve9767_point_add(Q3, Q3, &T);
	do_lookup(&T, &curve9767_inner_window_G128, sb[23] >> 4);
	curve9767_point_add(Q3, Q3, &T);

	for (i = 1; i < 16; i ++) {
		uint32_t e0, e1, e2, e3;
		int j;

		/*
		 * Extract exponent bits.
		 */
		j = ((i + 1) & 1) << 2;
		e0 = (sb[(15 - i) >> 1] >> j) & 0x0F;
		e1 = (sb[(31 - i) >> 1] >> j) & 0x0F;
		e2 = (sb[(47 - i) >> 1] >> j) & 0x0F;
		e3 = (sb[(63 - i) >> 1] >> j) & 0x0F;

		/*
		 * Window lookups and additions.
		 */
		curve9767_point_mul2k(Q3, Q3, 4);
		do_lookup(&T, &curve9767_inner_window_G, e0);
		curve9767_point_add(Q3, Q3, &T);
		do_lookup(&T, &curve9767_inner_window_G64, e1);
		curve9767_point_add(Q3, Q3, &T);
		do_lookup(&T, &curve9767_inner_window_G128, e2);
		curve9767_point_add(Q3, Q3, &T);
		do_lookup(&T, &curve9767_inner_window_G192, e3);
		curve9767_point_add(Q3, Q3, &T);
	}
}

/* see curve9767.h */
void
curve9767_point_mul_mulgen_add(curve9767_point *Q3,
	const curve9767_point *Q1, const curve9767_scalar *s1,
	const curve9767_scalar *s2)
{
	curve9767_scalar ss;
	uint8_t sb1[32], sb2[32];
	curve9767_point T;
	window_point8 window;
	int i;
	uint32_t qz;

	/*
	 * Apply offset on both scalars and encode them into bytes. This
	 * involves normalization to 0..n-1.
	 */
	curve9767_scalar_decode_strict(&ss,
		scalar_win4_off, sizeof scalar_win4_off);
	curve9767_scalar_add(&ss, &ss, s1);
	curve9767_scalar_encode(sb1, &ss);
	curve9767_scalar_decode_strict(&ss,
		scalar_win4_off, sizeof scalar_win4_off);
	curve9767_scalar_add(&ss, &ss, s2);
	curve9767_scalar_encode(sb2, &ss);

	/*
	 * Create window contents (for Q1).
	 */
	T = *Q1;
	for (i = 1; i <= 8; i ++) {
		if (i != 1) {
			curve9767_point_add(&T, &T, Q1);
		}
		curve9767_inner_window_put(&window, &T, i - 1);
	}

	/*
	 * Perform the chunk-by-chunk computation.
	 */
	qz = Q1->neutral;
	for (i = 0; i < 63; i ++) {
		uint32_t e;

		/*
		 * Lookup for Q1. Don't forget to adjust the neutral flag
		 * of T, in case Q1 = infinity.
		 */
		e = (sb1[(62 - i) >> 1] >> (((62 - i) & 1) << 2)) & 0x0F;
		do_lookup(&T, &window, e);
		T.neutral |= qz;
		if (i == 0) {
			*Q3 = T;
		} else {
			curve9767_point_mul2k(Q3, Q3, 4);
			curve9767_point_add(Q3, Q3, &T);
		}

		/*
		 * Lookup for G.
		 */
		e = (sb2[(62 - i) >> 1] >> (((62 - i) & 1) << 2)) & 0x0F;
		do_lookup(&T, &curve9767_inner_window_G, e);
		curve9767_point_add(Q3, Q3, &T);
	}
}
