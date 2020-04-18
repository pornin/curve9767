/*
 * Reference implementation, portable C.
 *
 * All operations are implemented in plain C code. Multiplication operations
 * are over 32 bits (with only 32-bit results).
 */

#include "inner.h"

/* ====================================================================== */
/*
 * Base Field Functions (GF(9767))
 * -------------------------------
 *
 *   p     modulus (9767)
 *   p0i   1/p mod 2^32
 *   p1i   -1/p mod 2^32
 *   R     2^32 mod p
 *   R2    2^64 mod p
 *   R1I   1/(2^32) mod p
 *
 * Values in the base field use Montgomery representation: value x is
 * stored as x*R mod p, in the 1..p range (i.e. if x*R mod p = 0, then
 * it is stored as p, not 0).
 *
 * All mp_xxx() functions expect input values that follow that
 * representation, and return values in that representation.
 * Two of these functions have larger acceptable input ranges:
 *   mp_frommonty() expects the source value to be in the 1..3654952486 range.
 *   mp_tomonty() expects the source value to be in the 0..2278869 range.
 *
 * Even when inputs are incorrect (out of range), the functions are still
 * constant-time, and incur no undefined behaviour.
 */

#define P      9767
#define P0I    659614103
#define P1I    3635353193
#define R      7182
#define R2     1597
#define R1I    8267

/*
 * Some constants, in Montgomery representation.
 */
#define THREEm           (((uint32_t)3 * R) % P)
#define FOURm            (((uint32_t)4 * R) % P)
#define SIXm             (((uint32_t)6 * R) % P)
#define EIGHTm           (((uint32_t)8 * R) % P)
#define SIXTEENm         (((uint32_t)16 * R) % P)
#define TWENTYSEVENmm    (((((uint32_t)27 * R) % P) * R) % P)

#define MNINEm           (((uint32_t)(P - 9) * R) % P)
#define MFIFTYFOURm      (((uint32_t)(P - 54) * R) % P)

#define HALFm            (((uint32_t)((P + 1) >> 1) * R) % P)

#define NINEmm           (((((uint32_t)9 * R) % P) * R) % P)

#define ITHREEm          ((uint32_t)2394)
#define IMTWENTYSEVENm   ((uint32_t)9501)

/*
 * Compute a + b in the base field.
 */
static inline uint32_t
mp_add(uint32_t a, uint32_t b)
{
	uint32_t c;

	/* Compute -(a+b); this is in the -p..+p-2 range. */
	c = P - (a + b);

	/* We add P if the value is negative; now, c is in 0..p-1. */
	c += P & -(c >> 31);

	/* Negate c to obtain a+b in the 1..p range. */
	return P - c;
}

/*
 * Compute a - b in the base field.
 */
static inline uint32_t
mp_sub(uint32_t a, uint32_t b)
{
	uint32_t c;

	/* Compute b-a; this is in the -p+1..+p-1 range. */
	c = b - a;

	/* We add P if the value is negative; now, c is in 0..p-1. */
	c += P & -(c >> 31);

	/* Negate c to obtain a-b in the 1..p range. */
	return P - c;
}

/*
 * Given x in the 1..3654952486 range, compute x/R mod p.
 */
static inline uint32_t
mp_frommonty(uint32_t x)
{
	/*
	 * Principle: given input x, Montgomery reduction works thus:
	 *
	 *   - Let p1i = -1/p mod 2^32
	 *   - Set: y = x * p1i mod 2^32
	 *   - Compute: y <- (y*p + x) / 2^32
	 *
	 * Indeed, y*p = x * (-1/p) * p = -x mod 2^32
	 * Therefore, y*p + x = 0 mod 2^32, and the division by 2^32 is
	 * exact. Moreover, y*2^32 = x mod p.
	 *
	 * It so happens that within the range of inputs that we accept,
	 * the computations can be simplified. Write y as:
	 *    y = y0 + y1*2^16
	 * with 0 <= y0 < 2^16 and 0 <= y1 < 2^16 (i.e. y0 is the "low
	 * half" and y1 the "high half").
	 *
	 * Then:
	 *    y*p + x = x + y0*p + y1*p*2^16
	 * Since y1 < 2^16, we know that y1*p < p*2^16 < 2^32. Split
	 * y1*p into its low and high halves:
	 *    y1*p = y2 + y3*2^16
	 * This yields:
	 *    y*p + x = x + y0*p + y2*2^16 + y3*2^32
	 * We know that y0 <= 2^16-1 and y2 <= 2^16-1. Therefore:
	 *    x + y0*p + y2*2^16 <= x + (2^16-1)*(p+2^16)
	 * Thus, if x <= 2^33-1 - (2^16-1)*(p+2^16) = 3654952486, then:
	 *    x + y0*p + y2*2^16 <= 2^33-1
	 *
	 * We also know that x >= 1, and that y*p + x is a multiple of
	 * 2^32. Therefore, the only possibility is that:
	 *    x + y0*p + y2*2^16 = 2^32
	 * which yields:
	 *    y*p + x = 2^32 + y3*2^32
	 * and therefore:
	 *    (y*p + x) / 2^32 = 1 + y3
	 *
	 * Notice that y3 = floor(y1*p / 2^16), with y1 < 2^16. Therefore,
	 * y1*p / 2^16 < p, and y3 is in the 0..p-1 range. Thus, 1+y3
	 * is already in the proper range.
	 *
	 * (Experimentally, the 3654952486 bound is exact; applying the
	 * expression below on 3654952487 yields the wrong result.)
	 */
	return 1 + ((((uint32_t)(x * (uint32_t)P1I) >> 16)
		* (uint32_t)P) >> 16);
}

/*
 * Compute a * b in the base field.
 * This is a Montgomery multiplication; since a and b are represented
 * by integers a*R mod p and b*R mod p, then the Montgomery representation
 * of a*b is (a*R)*(b*R)/R mod p, which is computed with a simple
 * product followed by a Montgomery reduction.
 */
static inline uint32_t
mp_montymul(uint32_t a, uint32_t b)
{
	return mp_frommonty(a * b);
}

/*
 * Convert a source integer to Montgomery representation. The source
 * integer can be anywhere in the 0..2278869 range.
 */
static inline uint32_t
mp_tomonty(uint32_t x)
{
	/*
	 * x*2^64 / 2^32 = x / 2^32 mod p
	 *
	 * We add p to x so that a source value x == 0 is supported.
	 * If x <= 2278869, then (x+p)*R2 <= 3654951692, i.e. in the
	 * supported range for mp_frommonty().
	 */
	return mp_frommonty((x + P) * R2);
}

/*
 * Compute 1/x. If x is zero (i.e. x == p), this function returns
 * zero (i.e. p).
 */
static uint32_t
mp_inv(uint32_t x)
{
	uint32_t x8, x9, x152, x2441, xi;

	/* x8 = x^8 */
	x8 = mp_montymul(x, x);
	x8 = mp_montymul(x8, x8);
	x8 = mp_montymul(x8, x8);

	/* x9 = x^9 */
	x9 = mp_montymul(x, x8);

	/* x152 = x^(9*16+8) */
	x152 = mp_montymul(x9, x9);
	x152 = mp_montymul(x152, x152);
	x152 = mp_montymul(x152, x152);
	x152 = mp_montymul(x152, x152);
	x152 = mp_montymul(x152, x8);

	/* x2441 = x^((9*16+8)*16+9) */
	x2441 = mp_montymul(x152, x152);
	x2441 = mp_montymul(x2441, x2441);
	x2441 = mp_montymul(x2441, x2441);
	x2441 = mp_montymul(x2441, x2441);
	x2441 = mp_montymul(x2441, x9);

	/* xi = x^(((9*16+8)*16+9)*4+1) = x^9765 = x/y */
	xi = mp_montymul(x2441, x2441);
	xi = mp_montymul(xi, xi);
	xi = mp_montymul(xi, x);

	return xi;
}

/*
 * Test whether x is a quadratic residue modulo p. This returns 1 if x
 * is a QR (including if x is zero).
 */
static uint32_t
mp_is_qr(uint32_t x)
{
	uint32_t r, x19;

	/* x^9 = (x^8)*x */
	r = mp_montymul(x, x);
	r = mp_montymul(r, r);
	r = mp_montymul(r, r);
	r = mp_montymul(r, x);
	/* x^19 = ((x^9)^2)*x */
	r = mp_montymul(r, r);
	r = mp_montymul(r, x);
	x19 = r;
	/* x^4883 = ((x^19)^256)*(x^19) */
	r = mp_montymul(r, r);
	r = mp_montymul(r, r);
	r = mp_montymul(r, r);
	r = mp_montymul(r, r);
	r = mp_montymul(r, r);
	r = mp_montymul(r, r);
	r = mp_montymul(r, r);
	r = mp_montymul(r, r);
	r = mp_montymul(r, x19);

	/*
	 * Three possible values at this point:
	 *  - if x is zero, then r == 9767  (representation of 0)
	 *  - if x is a non-zero QR, then r == 7182  (representation of 1)
	 *  - if x is a non-zero non-QR, then r == 2585  (representation of -1)
	 */
	return (r + 1500) >> 13;
}

/* ====================================================================== */

/*
 * Extension field functions.
 *
 * We consider polynomials in GF(p)[z] of degree at most 18, i.e.
 * polynomials that fit in 19 coefficients in GF(p). The coefficients
 * are all in the Montgomery representation described above, and they
 * are in ascending order (i.e. if a = a0 + a1*z + a2*z^2 + ... + a18,
 * then a0 is in a[0], a1 in a[1], and so on).
 *
 * All computations are done modulo z^19-2. Since that polynomial is
 * irreducible over GF(p), we obtain a finite field of cardinal p^19.
 *
 * To speed up computations, we perform multiplication and squarings
 * over integer coefficients (really, modulo 2^32) and mutualize the
 * Montgomery reductions. Indeed, given two polynomials a and b of
 * degree at most 18, we can compute c = a*b mod z^19-2 as:
 *
 *   c0 = a0*b0 + 2 * (a1*b18 + a2*b17 + a3*b16 + ... + a18*b1)
 *   c1 = a0*b1 + a1*b0 + 2 * (a2*b18 + a3*b17 + ... + a18*b2)
 *   c2 = a0*b2 + a1*b1 + a2*b0 + 2 * (a3*b18 + a4*b17 + ... + a18*b3)
 *   ...
 *
 * If all coefficients of a and b are at most p (as integers), then all
 * coefficients of c will be at most 37*p^2 = 3529588693, which fits on
 * 32 bits and is furthermore within the acceptable range for
 * mp_frommonty().
 *
 * (Indeed, p = 9767 is the largest prime such that z^19-2 is
 * irreducible over GF(p) and 37*p^2 fits in the range acceptable by
 * mp_frommonty() for that p, which is why that value was chosen in the
 * first place.)
 */

/*
 * Helper macros to get local short names for functions.
 */
#define gf_add        curve9767_inner_gf_add
#define gf_sub        curve9767_inner_gf_sub
#define gf_neg        curve9767_inner_gf_neg
#define gf_mul        curve9767_inner_gf_mul
#define gf_sqr        curve9767_inner_gf_sqr
#define gf_inv        curve9767_inner_gf_inv
#define gf_sqrt       curve9767_inner_gf_sqrt
#define gf_cubert     curve9767_inner_gf_cubert
#define gf_eq         curve9767_inner_gf_eq
#define gf_is_neg     curve9767_inner_gf_is_neg

/* see inner.h */
const field_element curve9767_inner_gf_zero = {
	{ P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P }
};

/* see inner.h */
const field_element curve9767_inner_gf_one = {
	{ R, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P }
};

/* see inner.h */
void
curve9767_inner_gf_add(uint16_t *c, const uint16_t *a, const uint16_t *b)
{
	int i;

	for (i = 0; i < 19; i ++) {
		c[i] = (uint16_t)mp_add(a[i], b[i]);
	}
}

/* see inner.h */
void
curve9767_inner_gf_sub(uint16_t *c, const uint16_t *a, const uint16_t *b)
{
	int i;

	for (i = 0; i < 19; i ++) {
		c[i] = (uint16_t)mp_sub(a[i], b[i]);
	}
}

/* see inner.h */
void
curve9767_inner_gf_neg(uint16_t *c, const uint16_t *a)
{
	int i;

	for (i = 0; i < 19; i ++) {
		c[i] = (uint16_t)mp_sub(P, a[i]);
	}
}

/* see inner.h */
void
curve9767_inner_gf_condneg(uint16_t *c, uint32_t ctl)
{
	int i;
	uint32_t m;

	m = -ctl;
	for (i = 0; i < 19; i ++) {
		uint32_t wc;

		wc = c[i];
		wc ^= m & (wc ^ mp_sub(P, wc));
		c[i] = (uint16_t)wc;
	}
}

/*
 * Some macros for Karastuba fix-up. These macros assume the following:
 *
 *   - We are multiplying two polynomials a and b; they have been split
 *     into low and high halves (10 and 9 elements, respectively):
 *
 *        a = aL + aH*z^10
 *        b = bL + bH*z^10
 *
 *   - Three products have been assembled, in local arrays (uint32_t[]):
 *
 *        t1[19]: aL*bL
 *        t2[17]: aH*bH
 *        t3[19]: (aL+aH)*(bL+bH)
 *
 *     The actual implementation will ignore the top word of t3[] (the
 *     computation would involve computing t3[18] - t1[18], and that
 *     always yields 0 because aH and bH are shorter than aL and bL).
 *     Therefore, the caller needs not compute t3[18], and the t3[]
 *     array can be of length 18 instead.
 *
 *   - Output is called c[] (array of uint16_t).
 *
 * In all generality, we must compute:
 *
 *   c[i] = t1[i] + t3[i - 10] - t1[i - 10] - t2[i - 10]
 *        + 2*(t2[i - 1] + t3[i + 9] - t1[i + 9] - t2[i + 9])
 *
 * with the convention that tx[i] == 0 when i is out of range for that
 * array. The STEP_KFIX_* macros are invoked in a specific order that
 * allows mutualizing array accesses (i.e. values from t1[] and t2[]
 * read at one step are reused in the next step, so we can keep them
 * in local variable).
 */

/* Karatsuba fix-up step for i == 0 */
#define STEP_KFIX_P0(i)   do { \
		uint32_t nr1, nr3; \
		nr1 = t1[0]; \
		nr3 = t2[9]; \
		c[0] = (uint16_t)mp_frommonty( \
			nr1 + ((t3[9] - r1 - nr3) << 1)); \
		r1 = nr1; \
		r3 = nr3; \
	} while (0)

/* Karatsuba fix-up step for 1 <= i <= 7 */
#define STEP_KFIX_P1_7(i)   do { \
		uint32_t nr1, nr3; \
		nr1 = t1[(i)]; \
		nr3 = t2[(i) + 9]; \
		c[(i)] = (uint16_t)mp_frommonty( \
			nr1 + ((r3 + t3[(i) + 9] - r1 - nr3) << 1)); \
		r1 = nr1; \
		r3 = nr3; \
	} while (0)

/* Karatsuba fix-up step for i == 8 */
#define STEP_KFIX_P8(i)   do { \
		uint32_t nr1; \
		nr1 = t1[8]; \
		c[8] = (uint16_t)mp_frommonty( \
			nr1 + ((r3 + t3[17] - r1) << 1)); \
		r1 = nr1; \
	} while (0)

/* Karatsuba fix-up step for i == 9 */
#define STEP_KFIX_P9(i)   do { \
		uint32_t nr1; \
		nr1 = t1[9]; \
		c[9] = (uint16_t)mp_frommonty( \
			nr1 + (r3 << 1)); \
		r1 = nr1; \
	} while (0)

/* Karatsuba fix-up step for 10 <= i <= 17 */
#define STEP_KFIX_P10_17(i)   do { \
		uint32_t nr1, nr3; \
		nr1 = t1[(i)]; \
		nr3 = t2[(i) - 10]; \
		c[(i)] = (uint16_t)mp_frommonty( \
			nr1 + t3[(i) - 10] - r1 - nr3 + (r3 << 1)); \
		r1 = nr1; \
		r3 = nr3; \
	} while (0)

/* Karatsuba fix-up step for i == 18 */
#define STEP_KFIX_P18(i)   do { \
		uint32_t nr1, nr3; \
		nr1 = t1[18]; \
		nr3 = t2[8]; \
		c[18] = (uint16_t)mp_frommonty( \
			nr1 + t3[8] - r1 - nr3); \
		r1 = nr1; \
		r3 = nr3; \
	} while (0)

/*
 * Perform the full Karatsuba fix-up.
 */
#define KFIX   do { \
		uint32_t r1, r3; \
		r3 = t2[8]; \
		STEP_KFIX_P9(     9); \
		STEP_KFIX_P0(     0); \
		STEP_KFIX_P10_17(10); \
		STEP_KFIX_P1_7(   1); \
		STEP_KFIX_P10_17(11); \
		STEP_KFIX_P1_7(   2); \
		STEP_KFIX_P10_17(12); \
		STEP_KFIX_P1_7(   3); \
		STEP_KFIX_P10_17(13); \
		STEP_KFIX_P1_7(   4); \
		STEP_KFIX_P10_17(14); \
		STEP_KFIX_P1_7(   5); \
		STEP_KFIX_P10_17(15); \
		STEP_KFIX_P1_7(   6); \
		STEP_KFIX_P10_17(16); \
		STEP_KFIX_P1_7(   7); \
		STEP_KFIX_P10_17(17); \
		STEP_KFIX_P8(     8); \
		STEP_KFIX_P18(   18); \
		(void)r1; \
		(void)r3; \
	} while (0)

/* see inner.h */
void
curve9767_inner_gf_mul(uint16_t *c, const uint16_t *a, const uint16_t *b)
{
	/*
	 * We use one step of Karatsuba multiplication. Depending
	 * on the architecture and compiler, nested Karatsuba steps
	 * may or may not be a good idea, but at least one step is
	 * probably almost always a gain.
	 *
	 * Splitting a and b into a low part (ten elements) and a
	 * high part (nine elements), we have:
	 *
	 *   a = aL + aH*z^10
	 *   b = bL + bH*z^10
	 *   a*b = aL*bL + aH*bH*z^20 + ((aL+aH)*(bL+bH) - aL*bL - aH*bH)*z^10
	 */
	uint32_t t1[19], t2[17], t3[18], t4[10], t5[10];

	/*
	 * aL*bL -> t1
	 */
#define M1(i, j)   ((uint32_t)a[i] * (uint32_t)b[j])

	t1[0] = M1(0, 0);
	t1[1] = M1(0, 1) + M1(1, 0);
	t1[2] = M1(0, 2) + M1(1, 1) + M1(2, 0);
	t1[3] = M1(0, 3) + M1(1, 2) + M1(2, 1) + M1(3, 0);
	t1[4] = M1(0, 4) + M1(1, 3) + M1(2, 2) + M1(3, 1) + M1(4, 0);
	t1[5] = M1(0, 5) + M1(1, 4) + M1(2, 3) + M1(3, 2) + M1(4, 1)
		+ M1(5, 0);
	t1[6] = M1(0, 6) + M1(1, 5) + M1(2, 4) + M1(3, 3) + M1(4, 2)
		+ M1(5, 1) + M1(6, 0);
	t1[7] = M1(0, 7) + M1(1, 6) + M1(2, 5) + M1(3, 4) + M1(4, 3)
		+ M1(5, 2) + M1(6, 1) + M1(7, 0);
	t1[8] = M1(0, 8) + M1(1, 7) + M1(2, 6) + M1(3, 5) + M1(4, 4)
		+ M1(5, 3) + M1(6, 2) + M1(7, 1) + M1(8, 0);
	t1[9] = M1(0, 9) + M1(1, 8) + M1(2, 7) + M1(3, 6) + M1(4, 5)
		+ M1(5, 4) + M1(6, 3) + M1(7, 2) + M1(8, 1) + M1(9, 0);
	t1[10] = M1(1, 9) + M1(2, 8) + M1(3, 7) + M1(4, 6) + M1(5, 5)
		+ M1(6, 4) + M1(7, 3) + M1(8, 2) + M1(9, 1);
	t1[11] = M1(2, 9) + M1(3, 8) + M1(4, 7) + M1(5, 6) + M1(6, 5)
		+ M1(7, 4) + M1(8, 3) + M1(9, 2);
	t1[12] = M1(3, 9) + M1(4, 8) + M1(5, 7) + M1(6, 6) + M1(7, 5)
		+ M1(8, 4) + M1(9, 3);
	t1[13] = M1(4, 9) + M1(5, 8) + M1(6, 7) + M1(7, 6) + M1(8, 5)
		+ M1(9, 4);
	t1[14] = M1(5, 9) + M1(6, 8) + M1(7, 7) + M1(8, 6) + M1(9, 5);
	t1[15] = M1(6, 9) + M1(7, 8) + M1(8, 7) + M1(9, 6);
	t1[16] = M1(7, 9) + M1(8, 8) + M1(9, 7);
	t1[17] = M1(8, 9) + M1(9, 8);
	t1[18] = M1(9, 9);

#undef M1

	/*
	 * aH*bH -> t2
	 */
#define M2(i, j)   ((uint32_t)a[(i) + 10] * (uint32_t)b[(j) + 10])

	t2[0] = M2(0, 0);
	t2[1] = M2(0, 1) + M2(1, 0);
	t2[2] = M2(0, 2) + M2(1, 1) + M2(2, 0);
	t2[3] = M2(0, 3) + M2(1, 2) + M2(2, 1) + M2(3, 0);
	t2[4] = M2(0, 4) + M2(1, 3) + M2(2, 2) + M2(3, 1) + M2(4, 0);
	t2[5] = M2(0, 5) + M2(1, 4) + M2(2, 3) + M2(3, 2) + M2(4, 1)
		+ M2(5, 0);
	t2[6] = M2(0, 6) + M2(1, 5) + M2(2, 4) + M2(3, 3) + M2(4, 2)
		+ M2(5, 1) + M2(6, 0);
	t2[7] = M2(0, 7) + M2(1, 6) + M2(2, 5) + M2(3, 4) + M2(4, 3)
		+ M2(5, 2) + M2(6, 1) + M2(7, 0);
	t2[8] = M2(0, 8) + M2(1, 7) + M2(2, 6) + M2(3, 5) + M2(4, 4)
		+ M2(5, 3) + M2(6, 2) + M2(7, 1) + M2(8, 0);
	t2[9] = M2(1, 8) + M2(2, 7) + M2(3, 6) + M2(4, 5) + M2(5, 4)
		+ M2(6, 3) + M2(7, 2) + M2(8, 1);
	t2[10] = M2(2, 8) + M2(3, 7) + M2(4, 6) + M2(5, 5) + M2(6, 4)
		+ M2(7, 3) + M2(8, 2);
	t2[11] = M2(3, 8) + M2(4, 7) + M2(5, 6) + M2(6, 5) + M2(7, 4)
		+ M2(8, 3);
	t2[12] = M2(4, 8) + M2(5, 7) + M2(6, 6) + M2(7, 5) + M2(8, 4);
	t2[13] = M2(5, 8) + M2(6, 7) + M2(7, 6) + M2(8, 5);
	t2[14] = M2(6, 8) + M2(7, 7) + M2(8, 6);
	t2[15] = M2(7, 8) + M2(8, 7);
	t2[16] = M2(8, 8);

#undef M2

	/*
	 * aL+aH -> t4
	 * bL+bH -> t5
	 */
	t4[0] = (uint32_t)a[0] + (uint32_t)a[10];
	t4[1] = (uint32_t)a[1] + (uint32_t)a[11];
	t4[2] = (uint32_t)a[2] + (uint32_t)a[12];
	t4[3] = (uint32_t)a[3] + (uint32_t)a[13];
	t4[4] = (uint32_t)a[4] + (uint32_t)a[14];
	t4[5] = (uint32_t)a[5] + (uint32_t)a[15];
	t4[6] = (uint32_t)a[6] + (uint32_t)a[16];
	t4[7] = (uint32_t)a[7] + (uint32_t)a[17];
	t4[8] = (uint32_t)a[8] + (uint32_t)a[18];
	t4[9] = (uint32_t)a[9];
	t5[0] = (uint32_t)b[0] + (uint32_t)b[10];
	t5[1] = (uint32_t)b[1] + (uint32_t)b[11];
	t5[2] = (uint32_t)b[2] + (uint32_t)b[12];
	t5[3] = (uint32_t)b[3] + (uint32_t)b[13];
	t5[4] = (uint32_t)b[4] + (uint32_t)b[14];
	t5[5] = (uint32_t)b[5] + (uint32_t)b[15];
	t5[6] = (uint32_t)b[6] + (uint32_t)b[16];
	t5[7] = (uint32_t)b[7] + (uint32_t)b[17];
	t5[8] = (uint32_t)b[8] + (uint32_t)b[18];
	t5[9] = (uint32_t)b[9];

	/*
	 * (aL+aH)*(bL+bH) -> t3
	 * We do not compute the top word (t3[18]).
	 */
#define M3(i, j)   (t4[i] * t5[j])

	t3[0] = M3(0, 0);
	t3[1] = M3(0, 1) + M3(1, 0);
	t3[2] = M3(0, 2) + M3(1, 1) + M3(2, 0);
	t3[3] = M3(0, 3) + M3(1, 2) + M3(2, 1) + M3(3, 0);
	t3[4] = M3(0, 4) + M3(1, 3) + M3(2, 2) + M3(3, 1) + M3(4, 0);
	t3[5] = M3(0, 5) + M3(1, 4) + M3(2, 3) + M3(3, 2) + M3(4, 1)
		+ M3(5, 0);
	t3[6] = M3(0, 6) + M3(1, 5) + M3(2, 4) + M3(3, 3) + M3(4, 2)
		+ M3(5, 1) + M3(6, 0);
	t3[7] = M3(0, 7) + M3(1, 6) + M3(2, 5) + M3(3, 4) + M3(4, 3)
		+ M3(5, 2) + M3(6, 1) + M3(7, 0);
	t3[8] = M3(0, 8) + M3(1, 7) + M3(2, 6) + M3(3, 5) + M3(4, 4)
		+ M3(5, 3) + M3(6, 2) + M3(7, 1) + M3(8, 0);
	t3[9] = M3(0, 9) + M3(1, 8) + M3(2, 7) + M3(3, 6) + M3(4, 5)
		+ M3(5, 4) + M3(6, 3) + M3(7, 2) + M3(8, 1) + M3(9, 0);
	t3[10] = M3(1, 9) + M3(2, 8) + M3(3, 7) + M3(4, 6) + M3(5, 5)
		+ M3(6, 4) + M3(7, 3) + M3(8, 2) + M3(9, 1);
	t3[11] = M3(2, 9) + M3(3, 8) + M3(4, 7) + M3(5, 6) + M3(6, 5)
		+ M3(7, 4) + M3(8, 3) + M3(9, 2);
	t3[12] = M3(3, 9) + M3(4, 8) + M3(5, 7) + M3(6, 6) + M3(7, 5)
		+ M3(8, 4) + M3(9, 3);
	t3[13] = M3(4, 9) + M3(5, 8) + M3(6, 7) + M3(7, 6) + M3(8, 5)
		+ M3(9, 4);
	t3[14] = M3(5, 9) + M3(6, 8) + M3(7, 7) + M3(8, 6) + M3(9, 5);
	t3[15] = M3(6, 9) + M3(7, 8) + M3(8, 7) + M3(9, 6);
	t3[16] = M3(7, 9) + M3(8, 8) + M3(9, 7);
	t3[17] = M3(8, 9) + M3(9, 8);

#undef M3

	/*
	 * Karatsuba fix-up and Montgomery reductions.
	 */
	KFIX;
}

/* see inner.h */
void
curve9767_inner_gf_sqr(uint16_t *c, const uint16_t *a)
{
	/*
	 * If we split a into low part and high part, we have:
	 *
	 *   a = aL + aH*z^10
	 *
	 * which yields:
	 *
	 *   a*a = aL*aL + aH*aH*z^20 + 2*aL*aH*z^10
	 *
	 * On small platforms, it seems that the individual squarings
	 * are close to twice faster than generic multiplications,
	 * making it worthwhile to use instead:
	 *
	 *   a*a = aL*aL + aH*aH*z^20 + ((aL+aH)*(aL+aH) - aL*aL - aH*aH)*z^10
	 *
	 * We thus use a structure similar to gf_mul().
	 */
	uint32_t t1[19], t2[17], t3[18], t4[10];

	/*
	 * aL*aL -> t1
	 */
#define M1(i, j)   ((uint32_t)a[i] * (uint32_t)a[j])

	t1[0] = M1(0, 0);
	t1[1] = M1(0, 1) << 1;
	t1[2] = M1(1, 1) + (M1(0, 2) << 1);
	t1[3] = (M1(0, 3) + M1(1, 2)) << 1;
	t1[4] = M1(2, 2) + ((M1(0, 4) + M1(1, 3)) << 1);
	t1[5] = (M1(0, 5) + M1(1, 4) + M1(2, 3)) << 1;
	t1[6] = M1(3, 3) + ((M1(0, 6) + M1(1, 5) + M1(2, 4)) << 1);
	t1[7] = (M1(0, 7) + M1(1, 6) + M1(2, 5) + M1(3, 4)) << 1;
	t1[8] = M1(4, 4) + ((M1(0, 8) + M1(1, 7) + M1(2, 6) + M1(3, 5)) << 1);
	t1[9] = (M1(0, 9) + M1(1, 8) + M1(2, 7) + M1(3, 6) + M1(4, 5)) << 1;
	t1[10] = M1(5, 5) + ((M1(1, 9) + M1(2, 8) + M1(3, 7) + M1(4, 6)) << 1);
	t1[11] = (M1(2, 9) + M1(3, 8) + M1(4, 7) + M1(5, 6)) << 1;
	t1[12] = M1(6, 6) + ((M1(3, 9) + M1(4, 8) + M1(5, 7)) << 1);
	t1[13] = (M1(4, 9) + M1(5, 8) + M1(6, 7)) << 1;
	t1[14] = M1(7, 7) + ((M1(5, 9) + M1(6, 8)) << 1);
	t1[15] = (M1(6, 9) + M1(7, 8)) << 1;
	t1[16] = M1(8, 8) + (M1(7, 9) << 1);
	t1[17] = M1(8, 9) << 1;
	t1[18] = M1(9, 9);

#undef M1

	/*
	 * aH*aH -> t2
	 */
#define M2(i, j)   ((uint32_t)a[(i) + 10] * (uint32_t)a[(j) + 10])

	t2[0] = M2(0, 0);
	t2[1] = M2(0, 1) << 1;
	t2[2] = M2(1, 1) + (M2(0, 2) << 1);
	t2[3] = (M2(0, 3) + M2(1, 2)) << 1;
	t2[4] = M2(2, 2) + ((M2(0, 4) + M2(1, 3)) << 1);
	t2[5] = (M2(0, 5) + M2(1, 4) + M2(2, 3)) << 1;
	t2[6] = M2(3, 3) + ((M2(0, 6) + M2(1, 5) + M2(2, 4)) << 1);
	t2[7] = (M2(0, 7) + M2(1, 6) + M2(2, 5) + M2(3, 4)) << 1;
	t2[8] = M2(4, 4) + ((M2(0, 8) + M2(1, 7) + M2(2, 6) + M2(3, 5)) << 1);
	t2[9] = (M2(1, 8) + M2(2, 7) + M2(3, 6) + M2(4, 5)) << 1;
	t2[10] = M2(5, 5) + ((M2(2, 8) + M2(3, 7) + M2(4, 6)) << 1);
	t2[11] = (M2(3, 8) + M2(4, 7) + M2(5, 6)) << 1;
	t2[12] = M2(6, 6) + ((M2(4, 8) + M2(5, 7)) << 1);
	t2[13] = (M2(5, 8) + M2(6, 7)) << 1;
	t2[14] = M2(7, 7) + (M2(6, 8) << 1);
	t2[15] = M2(7, 8) << 1;
	t2[16] = M2(8, 8);

#undef M2

	/*
	 * aL+aH -> t4
	 */
	t4[0] = (uint32_t)a[0] + (uint32_t)a[10];
	t4[1] = (uint32_t)a[1] + (uint32_t)a[11];
	t4[2] = (uint32_t)a[2] + (uint32_t)a[12];
	t4[3] = (uint32_t)a[3] + (uint32_t)a[13];
	t4[4] = (uint32_t)a[4] + (uint32_t)a[14];
	t4[5] = (uint32_t)a[5] + (uint32_t)a[15];
	t4[6] = (uint32_t)a[6] + (uint32_t)a[16];
	t4[7] = (uint32_t)a[7] + (uint32_t)a[17];
	t4[8] = (uint32_t)a[8] + (uint32_t)a[18];
	t4[9] = (uint32_t)a[9];

	/*
	 * (aL+aH)*(aL+aH) -> t3
	 * We do not compute the top word (t3[18]).
	 */
#define M3(i, j)   (t4[i] * t4[j])

	t3[0] = M3(0, 0);
	t3[1] = M3(0, 1) << 1;
	t3[2] = M3(1, 1) + (M3(0, 2) << 1);
	t3[3] = (M3(0, 3) + M3(1, 2)) << 1;
	t3[4] = M3(2, 2) + ((M3(0, 4) + M3(1, 3)) << 1);
	t3[5] = (M3(0, 5) + M3(1, 4) + M3(2, 3)) << 1;
	t3[6] = M3(3, 3) + ((M3(0, 6) + M3(1, 5) + M3(2, 4)) << 1);
	t3[7] = (M3(0, 7) + M3(1, 6) + M3(2, 5) + M3(3, 4)) << 1;
	t3[8] = M3(4, 4) + ((M3(0, 8) + M3(1, 7) + M3(2, 6) + M3(3, 5)) << 1);
	t3[9] = (M3(0, 9) + M3(1, 8) + M3(2, 7) + M3(3, 6) + M3(4, 5)) << 1;
	t3[10] = M3(5, 5) + ((M3(1, 9) + M3(2, 8) + M3(3, 7) + M3(4, 6)) << 1);
	t3[11] = (M3(2, 9) + M3(3, 8) + M3(4, 7) + M3(5, 6)) << 1;
	t3[12] = M3(6, 6) + ((M3(3, 9) + M3(4, 8) + M3(5, 7)) << 1);
	t3[13] = (M3(4, 9) + M3(5, 8) + M3(6, 7)) << 1;
	t3[14] = M3(7, 7) + ((M3(5, 9) + M3(6, 8)) << 1);
	t3[15] = (M3(6, 9) + M3(7, 8)) << 1;
	t3[16] = M3(8, 8) + (M3(7, 9) << 1);
	t3[17] = M3(8, 9) << 1;

#undef M3

	/*
	 * Karatsuba fix-up and Montgomery reductions.
	 */
	KFIX;
}

/*
 * Apply the Frobenius operator to a[], result in c[]. c[] may be the same
 * array as a[]. The Frobenius coefficients are provided as f[]; the array
 * f[] has length 18.
 */
static void
gf_frob(uint16_t *c, const uint16_t *a, const uint16_t *f)
{
	/*
	 * The Frobenius operator is raising a to the power p^j for some
	 * integer j (all values j in the range 0 to 18 are possible).
	 * This is an automorphism, i.e.:
	 *
	 *    (a + b)^(p^j) = a^(p^j) + b^(p^j)
	 *    (a * b)^(p^j) = a^(p^j) * b^(p^j)
	 *
	 * for all field elements a and b. Since we compute modulo z^19-2,
	 * applying the Frobenius is equivalent to multiplying each
	 * coefficient by a constant; namely:
	 *
	 *    c[i] = a[i] * 2^(k*i*j)
	 *
	 * where k = (p-1)/19. We provide the 2^(k*i*j) in the array f[];
	 * we omit the first one, which has value 1.
	 */
	int i;

	c[0] = a[0];
	for (i = 0; i < 18; i ++) {
		c[i + 1] = (uint16_t)mp_montymul(a[i + 1], f[i]);
	}
}

/*
 * Precomputed constants for the Frobenius operator. Each array is for
 * raising the input to the power p^j, for j = 1, 2, 4, 8 or 9.
 *
 * Note that elements are in Montgomery representation. In each array,
 * the coefficients are exactly the 19-th roots of 1 in the field (the
 * 19th root, of value 1, is implicit), which is why the same values are
 * found in all arrays (but in a different order).
 */
static const uint16_t frob1[] = {
	3267, 5929, 2440,  449, 4794, 7615, 6585, 4354, 6093,
	7802, 1860, 5546, 8618, 8767, 5420, 1878, 2323, 6748
};
static const uint16_t frob2[] = {
	5929,  449, 7615, 4354, 7802, 5546, 8767, 1878, 6748,
	3267, 2440, 4794, 6585, 6093, 1860, 8618, 5420, 2323
};
static const uint16_t frob4[] = {
	 449, 4354, 5546, 1878, 3267, 4794, 6093, 8618, 2323,
	5929, 7615, 7802, 8767, 6748, 2440, 6585, 1860, 5420
};
static const uint16_t frob8[] = {
	4354, 1878, 4794, 8618, 5929, 7802, 6748, 6585, 5420,
	 449, 5546, 3267, 6093, 2323, 7615, 8767, 2440, 1860
};
static const uint16_t frob9[] = {
	6093, 6748, 4354, 2323, 6585, 1878, 7615, 5420, 4794,
	8767,  449, 8618, 2440, 5546, 5929, 1860, 3267, 7802
};

/* see inner.h */
void
curve9767_inner_gf_inv(uint16_t *c, const uint16_t *a)
{
	/*
	 * We use the Itoh-Tsuji algorithm.
	 *
	 * Let r = (p^19 - 1) / (p - 1)
	 *       = 1 + p + p^2 + p^3 + ... + p^17 + p^18
	 *
	 * For all non-zero field elements x, we have:
	 *
	 *   (x^r)^(p-1) = x^(p^19-1) = 1
	 *
	 * Thus, x^r is a p-1-th root of 1. There cannot be more than
	 * p-1 such roots (because we are in a field), and all non-zero
	 * elements of the base field GF(p) are p-1-th roots of 1. Thus,
	 * p-1-th roots of 1 are exactly the non-zero elements of GF(p). It
	 * follows that x^r, for any x != 0, is a non-zero element of GF(p).
	 *
	 * We can then compute:
	 *
	 *   1/x = (x^(r-1)) / (x^r)
	 *
	 * The inverse of x^r is easy to compute, since it is part of
	 * GF(p) (we use an exponentiation modulo p, with exponent p-2).
	 * To compute x^(r-1), we use the fact that raising any value to
	 * the power p^j (for any j in 0..18) is applying the Frobenius
	 * operator, which is a simple coefficient-wise multiplication
	 * with constants (see the gf_frob() function). Leveraging the
	 * special format of r-1, we can compute x^(r-1) in as few as
	 * five multiplications and six Frobenius invocations.
	 *
	 * Since we only use multiplications throughout, it is easy to
	 * see that if the input is zero, the output will be zero.
	 */
	field_element t1, t2;
	uint32_t y, yi;
	int i;

	/* a^(1+p) -> t1 */
	gf_frob(t2.v, a, frob1);
	gf_mul(t1.v, t2.v, a);

	/* a^(1+p+p^2+p^3) -> t1 */
	gf_frob(t2.v, t1.v, frob2);
	gf_mul(t1.v, t2.v, t1.v);

	/* a^(1+p+p^2+p^3+p^4+p^5+p^6+p^7) -> t1 */
	gf_frob(t2.v, t1.v, frob4);
	gf_mul(t1.v, t2.v, t1.v);

	/* a^(1+p+p^2+p^3+p^4+p^5+p^6+p^7+p^8) -> t1 */
	gf_frob(t1.v, t1.v, frob1);
	gf_mul(t1.v, t1.v, a);

	/* a^(1+p+p^2+p^3+..+p^17) -> t1 */
	gf_frob(t2.v, t1.v, frob9);
	gf_mul(t1.v, t2.v, t1.v);

	/* a^(p+p^2+p^3+..+p^17+p^18) = a^(r-1) -> t1 */
	gf_frob(t1.v, t1.v, frob1);

	/*
	 * Compute a^r = a*a^(r-1). Since a^r is in GF(p), we just need
	 * to compute the low coefficients (all others are zero).
	 */
	y = 0;
	for (i = 1; i < 19; i ++) {
		y += (uint32_t)a[i] * (uint32_t)t1.v[19 - i];
	}
	y <<= 1;
	y += (uint32_t)a[0] * (uint32_t)t1.v[0];
	y = mp_frommonty(y);

	/*
	 * Compute 1/(a^r) = (a^r)^(p-2).
	 */
	yi = mp_inv(y);

	/*
	 * Now compute 1/a = (1/(a^r))*(a^(r-1)).
	 */
	for (i = 0; i < 19; i ++) {
		c[i] = (uint16_t)mp_montymul(yi, t1.v[i]);
	}
}

/* see inner.h */
uint32_t
curve9767_inner_gf_sqrt(uint16_t *c, const uint16_t *a)
{
	/*
	 * Since p^19 = 3 mod 4, a square root is obtained by raising
	 * the input to the power (p^19+1)/4. We can write that exponent
	 * as:
	 *
	 *    (p^19+1)/4 = ((p+1)/4)*(2*e-r)
	 *
	 * where:
	 *
	 *   d =               1 + p^2 + p^4 + ... + p^14 + p^16
	 *   e = 1 + (p^2)*d = 1 + p^2 + p^4 + ... + p^14 + p^16 + p^18
	 *   f = p*d         = p + p^3 + p^5 + ... + p^15 + p^17
	 *   r = e + f       = 1 + p + p^2 + p^3 + ... + p^17 + p^18
	 *
	 * Therefore:
	 *
	 *    sqrt(a) = (((a^e)^2)/(a^r))^((p+1)/4)
	 *
	 * Note that a^r is in the base field GF(p) (see gf_inv() for
	 * details on that); thus, it is easy to invert.
	 */
	field_element t1, t2, t3;
	uint32_t y, yi, r;
	int i;

	/* a^(1+p^2) -> t1 */
	gf_frob(t2.v, a, frob2);
	gf_mul(t1.v, t2.v, a);

	/* a^(1+p^2+p^4+p^6) -> t1 */
	gf_frob(t2.v, t1.v, frob4);
	gf_mul(t1.v, t2.v, t1.v);

	/* a^(1+p^2+p^4+p^6+p^8+p^10+p^12+p^14) -> t1 */
	gf_frob(t2.v, t1.v, frob8);
	gf_mul(t1.v, t2.v, t1.v);

	/* a^(1+p^2+p^4+p^6+p^8+p^10+p^12+p^14+p^16) = a^d -> t1 */
	gf_frob(t2.v, t1.v, frob2);
	gf_mul(t1.v, t2.v, a);

	/* (a^d)^p = a^f -> t2 */
	gf_frob(t2.v, t1.v, frob1);

	/* a^e = a*((a^f)^p) -> t1 */
	gf_frob(t1.v, t2.v, frob1);
	gf_mul(t1.v, t1.v, a);

	/*
	 * Compute a^r = (a^e)*(a^f). This is an element of GF(p), so
	 * we compute only the low coefficient.
	 */
	y = 0;
	for (i = 1; i < 19; i ++) {
		y += (uint32_t)t1.v[i] * (uint32_t)t2.v[19 - i];
	}
	y <<= 1;
	y += (uint32_t)t1.v[0] * (uint32_t)t2.v[0];
	y = mp_frommonty(y);

	/*
	 * a is a QR if and only if a^r is a QR.
	 */
	r = mp_is_qr(y);

	/*
	 * If the square root is not actually needed, but only the QR
	 * status, then return it now.
	 */
	if (c == NULL) {
		return r;
	}

	/*
	 * Compute 1/(a^r).
	 */
	yi = mp_inv(y);

	/* (a^e)^2 -> t1 */
	gf_sqr(t1.v, t1.v);

	/* ((a^e)^2)/(a^r) -> t2 */
	for (i = 0; i < 19; i ++) {
		t2.v[i] = (uint16_t)mp_montymul(t1.v[i], yi);
	}

	/*
	 * Let x = ((a^e)^2)/(a^r) (currently in t2). All that remains
	 * to do is to raise x to the power (p+1)/4 = 2442.
	 */

	/* x^4 -> t1 */
	gf_sqr(t1.v, t2.v);
	gf_sqr(t1.v, t1.v);

	/* x^5 -> t3 */
	gf_mul(t3.v, t1.v, t2.v);

	/* x^9 -> t1 */
	gf_mul(t1.v, t1.v, t3.v);

	/* x^18 -> t1 */
	gf_sqr(t1.v, t1.v);

	/* x^19 -> t1 */
	gf_mul(t1.v, t1.v, t2.v);

	/* x^(19*64) = x^1216 -> t1 */
	gf_sqr(t1.v, t1.v);
	gf_sqr(t1.v, t1.v);
	gf_sqr(t1.v, t1.v);
	gf_sqr(t1.v, t1.v);
	gf_sqr(t1.v, t1.v);
	gf_sqr(t1.v, t1.v);

	/* x^1221 -> t1 */
	gf_mul(t1.v, t1.v, t3.v);

	/* x^2442 -> out */
	gf_sqr(c, t1.v);

	/*
	 * Return the quadratic residue status.
	 */
	return r;
}

/* see inner.h */
void
curve9767_inner_gf_cubert(uint16_t *c, const uint16_t *a)
{
	/*
	 * Since p^19 = 2 mod 3, a square root is obtained by raising
	 * the input to the power (2*p^19-1)/3. We can write that exponent
	 * as:
	 *
	 *    (2*p^19-1)/3 = ((2*p-1)/3)*e + ((p-2)/3)*f
	 *
	 * where:
	 *
	 *   d =               1 + p^2 + p^4 + ... + p^14 + p^16
	 *   e = 1 + (p^2)*d = 1 + p^2 + p^4 + ... + p^14 + p^16 + p^18
	 *   f = p*d         = p + p^3 + p^5 + ... + p^15 + p^17
	 *
	 * Thus, we compute (a^e)^((2*p-1)/3) * (a^f)^((p-2)/3). Both a^e
	 * and a^f are computed with a few multiplications and Frobenius
	 * operators.
	 */
	field_element t1, t2, t3, t4;

	/* a^(1+p^2) -> t1 */
	gf_frob(t2.v, a, frob2);
	gf_mul(t1.v, t2.v, a);

	/* a^(1+p^2+p^4+p^6) -> t1 */
	gf_frob(t2.v, t1.v, frob4);
	gf_mul(t1.v, t2.v, t1.v);

	/* a^(1+p^2+p^4+p^6+p^8+p^10+p^12+p^14) -> t1 */
	gf_frob(t2.v, t1.v, frob8);
	gf_mul(t1.v, t2.v, t1.v);

	/* a^(1+p^2+p^4+p^6+p^8+p^10+p^12+p^14+p^16) = a^d -> t1 */
	gf_frob(t2.v, t1.v, frob2);
	gf_mul(t1.v, t2.v, a);

	/* (a^d)^p = a^f -> t2 */
	gf_frob(t2.v, t1.v, frob1);

	/* a^e = a*((a^f)^p) -> t1 */
	gf_frob(t1.v, t2.v, frob1);
	gf_mul(t1.v, t1.v, a);

	/*
	 * Compute (a^e)^((2*p-1)/3) * (a^f)^((p-2)/3). Since we have:
	 *   (2*p-1)/3 = 2 * ((p-2)/3) + 1
	 * we can simply compute:
	 *   (a^e) * ((a^(2*e+f))^((p-2)/3))
	 */

	/* u = a^(2*e+f) -> t2 */
	gf_sqr(t3.v, t1.v);
	gf_mul(t2.v, t2.v, t3.v);

	/* u^3 -> t3 */
	gf_sqr(t3.v, t2.v);
	gf_mul(t3.v, t2.v, t3.v);
	/* u^12 -> t4 */
	gf_sqr(t4.v, t3.v);
	gf_sqr(t4.v, t4.v);
	/* u^13 -> t2 */
	gf_mul(t2.v, t4.v, t2.v);
	/* u^25 -> t4 */
	gf_mul(t4.v, t2.v, t4.v);
	/* u^813 -> t4 */
	gf_sqr(t4.v, t4.v);
	gf_sqr(t4.v, t4.v);
	gf_sqr(t4.v, t4.v);
	gf_sqr(t4.v, t4.v);
	gf_sqr(t4.v, t4.v);
	gf_mul(t4.v, t2.v, t4.v);
	/* u^3255 -> t4 */
	gf_sqr(t4.v, t4.v);
	gf_sqr(t4.v, t4.v);
	gf_mul(t4.v, t3.v, t4.v);

	/*
	 * Final product by a^e.
	 */
	gf_mul(c, t1.v, t4.v);
}

/* see inner.h */
uint32_t
curve9767_inner_gf_is_neg(const uint16_t *a)
{
	int i;
	uint32_t cc, t;

	/*
	 * Get the most significant non-zero element. Elements of value 0
	 * are represented as p, so we can subtract p: this will yield a
	 * negative value except for p.
	 */
	t = 0;
	cc = (uint32_t)-1;
	for (i = 18; i >= 0; i --) {
		uint32_t w, wnz;

		w = a[i];
		wnz = -((w - P) >> 31);
		t |= cc & wnz & w;
		cc &= ~wnz;
	}

	/*
	 * We must convert the value back from Montgomery representation.
	 * If the value is non-zero, the conversion will yield an integer
	 * in the 1..p-1 range; if it is zero (which will mean t == 0 at
	 * this point), then we want the integer 0.
	 *
	 * Since t is in the 0..p-1 range, -t will have its top 16 bits
	 * equal to 1 if t != 0.
	 */
	t = mp_frommonty(t) & ((-t) >> 16);

	/*
	 * Value is "negative" if strictly greater than (p-1)/2.
	 */
	return (((P - 1) >> 1) - t) >> 31;
}

/* see inner.h */
uint32_t
curve9767_inner_gf_eq(const uint16_t *a, const uint16_t *b)
{
	int i;
	uint32_t r;

	r = 0;
	for (i = 0; i < 19; i ++) {
		r |= -(a[i] ^ b[i]);
	}
	return 1 - (r >> 31);
}

/* see inner.h */
void
curve9767_inner_gf_encode(void *dst, const uint16_t *a)
{
	/*
	 * Encoding format is optimized, so that the value fits in
	 * 32 bytes. Elements 0..17 are grouped by three into
	 * 5-byte values; if the elements are x0, x1 and x2, then
	 * the following 40-bit integer is computed:
	 *
	 *   (x0 & 0x7FF) | ((x1 & 0x7FF) << 11) | ((x2 & 0x7FF) << 22)
	 *   | (((x0 >> 11) + 5 * (x1 >> 11) + 25 * (x2 >> 11)) << 33)
	 *
	 * In other words, we first encode 11 bits for each of the three
	 * values, and then we group the three upper parts (each in the
	 * 0..4 range) into a 7-bit integer in the 0..124 range.
	 *
	 * Each 40-bit value is encoded as 5 bytes with little-endian
	 * convention. Thus, elements 0..17 yield 30 bytes. The top
	 * element (index 18) is encoded in the two last bytes, in
	 * little-endian convention.
	 */
	uint8_t *buf;
	uint32_t t[19];
	int i;

	buf = dst;

	/*
	 * Normalize all coefficients: convert back from Montgomery
	 * representation, and make sure that zero is zero, not p.
	 */
	for (i = 0; i < 19; i ++) {
		uint32_t w;

		w = mp_frommonty(a[i]);
		w &= -((w - P) >> 31);
		t[i] = w;
	}

	/*
	 * Encode the first 18 elements.
	 */
	for (i = 0; i < 6; i ++) {
		uint32_t x0, x1, x2, s;

		x0 = t[3 * i + 0];
		x1 = t[3 * i + 1];
		x2 = t[3 * i + 2];
		s = (x0 >> 11) + 5 * (x1 >> 11) + 25 * (x2 >> 11);
		buf[5 * i + 0] = (uint8_t)x0;
		buf[5 * i + 1] = (uint8_t)(((x0 >> 8) & 0x07) | (x1 << 3));
		buf[5 * i + 2] = (uint8_t)(((x1 >> 5) & 0x3F) | (x2 << 6));
		buf[5 * i + 3] = (uint8_t)(x2 >> 2);
		buf[5 * i + 4] = (uint8_t)(((x2 >> 10) & 0x01) | (s << 1));
	}

	/*
	 * Encode the top element.
	 */
	buf[30] = (uint8_t)t[18];
	buf[31] = (uint8_t)(t[18] >> 8);
}

/* see inner.h */
uint32_t
curve9767_inner_gf_decode(uint16_t *c, const void *src)
{
	const uint8_t *buf;
	int i;
	uint32_t r, w;

	buf = src;

	/*
	 * Get the first 18 elements.
	 */
	r = 0;
	for (i = 0; i < 6; i ++) {
		uint32_t x0, x1, x2, d;

		w = buf[5 * i + 0];
		x0 = w;
		w = buf[5 * i + 1];
		x0 |= (w & 0x07) << 8;
		x1 = w >> 3;
		w = buf[5 * i + 2];
		x1 |= (w & 0x3F) << 5;
		x2 = w >> 6;
		w = buf[5 * i + 3];
		x2 |= w << 2;
		w = buf[5 * i + 4];
		x2 |= (w & 0x01) << 10;

		w >>= 1;

		/*
		 * For all values x in 0..127, floor(x/5) = floor(x*103/512).
		 */
		d = (w * 103) >> 9;
		x0 += (w - 5 * d) << 11;
		w = (d * 103) >> 9;
		x1 += (d - 5 * w) << 11;
		x2 += w << 11;

		/*
		 * All values must be between 0 and p-1. If any is not in
		 * range, then r will be set to a negative value (high bit
		 * set).
		 */
		r |= P - 1 - x0;
		r |= P - 1 - x1;
		r |= P - 1 - x2;

		/*
		 * Convert to Montgomery representation (in the 1..p range).
		 */
		c[3 * i + 0] = (uint16_t)mp_tomonty(x0);
		c[3 * i + 1] = (uint16_t)mp_tomonty(x1);
		c[3 * i + 2] = (uint16_t)mp_tomonty(x2);
	}

	/*
	 * Last element is encoded as-is.
	 */
	w = (uint32_t)buf[30] + (((uint32_t)buf[31] & 0x3F) << 8);
	r |= P - 1 - w;
	c[18] = (uint16_t)mp_tomonty(w);

	return 1 - (r >> 31);
}

/*
 * If ctl == 1, value a is copied into c.
 * If ctl == 0, value c is unmodified.
 */
static void
gf_condcopy(uint16_t *c, const uint16_t *a, uint32_t ctl)
{
	int i;
	uint32_t m;

	m = -ctl;
	for (i = 0; i < 19; i ++) {
		uint32_t wa, wc;

		wa = a[i];
		wc = c[i];
		c[i] = (uint16_t)(wc ^ (m & (wa ^ wc)));
	}
}

/* see inner.h */
void
curve9767_inner_gf_map_to_base(uint16_t *c, const void *src)
{
	const uint8_t *buf;
	uint32_t x[24];
	int i;

	/*
	 * Decode the 48 bytes into a big integer x[] (16-bit limbs).
	 */
	buf = src;
	for (i = 0; i < 24; i ++) {
		x[i] = (uint32_t)buf[i << 1]
			| ((uint32_t)buf[(i << 1) + 1] << 8);
	}

	/*
	 * Get coefficients one by one. As the loop advances, the value
	 * x[] shrinks, allowing us to skip the last words.
	 */
	for (i = 0; i < 19; i ++) {
		uint32_t r;
		int j;

		/*
		 * max_xw[i] = min { j | floor(x / (p^i)) < 2^(16*j) }
		 * This is the maximum size, in 16-bit words, of the value
		 * x[] when starting iteration number i.
		 */
		static const uint8_t max_xw[] = {
			24, 24, 23, 22, 21, 20, 20, 19, 18, 17,
			16, 15, 15, 14, 13, 12, 11, 10, 10
		};

		/*
		 * Divide x by p, word by word.
		 * Each limb is up to 65535, to which is added the current
		 * remainder, scaled up, for a maximum dividend:
		 *   65535 + (9766 << 16) = 640090111
		 *
		 * We obtain a constant-time division of the dividend d
		 * with these two facts:
		 *
		 *   - Montgomery reduction of d yields (d/(2^32)) mod p;
		 *     with an extra Montgomery multiplication by 2^64 mod p,
		 *     we get d mod p.
		 *
		 *   - Let e = d - (d mod p). This is a multiple of p;
		 *     then: e * (1/p mod 2^32) = e/p mod 2^32
		 */
		r = 0;
		for (j = max_xw[i] - 1; j >= 0; j --) {
			uint32_t d;

			d = (r << 16) + x[j];

			/*
			 * We must add P to d, because mp_frommonty()
			 * cannot tolerate an input of value 0.
			 */
			r = mp_frommonty(d + P);

			/*
			 * Now r = (d/(2^32)) mod p, in the 1..p range.
			 * We need d mod p, in the 0..p-1 range. We first
			 * add R1I = (1/(2^32)) mod p, which yields
			 * (d+1)/(2^32) mod p. Then, we do a Montgomery
			 * multiplication with R2 = 2^64 mod p, yielding
			 * (d+1) mod p in the 1..p range. Finally, we
			 * subtract 1, and get d mod p in the 0..p-1 range.
			 */
			r = mp_montymul(r + R1I, R2) - 1;

			/*
			 * Divide e = (d-r) by p. This is an exact division,
			 * which can be done by mutiplying (modulo 2^32)
			 * with 1/p mod 2^32 (see section 9 of: T. Grandlund
			 * and P. Montgomery, "Division by Invariant Integers
			 * using Multiplication", SIGPLAN'94; section 9).
			 */
			x[j] = (d - r) * P0I;
		}

		/*
		 * Last remainder is x mod p; this is our next coefficients.
		 * We want it in Montgomery representation.
		 */
		c[i] = (uint16_t)mp_tomonty(r);
	}
}

/* ====================================================================== */
/*
 * Curve9767 Functions
 * -------------------
 *
 * Curve equation is Y^2 = X^3 + A*X + B, with A = -3 and B = 2048*z^9.
 * Its order is:
 *  6389436622109970582043832278503799542449455630003248488928817956373993578097
 * The quadratic twist has equation Y^2 = X^3 + A*X - B, with order:
 *  6389436622109970582043832278503799542456269640437724557045939388995057075711
 * Both orders are prime.
 *
 * In this implementation, we use affine coordinates and compressed
 * encoding: the representation, as bytes, of a point, is the encoding
 * of the X coordinate; we also use one of the two free bits in the last
 * byte to encode the "sign" of Y: coordinate Y is said to be "negative"
 * if its top-most non-zero coefficient is in the (p+1)/2..(p-1) range
 * (field element zero is not considered negative, but there is no curve
 * point with Y=0 anyway).
 */

#define A        ((uint32_t)(P - 3))
#define Am       ((A * R) % P)
#define B        ((uint32_t)2048)
#define Bm       ((B * R) % P)
#define Bi       9

#define EIGHTEENBm           ((18 * Bm) % P)
#define THIRTYSIXBm          ((36 * Bm) % P)
#define SEVENTYTWOBm         ((72 * Bm) % P)
#define HUNDREDFORTYFOURBm   ((144 * Bm) % P)

#define MEIGHTBm             (((P - 8) * Bm) % P)
#define MSIXTEENBm           (((P - 16) * Bm) % P)

#define MTWENTYSEVENBBmm     (((((P - 27) * Bm) % P) * Bm) % P)

/*
 * Conventional generator is: G = (0, 32*z^14)
 */
#define GYm      (((uint32_t)32 * R) % P)

/* see curve9767.h */
const curve9767_point curve9767_generator = {
	0,  /* neutral */
	{ P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P },    /* x */
	0,  /* dummy1 */
	{ P, P, P, P, P, P, P, P, P, P, P, P, P, P, GYm, P, P, P, P },  /* y */
	0   /* dummy2 */
};

/* see inner.h */
uint32_t
curve9767_inner_make_y(uint16_t *y, const uint16_t *x, uint32_t neg)
{
	uint32_t r, m;
	field_element t1;
	int i;

	/*
	 * Compute Y^2 = X^3 - 3*X + B (in t1)
	 */
	gf_sqr(t1.v, x);
	gf_mul(t1.v, t1.v, x);
	for (i = 0; i < 19; i ++) {
		t1.v[i] = (uint16_t)mp_add(t1.v[i], mp_montymul(x[i], Am));
	}
	t1.v[Bi] = mp_add(t1.v[Bi], Bm);

	/*
	 * Extract Y as a square root (y). If t1 is not a QR, then the
	 * decoding process will report a failure.
	 */
	r = gf_sqrt(y, t1.v);

	/*
	 * If the "sign" of Y does not match the sign bit in the encoding,
	 * then Y must be replaced with -Y.
	 */
	m = -(gf_is_neg(y) ^ neg);
	for (i = 0; i < 19; i ++) {
		uint32_t w;

		w = y[i];
		w ^= m & (w ^ mp_sub(P, w));
		y[i] = (uint16_t)w;
	}

	return r;
}

/* see curve9767.h */
void
curve9767_point_add(curve9767_point *Q3,
	const curve9767_point *Q1, const curve9767_point *Q2)
{
	uint32_t ex, ey, n0, n1, n2;
	field_element t1, t2, t3;
	int i;

	/*
	 * General case:
	 *   lambda = (y2-y1)/(x2-x1)
	 * Special cases:
	 *   Q1 = 0 and Q2 = 0: Q3 = 0
	 *   Q1 = 0: Q3 = Q2
	 *   Q2 = 0: Q3 = Q1
	 *   Q1 = -Q2: Q3 = 0
	 *   Q1 = Q2: lambda = (3*x1^2+a)/(2*y1)
	 */

	/*
	 * Compute lambda in t1.
	 * If x1 == x2, then we assume that this is a double (Q1 == Q2).
	 */
	ex = gf_eq(Q1->x, Q2->x);
	ey = gf_eq(Q1->y, Q2->y);
	gf_sub(t1.v, Q2->x, Q1->x);
	gf_add(t3.v, Q1->y, Q1->y);
	gf_condcopy(t1.v, t3.v, ex);
	gf_sub(t2.v, Q2->y, Q1->y);
	gf_sqr(t3.v, Q1->x);
	for (i = 0; i < 19; i ++) {
		t3.v[i] = (uint16_t)mp_montymul(t3.v[i], THREEm);
	}
	t3.v[0] = (uint16_t)mp_add(t3.v[0], Am);
	gf_condcopy(t2.v, t3.v, ex);
	gf_inv(t1.v, t1.v);
	gf_mul(t1.v, t1.v, t2.v);

	/*
	 * x3 = lambda^2 - x1 - x2  (in t2)
	 * y3 = lambda*(x1 - x3) - y1  (in t3)
	 */
	gf_sqr(t2.v, t1.v);
	gf_sub(t2.v, t2.v, Q1->x);
	gf_sub(t2.v, t2.v, Q2->x);
	gf_sub(t3.v, Q1->x, t2.v);
	gf_mul(t3.v, t3.v, t1.v);
	gf_sub(t3.v, t3.v, Q1->y);

	/*
	 * If Q1 != 0, Q2 != 0, and Q1 != -Q2, then (t2,t3) contains the
	 * correct result.
	 */
	n1 = -(Q1->neutral);
	n2 = -(Q2->neutral);
	n0 = ~(n1 | n2);
	for (i = 0; i < 19; i ++) {
		uint32_t w;

		/*
		 * If either Q1 or Q2 is zero, then we use the coordinates
		 * from the other one. If neither is zero, then we use
		 * the values we computed in t2 and t3.
		 */
		w = (uint32_t)t2.v[i] & n0;
		w |= n2 & (uint32_t)Q1->x[i];
		w |= n1 & (uint32_t)Q2->x[i];
		Q3->x[i] = (uint16_t)w;

		w = (uint32_t)t3.v[i] & n0;
		w |= n2 & (uint32_t)Q1->y[i];
		w |= n1 & (uint32_t)Q2->y[i];
		Q3->y[i] = (uint16_t)w;
	}

	/*
	 * We handled all the following cases:
	 *   Q1 != 0 and Q2 != 0 and Q1 != Q2 and Q1 != -Q2
	 *   Q1 != 0 and Q1 == Q2
	 *   Q1 == 0 and Q2 != 0
	 *   Q1 != 0 and Q2 == 0
	 * Only remaining cases are those that yield a 0 result:
	 *   Q1 == -Q2
	 *   Q1 == 0 and Q2 == 0
	 * We cannot have a 0 otherwise, since there is no point of
	 * order 2 on the curve.
	 */
	Q3->neutral = (Q1->neutral & Q2->neutral)
		| ((1 - Q1->neutral) & (1 - Q2->neutral) & ex & (1 - ey));
}

/* see curve9767.h */
void
curve9767_point_mul2k(curve9767_point *Q3,
	const curve9767_point *Q1, unsigned k)
{
	field_element X, Y, Z, XX, YY, YYYY, ZZ, S, M;
	int i;
	unsigned cc;

	if (k == 0) {
		*Q3 = *Q1;
		return;
	}
	if (k == 1) {
		curve9767_point_add(Q3, Q1, Q1);
		return;
	}

	/*
	 * If Q1 = infinity, then Q3 = infinity.
	 * If Q1 != infinity, then Q3 != infinity (there is no point with
	 * an even order on the curve).
	 */
	Q3->neutral = Q1->neutral;

	/*
	 * Using an idea from: https://eprint.iacr.org/2011/039.pdf
	 * We compute the first double into Jacobian coordinates
	 * with only four squarings:
	 *
	 *  X' = x^4 - 2*a*x^2 + a^2 - 8*b*x
	 *  Y' = y^4 + 18*b*y^2 + 3*a*x^4 - 6*a^2*x^2 - 24*a*b*x - 27*b^2 - a^3
	 *  Z' = 2y
	 */
	gf_sqr(XX.v, Q1->x);
	gf_sqr(S.v, XX.v);
	gf_sqr(YY.v, Q1->y);
	gf_sqr(YYYY.v, YY.v);
	for (i = 0; i < 19; i ++) {
		uint32_t m;

		m = R * (uint32_t)S.v[i] + SIXm * (uint32_t)XX.v[i];
		if (i < Bi) {
			m += MSIXTEENBm * (uint32_t)Q1->x[i + 19 - Bi];
		} else {
			m += MEIGHTBm * (uint32_t)Q1->x[i - Bi];
		}
		if (i == 0) {
			m += NINEmm;
		}
		X.v[i] = (uint16_t)mp_frommonty(m);

		m = R * (uint32_t)YYYY.v[i]
			+ MNINEm * (uint32_t)S.v[i]
			+ MFIFTYFOURm * (uint32_t)XX.v[i];
		if (i < Bi) {
			m += THIRTYSIXBm * (uint32_t)YY.v[i + 19 - Bi]
				+ HUNDREDFORTYFOURBm * Q1->x[i + 19 - Bi];
		} else {
			m += EIGHTEENBm * (uint32_t)YY.v[i - Bi]
				+ SEVENTYTWOBm * Q1->x[i - Bi];
		}
		if (i == 0) {
			m += TWENTYSEVENmm;
		}
		if (i == (2 * Bi)) {
			m += MTWENTYSEVENBBmm;
		}
		Y.v[i] = (uint16_t)mp_frommonty(m);

		Z.v[i] = (uint16_t)mp_add(Q1->y[i], Q1->y[i]);
	}

	/*
	 * We perform all remaining doublings in Jacobian coordinates,
	 * using formulas from:
	 *   https://hyperelliptic.org/EFD/g1p/auto-shortw-jacobian.html
	 * This uses 1M+8S for a doubling. Since a = -3, we could use
	 * the 3M+5S formulas instead, but the generic 1M+8S are a tad
	 * faster if 3S < 2M, which is true on the ARM Cortex M0+.
	 *
	 * Moreover, for the first iteration, we can benefit from the
	 * squarings already computed above, since Z1 = 2*y; this saves
	 * two squarings in that iteration.
	 */
	for (cc = 1; cc < k; cc ++) {
		/* ZZ = Z1^2
		   ZZZZ = ZZ^2 (in S) */
		if (cc == 1) {
			int j;

			for (j = 0; j < 19; j ++) {
				ZZ.v[j] = (uint16_t)mp_montymul(
					YY.v[j], FOURm);
				S.v[j] = (uint16_t)mp_montymul(
					YYYY.v[j], SIXTEENm);
			}
		} else {
			gf_sqr(ZZ.v, Z.v);
			gf_sqr(S.v, ZZ.v);
		}

		/* XX = X1^2 */
		gf_sqr(XX.v, X.v);

		/* M = 3*XX+a*ZZZZ
		   (this releases S) */
		gf_sub(M.v, XX.v, S.v);
		for (i = 0; i < 19; i ++) {
			M.v[i] = (uint16_t)mp_montymul(M.v[i], THREEm);
		}

		/* YY = Y1^2 */
		gf_sqr(YY.v, Y.v);

		/* YYYY = YY^2 */
		gf_sqr(YYYY.v, YY.v);

		/* S = 2*((X1+YY)^2-XX-YYYY) */
		gf_add(S.v, X.v, YY.v);
		gf_sqr(S.v, S.v);
		gf_sub(S.v, S.v, XX.v);
		gf_sub(S.v, S.v, YYYY.v);
		gf_add(S.v, S.v, S.v);

		/* store Y1+Z1 into XX */
		gf_add(XX.v, Y.v, Z.v);

		/* X2 = M^2-2*S */
		gf_sqr(X.v, M.v);
		gf_sub(X.v, X.v, S.v);
		gf_sub(X.v, X.v, S.v);

		/* Y2 = M*(S-X2)-8*YYYY */
		gf_sub(S.v, S.v, X.v);
		gf_mul(S.v, S.v, M.v);
		for (i = 0; i < 19; i ++) {
			Y.v[i] = (uint16_t)mp_sub(S.v[i],
				mp_montymul(YYYY.v[i], EIGHTm));
		}

		/* Z2 = (Y1+Z1)^2-YY-ZZ */
		gf_sqr(XX.v, XX.v);
		gf_sub(XX.v, XX.v, YY.v);
		gf_sub(Z.v, XX.v, ZZ.v);
	}

	/*
	 * Convert back to affine coordinates.
	 * The Q3->neutral flag has already been set.
	 */
	gf_inv(Z.v, Z.v);
	gf_sqr(ZZ.v, Z.v);
	gf_mul(Q3->x, X.v, ZZ.v);
	gf_mul(ZZ.v, ZZ.v, Z.v);
	gf_mul(Q3->y, Y.v, ZZ.v);
}

/* see inner.h */
void
curve9767_inner_window_put(window_point8 *window,
	const curve9767_point *Q, uint32_t k)
{
	memcpy(&window->v[(k << 1) + 0], Q->x, sizeof Q->x);
	memcpy(&window->v[(k << 1) + 1], Q->y, sizeof Q->y);
	window->v[(k << 1) + 0].v[19] = 0;
	window->v[(k << 1) + 1].v[19] = 0;
}

/* see inner.h */
void
curve9767_inner_window_lookup(curve9767_point *Q,
	const window_point8 *window, uint32_t k)
{
	/*
	 * To speed things up a bit, we do the lookup with 32-bit words,
	 * i.e. two base field elements at a time.
	 */

	uint32_t u, m;
	size_t v;
	field_element x, y;

	/*
	 * Process first window element.
	 */
	m = (uint32_t)(-k >> 31) - 1;
	for (v = 0; v < 10; v ++) {
		x.w[v] = m & window->v[0].w[v];
		y.w[v] = m & window->v[1].w[v];
	}

	/*
	 * Process subsequent elements.
	 */
	for (u = 1; u < 8; u ++) {
		m = k - u;
		m = ((uint32_t)(m | -m) >> 31) - 1;
		for (v = 0; v < 10; v ++) {
			x.w[v] |= m & window->v[(u << 1) + 0].w[v];
			y.w[v] |= m & window->v[(u << 1) + 1].w[v];
		}
	}

	/*
	 * Return the point.
	 */
	memcpy(Q->x, x.v, sizeof Q->x);
	memcpy(Q->y, y.v, sizeof Q->y);
}

/* see inner.h */
const window_point8 curve9767_inner_window_G = { {
	/* 1 */
	{ { 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,
	    9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767 } },
	{ { 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,
	    9767, 9767, 9767, 9767, 5183, 9767, 9767, 9767, 9767 } },
	/* 2 */
	{ { 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,
	    5449, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767 } },
	{ { 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,
	    9767, 9767, 9767, 9767, 4584, 6461, 9767, 9767, 9767 } },
	/* 3 */
	{ { 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,  827,  976,
	    9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767 } },
	{ { 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,
	    9767, 9767, 3199, 3986, 5782, 9767, 9767, 9767, 9767 } },
	/* 4 */
	{ { 6847, 1976, 9464, 6412, 8169, 5071, 8735, 3293, 7935, 3726,
	    4904, 5698, 9489, 9400, 4154, 8678, 6975, 9238,  441 } },
	{ { 3372, 2850, 5801, 4484, 1491, 2780, 4926,  131, 1749, 7077,
	    1657, 4748,  224, 2314, 1505, 6490, 4870, 1223, 9118 } },
	/* 5 */
	{ { 2210, 6805, 8708, 6537, 3884, 7962, 4288, 4962, 1643, 1027,
	     137, 7547, 2061,  633, 7731, 4163, 5253, 3525, 7420 } },
	{ { 4545, 6304, 4229, 2572, 2696, 9639,  630,  626, 6761, 3512,
	    9591, 6690, 4265, 1077, 2897, 7052, 9297, 7036, 4309 } },
	/* 6 */
	{ { 5915, 7363, 8663, 1274, 8378,  914, 6128, 5336, 1659, 5799,
	    8881,  467, 1031, 4884, 5335,  241, 1478, 5948, 3000 } },
	{ { 4791, 6157, 8549, 7733, 7129, 4022,  157, 8626, 6629, 5674,
	    2437,  813, 3090, 1526, 4136, 9027, 6621, 6223, 2711 } },
	/* 7 */
	{ { 6201, 1567, 2635, 4915, 7852, 5478,   89, 4059, 8126, 5599,
	    4473, 5182, 7517, 1411, 1170, 3882, 7734, 7033, 6451 } },
	{ { 8131, 3939, 3355, 1142,  657, 7366, 9633, 3902, 3550, 2644,
	    9114, 7251, 7760, 3809, 9435, 1813, 3885, 3492, 3881 } },
	/* 8 */
	{ {  719, 4263, 8812, 9287, 1052, 5035, 6303, 4911, 1204, 5345,
	    1754, 1649, 9675, 6594, 5591, 5535, 4659, 7604, 8865 } },
	{ { 4732, 4902, 5613, 6248, 7507, 4751, 3381, 4626, 2043, 5131,
	    4247, 3350,  187, 9349, 3258, 2566, 1093, 2328, 4392 } }
} };

/* see inner.h */
const window_point8 curve9767_inner_window_G64 = { {
	/* 1 */
	{ { 5402, 8618,  942, 5768,   13, 3891, 7511, 9294, 5476, 8402,
	    4590, 8208, 9604, 5849, 2627, 6310, 4606, 3885, 4662 } },
	{ { 9306, 4077, 3962,   83, 6956, 9275, 3244, 2536, 6714, 9679,
	    3475, 9108, 8609, 4419, 4679, 4765, 3288, 2242, 7206 } },
	/* 2 */
	{ { 9339, 7619, 1746, 5439, 2068, 8249, 8667,  772, 8629, 2580,
	    3535, 1497, 7659,  942,  418, 4236, 4544, 6106, 3463 } },
	{ { 6331, 2046, 4103, 1626, 1236, 7043, 1142,  248, 4893, 7448,
	     904,  833, 1839, 4591, 5095, 4412, 4501, 4022, 1789 } },
	/* 3 */
	{ {    8, 8849,  369, 6009, 2146, 8061, 6681, 3744, 6338, 1598,
	    6526, 2620,  502, 8827,  972, 1258,  163, 9506, 1760 } },
	{ { 4708, 9599, 8298, 4019, 3529, 3452, 5713, 2038, 2826, 8551,
	    9589, 4902,  787, 9144, 9523,  522, 2662, 6523, 9544 } },
	/* 4 */
	{ { 6468, 6074, 2548, 5799, 2184, 9236, 9087, 8194, 2125, 4482,
	    1596, 4633, 1219, 1728, 8587, 4914, 6813, 7586, 9632 } },
	{ { 7030, 6132, 8317, 4360, 4703, 8700, 3474, 3142, 9058, 6083,
	    8665,  920, 5688,  710, 8794, 9433, 8022, 2356, 5285 } },
	/* 5 */
	{ { 4181, 5728, 9216, 4434, 6970, 1881, 2464,  514, 1761, 3203,
	    2625, 8814, 6534, 4902, 1128,  441, 4058, 8648, 4520 } },
	{ { 3588, 7938, 3839, 2596,  428, 1983, 2670, 2920, 9333, 9475,
	    9519, 5638, 7220, 5772, 1006, 1599, 3584, 2143, 4050 } },
	/* 6 */
	{ { 7762, 9352, 6512, 1541, 5866,  565, 1801, 3246, 1697, 5080,
	    3383, 4351,  374, 6823, 4763, 4474, 2885, 9241, 1300 } },
	{ { 6689, 7021, 4440, 3976, 3443, 7873, 7187, 3414, 1165, 8823,
	     777, 9405,   54, 5902, 6112, 4515, 7303, 8691, 2848 } },
	/* 7 */
	{ { 8648,  915, 3299, 3952, 9488, 3862, 6457, 5039, 8374, 4093,
	    1368, 7980, 9482, 8781, 7363, 4633, 4255, 4196, 2136 } },
	{ { 3469, 8416, 9696, 1326, 9065, 5954, 4500, 4959, 4339, 5552,
	    1958, 1917, 4947, 6000, 1038, 5963, 4016, 4130, 8603 } },
	/* 8 */
	{ { 6413, 3230, 7046, 7939, 7788, 2866, 4807, 2771,  431, 3670,
	    3499, 5171, 8340, 8553, 4912, 8246, 3368, 4711, 1096 } },
	{ { 4169, 7327,  448,  462, 5552, 5998, 2096, 7221, 8644, 2493,
	    4292, 4406, 6619, 7277, 2268, 8841, 4384, 7040, 7167 } }
} };

/* see inner.h */
const window_point8 curve9767_inner_window_G128 = { {
	/* 1 */
	{ {  380,  263, 4759, 4097,  181,  189, 5006, 4610, 9254, 6379,
	    6272, 5845, 9415, 3047, 1596, 8881, 7183, 5423, 2235 } },
	{ { 6163, 9431, 4357, 9676, 4711, 5555, 3662, 5607, 2967, 7973,
	    4860, 4592, 6575, 7155, 1170, 4774, 1910, 5227, 2442 } },
	/* 2 */
	{ { 1481, 2891, 4276,  503, 5380, 6821, 8485, 7577, 5705, 4661,
	    4931, 9465, 8613, 4976, 2486, 9056, 5680, 7836, 3053 } },
	{ { 1472, 6012, 4907,   18, 8418, 7702, 3518, 4736, 7491, 7602,
	    8759, 8319, 9182, 8357,  371, 9300, 2720, 4510, 9284 } },
	/* 3 */
	{ { 6654, 5694, 4667, 1476, 4683, 5380, 6665, 3646, 4183, 6378,
	    1804, 3321, 6321, 2515, 3203,  463, 9604, 7417, 4611 } },
	{ { 3296, 9223, 7168, 8235, 3896, 2560, 2981, 7937, 4510, 5427,
	     108, 2987, 6512, 2105, 5825, 2720, 2364, 1742, 7087 } },
	/* 4 */
	{ { 2165, 3108, 9435, 3694, 3344, 9054, 8767, 1948, 6635, 5896,
	    8631, 7602, 4752, 3842, 2097,  612, 5617,   82,  684 } },
	{ { 5040, 3982, 8914, 7635, 8796, 4838, 7872,  154, 8305, 9099,
	    5033, 2716, 1936, 8810, 1320, 3126, 9375, 3971, 4511 } },
	/* 5 */
	{ { 3733, 2716, 7734,  246,  636, 4902, 6509, 5967, 3480,  387,
	      31, 7474, 6791, 8214,  733, 9550,   13,  941,  897 } },
	{ { 7173, 4712, 7342, 8073, 5986, 3164, 7062, 8929, 5495, 1703,
	      98, 4721, 4988, 5517,  609, 8663, 5716, 4256, 2286 } },
	/* 6 */
	{ { 5791, 5324,  645, 1571, 4444, 5810, 5743, 3636,  215, 2633,
	    3751, 5942, 3932, 2404,  425,  458, 8843, 4890, 6863 } },
	{ { 8035, 9021, 9036, 1406, 2100, 4669, 4490, 6525, 3285, 6325,
	    2411, 1447, 3340, 6225, 8860, 8711, 5589, 1637, 3160 } },
	/* 7 */
	{ { 1209, 1752, 8277, 2095, 2861, 3406, 9001, 7385, 1214, 8626,
	    1568, 8438, 9444, 2164,  109, 5503,  880, 5453, 5670 } },
	{ {  145, 1939, 1647, 4249,  400, 8246, 8978, 6814, 6635, 8142,
	    3097, 3837, 4908, 8642,  423, 2757, 6341, 2466, 2473 } },
	/* 8 */
	{ { 4092, 7211, 1846, 2988, 2103, 3521, 3682,  242, 3157, 3344,
	    414, 1548, 7637,  706, 4324, 4079, 7797,  964, 5944 } },
	{ { 1978, 5559, 2543, 4324, 7281, 3230, 1148, 1748, 7880, 2613,
	    6362, 1623,  415, 1560, 4468, 2073, 9072, 3522,  875 } }
} };

/* see inner.h */
const window_point8 curve9767_inner_window_G192 = { {
	/* 1 */
	{ { 8407, 7445, 4044, 1822, 1726, 8235, 7931, 5851, 7572,  422,
	    3761, 2505, 6817, 8254, 1029,   24, 6853, 5715, 7561 } },
	{ { 8642, 2669,  807, 5680, 1002, 7294,  203,  345, 1511, 4053,
	    1451, 5450, 7893, 4334, 7782, 4018,  993, 9492, 2107 } },
	/* 2 */
	{ { 5774, 4518, 5439, 1520, 8860, 9461, 9413, 8032, 4623, 2969,
	    7141, 1814,  989, 4110, 4529, 6079, 5454, 5030,  555 } },
	{ { 5723,  525, 5045, 2786, 8035, 2051, 8718, 5779, 4652, 4681,
	    9375, 4561, 6395, 1365, 2424, 1708, 3436, 8471, 4510 } },
	/* 3 */
	{ {  300, 4238, 2725, 1272, 1817, 2821,   53,  232, 7632, 2935,
	    8755, 1226, 8444, 8831, 4040, 5202, 2477, 5159, 7411 } },
	{ { 6531, 8372, 3737, 9119, 2346, 5852, 5255, 2073, 2255,   77,
	    8701, 2739, 8628, 6985, 2160,  636, 3144, 1799,  404 } },
	/* 4 */
	{ { 4083, 7997, 2474, 7443, 5696, 3693, 4830, 6235, 7260, 8806,
	    1022, 3457,  251, 3423, 2568, 4085, 6577, 2822, 8893 } },
	{ {  802,  573,  852, 8119, 7218, 5003,  125, 1014, 1368, 7795,
	    2083, 7072, 2859, 4859, 9613, 5790, 4971, 6790, 4277 } },
	/* 5 */
	{ { 6509, 6781, 1168, 6858, 6924,  186, 1702, 2639,  982, 6319,
	     403, 7774, 8445, 8453, 5447, 4645, 4353, 9556, 7406 } },
	{ { 7378, 1742, 4404, 6291, 3385, 9167,  500, 6422, 6599, 6085,
	     208, 9435, 6935, 3581, 5175, 2223, 1199, 2171, 6773 } },
	/* 6 */
	{ { 3332, 1102, 3273, 1477, 4996, 6463, 7023, 6381, 1918, 1547,
	    6361, 6378, 7999, 5042, 8473, 1225, 5007, 4280, 9752 } },
	{ {  779, 1034, 2193, 7860, 9302, 3617, 6501, 2844, 5169, 1918,
	    4983,  119, 6018, 1267, 2758,  788, 9249, 1398, 5795 } },
	/* 7 */
	{ { 7214, 9610, 8764, 4059, 2822, 2708, 5630, 5382, 5441, 8807,
	    5464, 8259,  788, 7864, 2095, 4028, 3191, 5754, 8166 } },
	{ { 4697, 8893, 5086, 6596,  784, 5862, 7830, 6345, 6968, 8165,
	     316, 8644, 9554,  315, 9337,  736, 3371, 4476, 7232 } },
	/* 8 */
	{ { 2080, 1606, 7613, 1753,  773, 2343, 4365, 9079, 3522, 5307,
	    7475, 9273, 5018, 9428, 7506, 9204, 6734, 1373, 2019 } },
	{ { 7141, 2475, 1648, 7094, 7167, 7295, 5689, 5854,  658, 1878,
	    2724,  132, 5521, 9275, 7880, 7598, 5428, 6665, 5452 } }
} };

/* see inner.h */
void
curve9767_inner_Icart_map(curve9767_point *Q, const uint16_t *u)
{
	field_element t1, t2, t3, t4;
	int i;

	/* u^2 -> t1 */
	gf_sqr(t1.v, u);

	/* u^4 -> t2 */
	gf_sqr(t2.v, t1.v);

	/* u^6 -> t3 */
	gf_mul(t3.v, t1.v, t2.v);

	/* (3*a - u^4)/(6*u) -> t2   (value 'v' from the map) */
	gf_neg(t2.v, t2.v);
	t2.v[0] = (uint16_t)mp_add(t2.v[0], MNINEm);
	for (i = 0; i < 19; i ++) {
		t4.v[i] = (uint16_t)mp_montymul(u[i], SIXm);
	}
	gf_inv(t4.v, t4.v);
	gf_mul(t2.v, t2.v, t4.v);

	/* v^2 - b - (u^6)/27 -> t3 */
	for (i = 0; i < 19; i ++) {
		t3.v[i] = (uint16_t)mp_montymul(t3.v[i], IMTWENTYSEVENm);
	}
	gf_sqr(t4.v, t2.v);
	gf_add(t3.v, t3.v, t4.v);
	t3.v[Bi] = (uint16_t)mp_sub(t3.v[Bi], Bm);

	/* (v^2 - b - (u^6)/27)^(1/3) + (u^2)/3 -> x */
	gf_cubert(t3.v, t3.v);
	for (i = 0; i < 19; i ++) {
		t1.v[i] = (uint16_t)mp_montymul(t1.v[i], ITHREEm);
	}
	gf_add(Q->x, t3.v, t1.v);

	/* u*x + v -> y */
	gf_mul(t1.v, Q->x, u);
	gf_add(Q->y, t1.v, t2.v);

	/* Set Q to the point-at-infinity if and only if u is zero. */
	Q->neutral = gf_eq(u, curve9767_inner_gf_zero.v);
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

/*
 * Input: c is a 128-bit signed integer, in signed little-endian encoding
 * (16 bytes).
 * This function replaces c with |c|. Returned value is 1 if c was negative,
 * 0 if it was 0 or positive.
 * NOT CONSTANT-TIME
 */
static int
abs_i128(uint8_t *c)
{
	int i;
	unsigned cc;

	if (c[15] < 0x80) {
		return 0;
	}
	cc = 1;
	for (i = 0; i < 16; i ++) {
		unsigned w;

		w = c[i];
		w = (w ^ 0xFF) + cc;
		c[i] = (uint8_t)w;
		cc = w >> 8;
	}
	return 1;
}

/*
 * Prepare NAF_w recoding. Given c[] of length 'len' bytes (unsigned
 * little-endian encoding, top bit of last byte must be zero), this
 * function computes the position of the non-zero NAF_w coefficients
 * and writes them as ones into the rcbf[] bit field (8*len bits).
 */
static void
prepare_recode_NAF(uint8_t *rcbf, const uint8_t *c, size_t len, int w)
{
	unsigned acc, mask1, mask2;
	int acc_len;
	size_t u, v;

	acc = 0;
	acc_len = 0;
	u = 0;
	memset(rcbf, 0, len);
	mask1 = 1u << (w - 1);
	mask2 = ~((1u << w) - 1u);
	for (v = 0; v < (len << 3); v ++) {
		if (acc_len < w && u < len) {
			acc += (unsigned)c[u ++] << acc_len;
			acc_len += 8;
		}
		if ((acc & 1) != 0) {
			acc += acc & mask1;
			acc &= mask2;
			rcbf[v >> 3] |= 1u << (v & 7);
		}
		acc >>= 1;
		acc_len --;
	}
}

static const field_element window_odd5_G[] = {
	/* 1 */
	{ { 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,
	    9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,    0 } },
	{ { 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,
	    9767, 9767, 9767, 9767, 5183, 9767, 9767, 9767, 9767,    0 } },
	/* 3 */
	{ { 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,  827,  976,
	    9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,    0 } },
	{ { 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,
	    9767, 9767, 3199, 3986, 5782, 9767, 9767, 9767, 9767,    0 } },
	/* 5 */
	{ { 2210, 6805, 8708, 6537, 3884, 7962, 4288, 4962, 1643, 1027,
	     137, 7547, 2061,  633, 7731, 4163, 5253, 3525, 7420,    0 } },
	{ { 4545, 6304, 4229, 2572, 2696, 9639,  630,  626, 6761, 3512,
	    9591, 6690, 4265, 1077, 2897, 7052, 9297, 7036, 4309,    0 } },
	/* 7 */
	{ { 6201, 1567, 2635, 4915, 7852, 5478,   89, 4059, 8126, 5599,
	    4473, 5182, 7517, 1411, 1170, 3882, 7734, 7033, 6451,    0 } },
	{ { 8131, 3939, 3355, 1142,  657, 7366, 9633, 3902, 3550, 2644,
	    9114, 7251, 7760, 3809, 9435, 1813, 3885, 3492, 3881,    0 } },
	/* 9 */
	{ {  363, 8932, 3221, 8711, 6270, 2703, 5538, 7030, 7675, 4644,
	     635,  606, 6910, 6333, 3475, 2179, 1877, 3507, 8687,    0 } },
	{ { 9675, 9445, 1940, 4624, 8972, 5163, 2711, 9537, 4839, 9654,
	    9763, 2611, 7206, 1457, 4841,  640, 2748,  696, 1806,    0 } },
	/* 11 */
	{ { 7650, 9241,  962, 2228, 1594, 3577, 6783, 9424, 1599, 2635,
	    8045, 1344, 4828, 5684, 4114, 1156, 7682, 5903, 9381,    0 } },
	{ { 9077,   79, 3130, 1773, 7395, 5472, 9573, 3901, 3315, 6687,
	    1029,  225, 8685, 9176, 1656, 8364, 9267, 7339, 8610,    0 } },
	/* 13 */
	{ { 4629,  168, 5989, 6341, 7443, 1266, 1254, 4985, 6529, 4344,
	    6293, 3899, 5915, 6215, 8149, 6016, 5667, 9333, 1047,    0 } },
	{ { 1029, 1598, 6939, 3680, 2190, 4891, 7700, 1863, 7734, 2594,
	    7503, 6411, 1286, 3129, 8966,  980, 9457, 6898, 6219,    0 } },
	/* 15 */
	{ { 9512, 9233, 4182, 1978, 7278, 5606, 9663, 8472,  639, 3390,
	    5480, 9279, 2692, 3295, 7832, 6774, 9345, 1616, 1767,    0 } },
	{ { 4559, 1683, 7874, 2533, 1353, 1371, 6394, 7339, 7591, 3800,
	    1677,   78, 9681, 1379, 4305, 7061,  529, 9533, 9374,    0 } },
};

static const field_element window_odd5_G128[] = {
	/* 1 */
	{ {  380,  263, 4759, 4097,  181,  189, 5006, 4610, 9254, 6379,
	    6272, 5845, 9415, 3047, 1596, 8881, 7183, 5423, 2235,    0 } },
	{ { 6163, 9431, 4357, 9676, 4711, 5555, 3662, 5607, 2967, 7973,
	    4860, 4592, 6575, 7155, 1170, 4774, 1910, 5227, 2442,    0 } },
	/* 3 */
	{ { 6654, 5694, 4667, 1476, 4683, 5380, 6665, 3646, 4183, 6378,
	    1804, 3321, 6321, 2515, 3203,  463, 9604, 7417, 4611,    0 } },
	{ { 3296, 9223, 7168, 8235, 3896, 2560, 2981, 7937, 4510, 5427,
	     108, 2987, 6512, 2105, 5825, 2720, 2364, 1742, 7087,    0 } },
	/* 5 */
	{ { 3733, 2716, 7734,  246,  636, 4902, 6509, 5967, 3480,  387,
	      31, 7474, 6791, 8214,  733, 9550,   13,  941,  897,    0 } },
	{ { 7173, 4712, 7342, 8073, 5986, 3164, 7062, 8929, 5495, 1703,
	      98, 4721, 4988, 5517,  609, 8663, 5716, 4256, 2286,    0 } },
	/* 7 */
	{ { 1209, 1752, 8277, 2095, 2861, 3406, 9001, 7385, 1214, 8626,
	    1568, 8438, 9444, 2164,  109, 5503,  880, 5453, 5670,    0 } },
	{ {  145, 1939, 1647, 4249,  400, 8246, 8978, 6814, 6635, 8142,
	    3097, 3837, 4908, 8642,  423, 2757, 6341, 2466, 2473,    0 } },
	/* 9 */
	{ { 6631, 7588, 1952, 4374, 8217, 8672, 5188, 1936, 7566,  375,
	    6815, 7315, 3722, 4584, 8873, 6057,  489, 5733, 1093,    0 } },
	{ { 1229, 7837,  739, 5943, 3608, 5875, 6885,  726, 4885, 3608,
	    1216, 4182,  357, 2637, 7653, 1176, 4836, 9068, 5765,    0 } },
	/* 11 */
	{ { 4654, 3775, 6645, 6370, 5153, 5726, 8294, 5693, 1114, 5363,
	    8356, 1933, 2539, 2708, 9116, 8695,  169, 3959, 7314,    0 } },
	{ { 9451, 7628, 8982, 5735, 4808, 8199, 4164, 1030, 8346, 8643,
	    5476, 9020, 2621, 5566, 7917, 6041, 3438, 8972, 2822,    0 } },
	/* 13 */
	{ {  943,  239, 2994, 7226, 4656, 2110, 5835, 1272, 5042, 2600,
	     990, 5338, 3774, 7370,  234, 4208, 7439, 3914, 2208,    0 } },
	{ { 9466, 5076, 2796, 9013, 8794, 7555, 5417, 7292, 9051, 9048,
	    1895, 6041,  802, 6809, 7064, 5828, 7251, 3444, 6933,    0 } },
	/* 15 */
	{ { 1304, 2731, 6661, 9618, 7689,  121,  991, 1683, 5627, 3143,
	    2891, 4724, 5853, 3174, 8571, 7021, 2925, 5461,  409,    0 } },
	{ { 8072, 5485, 6915, 5742, 5583, 1904, 8913,  678, 9327, 6739,
	    7675, 1134, 7284, 8485, 7235, 1210, 2261, 6781,  360,    0 } },
};

/* see inner.h */
void
curve9767_inner_mul2_mulgen_add_vartime(curve9767_point *Q3,
	const curve9767_point *Q0, const uint8_t *c0, int neg0,
	const curve9767_point *Q1, const uint8_t *c1, int neg1,
	const uint8_t *c2)
{
	uint8_t rcbf0[16], rcbf1[16], rcbf2[32];
	curve9767_point T, W0[4], W1[4];
	int i, dbl;
	unsigned acc0, acc1, acc2, acc3;

	/*
	 * Prepare NAF_w recoding of multipliers.
	 */
	prepare_recode_NAF(rcbf0, c0, 16, 4);
	prepare_recode_NAF(rcbf1, c1, 16, 4);
	prepare_recode_NAF(rcbf2, c2, 32, 5);

	/*
	 * Make window for Q0. If c0 is negative (neg0 == 1), we set:
	 *   W0[k] = -(2*k+1)*Q0
	 * Otherwise, we set:
	 *   W0[k] = (2*k+1)*Q0
	 */
	W0[0] = *Q0;
	if (neg0) {
		curve9767_point_neg(&W0[0], &W0[0]);
	}
	curve9767_point_add(&T, &W0[0], &W0[0]);
	for (i = 1; i < 4; i ++) {
		curve9767_point_add(&W0[i], &W0[i - 1], &T);
	}

	/*
	 * Similar window construction for Q1.
	 */
	W1[0] = *Q1;
	if (neg1) {
		curve9767_point_neg(&W1[0], &W1[0]);
	}
	curve9767_point_add(&T, &W1[0], &W1[0]);
	for (i = 1; i < 4; i ++) {
		curve9767_point_add(&W1[i], &W1[i - 1], &T);
	}

	/*
	 * Do the window based point multiplication. Q0 and Q1 use the
	 * computed windows W0 and W1; c2*G uses the precomputed windows
	 * for G and (2^128)*G.
	 * NAF_w recoding is computed on the fly, using the bits prepared
	 * in the rcbf*[] arrays.
	 */
	curve9767_point_set_neutral(Q3);
	dbl = 0;
	acc0 = c0[15];
	acc1 = c1[15];
	acc2 = c2[15] | ((unsigned)c2[16] << 8);
	acc3 = c2[31];
	for (i = 127; i >= 0; i --) {
		int m0, m1, m2, m3, s;

		dbl ++;
		s = (i & 7);
		if (((rcbf0[i >> 3] >> s) & 1) != 0) {
			m0 = (1u | (acc0 >> s)) & 0x0F;
		} else {
			m0 = 0;
		}
		if (((rcbf1[i >> 3] >> s) & 1) != 0) {
			m1 = (1u | (acc1 >> s)) & 0x0F;
		} else {
			m1 = 0;
		}
		if (((rcbf2[i >> 3] >> s) & 1) != 0) {
			m2 = (1u | (acc2 >> s)) & 0x1F;
		} else {
			m2 = 0;
		}
		if (((rcbf2[(i >> 3) + 16] >> s) & 1) != 0) {
			m3 = (1u | (acc3 >> s)) & 0x1F;
		} else {
			m3 = 0;
		}
		if (s == 0 && i != 0) {
			acc0 = (acc0 << 8) | c0[(i - 1) >> 3];
			acc1 = (acc1 << 8) | c1[(i - 1) >> 3];
			acc2 = (acc2 << 8) | c2[(i - 1) >> 3];
			acc3 = (acc3 << 8) | c2[((i - 1) >> 3) + 16];
		}

		if ((m0 | m1 | m2 | m3) == 0) {
			continue;
		}
		if (!Q3->neutral) {
			curve9767_point_mul2k(Q3, Q3, dbl);
		}
		dbl = 0;

		if (m0 != 0) {
			if (m0 < 0x08) {
				curve9767_point_add(Q3, Q3, &W0[m0 >> 1]);
			} else {
				curve9767_point_neg(&T, &W0[(16 - m0) >> 1]);
				curve9767_point_add(Q3, Q3, &T);
			}
		}

		if (m1 != 0) {
			if (m1 < 0x08) {
				curve9767_point_add(Q3, Q3, &W1[m1 >> 1]);
			} else {
				curve9767_point_neg(&T, &W1[(16 - m1) >> 1]);
				curve9767_point_add(Q3, Q3, &T);
			}
		}

		if (m2 != 0) {
			if (m2 < 0x10) {
				memcpy(T.x, window_odd5_G + (m2 - 1),
					sizeof T.x);
				memcpy(T.y, window_odd5_G + m2,
					sizeof T.y);
			} else {
				memcpy(T.x, window_odd5_G + (31 - m2),
					sizeof T.x);
				curve9767_inner_gf_neg(T.y,
					(window_odd5_G + (32 - m2))->v);
			}
			T.neutral = 0;
			curve9767_point_add(Q3, Q3, &T);
		}

		if (m3 != 0) {
			if (m3 < 0x10) {
				memcpy(T.x, window_odd5_G128 + (m3 - 1),
					sizeof T.x);
				memcpy(T.y, window_odd5_G128 + m3,
					sizeof T.y);
			} else {
				memcpy(T.x, window_odd5_G128 + (31 - m3),
					sizeof T.x);
				curve9767_inner_gf_neg(T.y,
					(window_odd5_G128 + (32 - m3))->v);
			}
			T.neutral = 0;
			curve9767_point_add(Q3, Q3, &T);
		}
	}

	if (dbl > 0 && Q3->neutral == 0) {
		curve9767_point_mul2k(Q3, Q3, dbl);
	}
}

/* see curve9767.h */
int
curve9767_point_verify_mul_mulgen_add_vartime(
	const curve9767_point *Q1, const curve9767_scalar *s1,
	const curve9767_scalar *s2, const curve9767_point *Q2)
{
	/*
	 * We want to verify:
	 *   c1*(s1*Q1 + s2*G - Q2) = 0
	 * For a well-chosen non-zero value c0. This equation is
	 * equivalent to:
	 *   (c1*s1)*Q1 + (c1*s2)*G - c1*Q2 = 0
	 * c1*s2 mod n is a 252-bit integer, but we can split it into
	 * two halves of 128 bits (low) and 124 bits (high) since we
	 * have a precomputed window for (2^128)*G.
	 *
	 * Thus, we look for a "small" c1 such that (c1*s1 mod n) is
	 * also "small". Reduction of a lattice basis in dimension two,
	 * as implemented by curve9767_inner_reduce_basis_vartime(),
	 * does exactly that, and returns c1 and c0 = c1*s1 as two
	 * signed integers whose absolute value is less than 2^127.
	 */
	uint8_t c0[16], c1[16], c2[32];
	curve9767_point T;
	curve9767_scalar ss;
	int neg0, neg1;

	/*
	 * Perform lattice basis reduction.
	 */
	curve9767_inner_reduce_basis_vartime(c0, c1, s1);

	/*
	 * Check the signs of c0 and c1, and replace them with their
	 * absolute value.
	 */
	neg0 = abs_i128(c0);
	neg1 = abs_i128(c1);

	/*
	 * Compute c1*s2 mod n and encode it.
	 */
	curve9767_scalar_decode_strict(&ss, c1, 16);
	if (neg1) {
		curve9767_scalar_neg(&ss, &ss);
	}
	curve9767_scalar_mul(&ss, &ss, s2);
	curve9767_scalar_encode(c2, &ss);

	/*
	 * Compute c0*Q1 + c2*G - c1*Q2 into T.
	 */
	curve9767_inner_mul2_mulgen_add_vartime(
		&T, Q1, c0, neg0, Q2, c1, 1 - neg1, c2);

	/*
	 * The equation is verified if and only if the result if the
	 * point at infinity.
	 */
	return T.neutral;
}
