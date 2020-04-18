/*
 * Reference implementation, portable C.
 *
 * All operations are implemented in plain C code. Multiplication operations
 * are over 32 bits (with only 32-bit results).
 */

#include "inner.h"

#include <immintrin.h>

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
	/*
	return 1 + ((((uint32_t)(x * (uint32_t)P1I) >> 16)
		* (uint32_t)P) >> 16);
	*/

	uint32_t y;
	
	y = x * (uint32_t)P1I;
	return (uint32_t)((((uint64_t)y * (uint64_t)P) + x) >> 32);
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

	/* xi = x^(((9*16+8)*16+9)*4+1) = x^9765 = 1/x */
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
 * Conventions:
 *   u0 contains a0..a15  (16 bits per element)
 *   u1 contains a16..a18  (16 bits per element)
 * Unused slots in u1 should be ignored; they are not necessarily zero.
 * The 'dummy' slot is there only for alignment purposes (it would be
 * there implicitly anyway).
 */
typedef struct {
	__m256i u0;
	__m128i u1;
	__m128i dummy;
} vgf;

static inline void
vgf_decode(vgf *d, const uint16_t *s)
{
	d->u0 = _mm256_loadu_si256((const __m256i *)s);
	d->u1 = _mm_bsrli_si128(_mm_loadu_si128((const __m128i *)(s + 11)), 10);
}

static inline void
vgf_encode(uint16_t *d, const vgf *s)
{
	_mm_storeu_si128((__m128i *)(d + 11), _mm_bslli_si128(s->u1, 10));
	_mm256_storeu_si256((__m256i *)d, s->u0);
}

static inline void
vgf_add(vgf *d, const vgf *a, const vgf *b)
{
	__m256i p16, t0;
	__m128i p8, t1;

	p16 = _mm256_set1_epi16(9767);
	p8 = _mm_set1_epi16(9767);
	t0 = _mm256_add_epi16(a->u0, b->u0);
	t1 = _mm_add_epi16(a->u1, b->u1);
	d->u0 = _mm256_sub_epi16(t0,
		_mm256_and_si256(_mm256_cmpgt_epi16(t0, p16), p16));
	d->u1 = _mm_sub_epi16(t1, _mm_and_si128(_mm_cmplt_epi16(p8, t1), p8));
}

static inline void
vgf_sub(vgf *d, const vgf *a, const vgf *b)
{
	__m256i p16, t0;
	__m128i t1;

	p16 = _mm256_set1_epi16(9767);
	t0 = _mm256_sub_epi16(a->u0, b->u0);
	t1 = _mm_sub_epi16(a->u1, b->u1);
	d->u0 = _mm256_add_epi16(t0,
		_mm256_andnot_si256(_mm256_cmpgt_epi16(a->u0, b->u0), p16));
	d->u1 = _mm_add_epi16(t1,
		_mm_andnot_si128(_mm_cmplt_epi16(b->u1, a->u1),
		_mm256_castsi256_si128(p16)));
}

static inline void
vgf_neg(vgf *d, const vgf *a)
{
	__m256i p16, t0;
	__m128i p8, t1;

	p16 = _mm256_set1_epi16(9767);
	p8 = _mm_set1_epi16(9767);
	t0 = _mm256_sub_epi16(p16, a->u0);
	t1 = _mm_sub_epi16(p8, a->u1);
	d->u0 = _mm256_add_epi16(t0,
		_mm256_andnot_si256(_mm256_cmpgt_epi16(p16, a->u0), p16));
	d->u1 = _mm_add_epi16(t1,
		_mm_andnot_si128(_mm_cmplt_epi16(a->u1, p8), p8));
}

static inline void
vgf_condneg(vgf *d, const vgf *a, uint32_t ctl)
{
	vgf b;
	__m256i m16;
	__m128i m8;

	m8 = _mm_set1_epi32(-(int)ctl);
	m16 = _mm256_broadcastsi128_si256(m8);
	vgf_neg(&b, a);
	d->u0 = _mm256_xor_si256(a->u0,
		_mm256_and_si256(m16, _mm256_xor_si256(a->u0, b.u0)));
	d->u1 = _mm_xor_si128(a->u1,
		_mm_and_si128(m8, _mm_xor_si128(a->u1, b.u1)));
}

/*
 * BCAST32_x is a constant value that can be used in vpshufb
 * (_mm256_shuffle_epi8()). Value x is between 0 and 7. In each lane,
 * the 16-bit element x in the source is zero-extended to 32 bits and
 * broadcasted to all four slots in the destination lane.
 */
#define BCAST32_0   _mm256_set1_epi32(-2139094784)  /* 0x80800100 */
#define BCAST32_1   _mm256_set1_epi32(-2139094270)  /* 0x80800302 */
#define BCAST32_2   _mm256_set1_epi32(-2139093756)  /* 0x80800504 */
#define BCAST32_3   _mm256_set1_epi32(-2139093242)  /* 0x80800706 */
#define BCAST32_4   _mm256_set1_epi32(-2139092728)  /* 0x80800908 */
#define BCAST32_5   _mm256_set1_epi32(-2139092214)  /* 0x80800B0A */
#define BCAST32_6   _mm256_set1_epi32(-2139091700)  /* 0x80800D0C */
#define BCAST32_7   _mm256_set1_epi32(-2139091186)  /* 0x80800F0E */

/*
 * There are several possible implementations. The one below represents
 * values in epi32 format (32 bits per element) and uses
 * _mm256_mullo_epi32(). This is a relatively expensive opcode (10 cycles
 * latency, but reciprocal throughput of 1 opcode per cycle, on a
 * Skylake).
 *
 * An alternative implementation would use epi16 (16 bits per element)
 * and use _mm256_mullo_epi16() and _mm256_mulhi_epu16() to get the two
 * halves of each 16x16->32 product. Each of these is faster than
 * _mm256_mullo_epi32() (latency = 5, reciprocal throughput = 0.5),
 * leading to more multiplications per cycle. However, that would yield
 * the products in a "split" format (low and high halves in separate
 * registers), requiring unpacking opcodes (_mm256_unpacklo_epi16() and
 * _mm256_unpackhi_epi16()) whose cost cancel the benfits of the higher
 * multiplication bandwidth.
 */
static void
vgf_mul(vgf *d, const vgf *a, const vgf *b)
{
	__m256i c0, c1, c2, d0, d1, d2, d3, d4;
	__m256i zt3, x, y, p1i_rev, p1i_low, p, one;
	__m256i t0_8, t4_12, t8_16, t12_20, t16_24, t20_28, t24_32;
	__m256i t28, t32, t36;
	__m256i t19_27, t23_31, t35;

	/*
	 * Read a into c0..c2, 32 bits per element:
	 *   c0:  a0 a1 a2 a3 : a8 a9 a10 a11
	 *   c1:  a4 a5 a6 a7 : a12 a13 a14 a15
	 *   c2:  a16 a17 a18 0 : 0 0 0 0
	 */
	zt3 = _mm256_setr_epi16(
		-1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	x = a->u0;
	c0 = _mm256_unpacklo_epi16(x, _mm256_setzero_si256());
	c1 = _mm256_unpackhi_epi16(x, _mm256_setzero_si256());
	x = _mm256_and_si256(zt3, _mm256_zextsi128_si256(a->u1));
	c2 = _mm256_unpacklo_epi16(x, _mm256_setzero_si256());

	/*
	 * Read b into d0..d2, 16 bits per element, lane duplication:
	 *   d0:  b0..b7 : b0..b7
	 *   d1:  b8..b15 : b8..b15
	 *   d2:  b16..b18 0..0 : b16..b18 0..0
	 */
	x = b->u0;
	d0 = _mm256_permute2x128_si256(x, x, 0x00);
	d1 = _mm256_permute2x128_si256(x, x, 0x11);
	x = _mm256_and_si256(zt3, _mm256_zextsi128_si256(b->u1));
	d2 = _mm256_permute2x128_si256(x, x, 0x00);

	/*
	 * Multiply a by b0, b4, b8, b12 and b16.
	 */
	x = _mm256_shuffle_epi8(d0, BCAST32_0);
	t0_8 = _mm256_mullo_epi32(x, c0);
	t4_12 = _mm256_mullo_epi32(x, c1);
	t16_24 = _mm256_mullo_epi32(x, c2);
	x = _mm256_shuffle_epi8(d0, BCAST32_4);
	t4_12 = _mm256_add_epi32(t4_12, _mm256_mullo_epi32(x, c0));
	t8_16 = _mm256_mullo_epi32(x, c1);
	t20_28 = _mm256_mullo_epi32(x, c2);
	x = _mm256_shuffle_epi8(d1, BCAST32_0);
	t8_16 = _mm256_add_epi32(t8_16, _mm256_mullo_epi32(x, c0));
	t12_20 = _mm256_mullo_epi32(x, c1);
	t24_32 = _mm256_mullo_epi32(x, c2);
	x = _mm256_shuffle_epi8(d1, BCAST32_4);
	t12_20 = _mm256_add_epi32(t12_20, _mm256_mullo_epi32(x, c0));
	t16_24 = _mm256_add_epi32(t16_24, _mm256_mullo_epi32(x, c1));
	t28 = _mm256_mullo_epi32(x, c2);
	x = _mm256_shuffle_epi8(d2, BCAST32_0);
	t16_24 = _mm256_add_epi32(t16_24, _mm256_mullo_epi32(x, c0));
	t20_28 = _mm256_add_epi32(t20_28, _mm256_mullo_epi32(x, c1));
	t32 = _mm256_mullo_epi32(x, c2);

	/*
	 * Morph a into z*a
	 *   c0:  0 a0 a1 a2 : a7 a8 a9 a10
	 *   c1:  a3 a4 a5 a6 : a11 a12 a13 a14
	 *   c2:  a15 a16 a17 a18 : 0 0 0 0
	 */
	x = _mm256_bsrli_epi128(c0, 12);
	y = _mm256_bsrli_epi128(c1, 12);
	c0 = _mm256_bslli_epi128(c0, 4);
	c1 = _mm256_bslli_epi128(c1, 4);
	c2 = _mm256_bslli_epi128(c2, 4);
	c0 = _mm256_or_si256(c0, _mm256_permute2x128_si256(y, y, 0x28));
	c1 = _mm256_or_si256(c1, x);
	c2 = _mm256_or_si256(c2, _mm256_permute2x128_si256(y, y, 0x83));

	/*
	 * Multiply z*a by b1, b5, b9, b13 and b17.
	 */
	x = _mm256_shuffle_epi8(d0, BCAST32_1);
	t0_8 = _mm256_add_epi32(t0_8, _mm256_mullo_epi32(x, c0));
	t4_12 = _mm256_add_epi32(t4_12, _mm256_mullo_epi32(x, c1));
	t16_24 = _mm256_add_epi32(t16_24, _mm256_mullo_epi32(x, c2));
	x = _mm256_shuffle_epi8(d0, BCAST32_5);
	t4_12 = _mm256_add_epi32(t4_12, _mm256_mullo_epi32(x, c0));
	t8_16 = _mm256_add_epi32(t8_16, _mm256_mullo_epi32(x, c1));
	t20_28 = _mm256_add_epi32(t20_28, _mm256_mullo_epi32(x, c2));
	x = _mm256_shuffle_epi8(d1, BCAST32_1);
	t8_16 = _mm256_add_epi32(t8_16, _mm256_mullo_epi32(x, c0));
	t12_20 = _mm256_add_epi32(t12_20, _mm256_mullo_epi32(x, c1));
	t24_32 = _mm256_add_epi32(t24_32, _mm256_mullo_epi32(x, c2));
	x = _mm256_shuffle_epi8(d1, BCAST32_5);
	t12_20 = _mm256_add_epi32(t12_20, _mm256_mullo_epi32(x, c0));
	t16_24 = _mm256_add_epi32(t16_24, _mm256_mullo_epi32(x, c1));
	t28 = _mm256_add_epi32(t28, _mm256_mullo_epi32(x, c2));
	x = _mm256_shuffle_epi8(d2, BCAST32_1);
	t16_24 = _mm256_add_epi32(t16_24, _mm256_mullo_epi32(x, c0));
	t20_28 = _mm256_add_epi32(t20_28, _mm256_mullo_epi32(x, c1));
	t32 = _mm256_add_epi32(t32, _mm256_mullo_epi32(x, c2));

	/*
	 * Morph z*a into (z^2)*a  (truncated: a18 is dropped)
	 *   c0:  0 0 a0 a1 : a6 a7 a8 a9
	 *   c1:  a2 a3 a4 a5 : a10 a11 a12 a13
	 *   c2:  a14 a15 a16 a17 : 0 0 0 0
	 */
	x = _mm256_bsrli_epi128(c0, 12);
	y = _mm256_bsrli_epi128(c1, 12);
	c0 = _mm256_bslli_epi128(c0, 4);
	c1 = _mm256_bslli_epi128(c1, 4);
	c2 = _mm256_bslli_epi128(c2, 4);
	c0 = _mm256_or_si256(c0, _mm256_permute2x128_si256(y, y, 0x28));
	c1 = _mm256_or_si256(c1, x);
	c2 = _mm256_or_si256(c2, _mm256_permute2x128_si256(y, y, 0x83));

	/*
	 * Multiply (z^2)*a by b2, b6, b10, b14 and b18.
	 */
	x = _mm256_shuffle_epi8(d0, BCAST32_2);
	t0_8 = _mm256_add_epi32(t0_8, _mm256_mullo_epi32(x, c0));
	t4_12 = _mm256_add_epi32(t4_12, _mm256_mullo_epi32(x, c1));
	t16_24 = _mm256_add_epi32(t16_24, _mm256_mullo_epi32(x, c2));
	x = _mm256_shuffle_epi8(d0, BCAST32_6);
	t4_12 = _mm256_add_epi32(t4_12, _mm256_mullo_epi32(x, c0));
	t8_16 = _mm256_add_epi32(t8_16, _mm256_mullo_epi32(x, c1));
	t20_28 = _mm256_add_epi32(t20_28, _mm256_mullo_epi32(x, c2));
	x = _mm256_shuffle_epi8(d1, BCAST32_2);
	t8_16 = _mm256_add_epi32(t8_16, _mm256_mullo_epi32(x, c0));
	t12_20 = _mm256_add_epi32(t12_20, _mm256_mullo_epi32(x, c1));
	t24_32 = _mm256_add_epi32(t24_32, _mm256_mullo_epi32(x, c2));
	x = _mm256_shuffle_epi8(d1, BCAST32_6);
	t12_20 = _mm256_add_epi32(t12_20, _mm256_mullo_epi32(x, c0));
	t16_24 = _mm256_add_epi32(t16_24, _mm256_mullo_epi32(x, c1));
	t28 = _mm256_add_epi32(t28, _mm256_mullo_epi32(x, c2));
	x = _mm256_shuffle_epi8(d2, BCAST32_2);
	t16_24 = _mm256_add_epi32(t16_24, _mm256_mullo_epi32(x, c0));
	t20_28 = _mm256_add_epi32(t20_28, _mm256_mullo_epi32(x, c1));
	t32 = _mm256_add_epi32(t32, _mm256_mullo_epi32(x, c2));

	/*
	 * Morph (z^2)*a into (z^3)*a  (truncated: a17 is dropped)
	 *   c0:  0 0 0 a0 : a5 a6 a7 a8
	 *   c1:  a1 a2 a3 a4 : a9 a10 a11 a12
	 *   c2:  a13 a14 a15 a16 : 0 0 0 0
	 */
	x = _mm256_bsrli_epi128(c0, 12);
	y = _mm256_bsrli_epi128(c1, 12);
	c0 = _mm256_bslli_epi128(c0, 4);
	c1 = _mm256_bslli_epi128(c1, 4);
	c2 = _mm256_bslli_epi128(c2, 4);
	c0 = _mm256_or_si256(c0, _mm256_permute2x128_si256(y, y, 0x28));
	c1 = _mm256_or_si256(c1, x);
	c2 = _mm256_or_si256(c2, _mm256_permute2x128_si256(y, y, 0x83));

	/*
	 * Multiply (z^3)*a by b3, b7, b11 and b15.
	 */
	x = _mm256_shuffle_epi8(d0, BCAST32_3);
	t0_8 = _mm256_add_epi32(t0_8, _mm256_mullo_epi32(x, c0));
	t4_12 = _mm256_add_epi32(t4_12, _mm256_mullo_epi32(x, c1));
	t16_24 = _mm256_add_epi32(t16_24, _mm256_mullo_epi32(x, c2));
	x = _mm256_shuffle_epi8(d0, BCAST32_7);
	t4_12 = _mm256_add_epi32(t4_12, _mm256_mullo_epi32(x, c0));
	t8_16 = _mm256_add_epi32(t8_16, _mm256_mullo_epi32(x, c1));
	t20_28 = _mm256_add_epi32(t20_28, _mm256_mullo_epi32(x, c2));
	x = _mm256_shuffle_epi8(d1, BCAST32_3);
	t8_16 = _mm256_add_epi32(t8_16, _mm256_mullo_epi32(x, c0));
	t12_20 = _mm256_add_epi32(t12_20, _mm256_mullo_epi32(x, c1));
	t24_32 = _mm256_add_epi32(t24_32, _mm256_mullo_epi32(x, c2));
	x = _mm256_shuffle_epi8(d1, BCAST32_7);
	t12_20 = _mm256_add_epi32(t12_20, _mm256_mullo_epi32(x, c0));
	t16_24 = _mm256_add_epi32(t16_24, _mm256_mullo_epi32(x, c1));
	t28 = _mm256_add_epi32(t28, _mm256_mullo_epi32(x, c2));

	/*
	 * Since we dropped a7 and a18, we lack some products:
	 *   a17 by b3, b7, b11 and b15
	 *   a18 by b2, b3, b6, b7, b10, b11, b14, b15 and b18
	 */

	/*
	 * Load a16..a18, 16 bits per element, duplicated to both lanes.
	 */
	c2 = _mm256_zextsi128_si256(a->u1);
	c2 = _mm256_permute2x128_si256(c2, c2, 0x00);

	/*
	 * Set the following (32 bits per element):
	 *   d0:  b3 0 0 0 : b11 0 0 0
	 *   d1:  b7 0 0 0 : b15 0 0 0
	 *   d2:  b2 b3 0 0 : b10 b11 0 0
	 *   d3:  b6 b7 0 0 : b14 b15 0 0
	 *   d4:  b18 0 0 0 : 0 0 0 0
	 */
	d4 = _mm256_bsrli_epi128(d2, 4);
	x = _mm256_blend_epi32(d0, d1, 0xF0);
	d0 = _mm256_shuffle_epi8(x, _mm256_setr_epi32(
		-2139093242, -2139062144, -2139062144, -2139062144,
		-2139093242, -2139062144, -2139062144, -2139062144));
	d1 = _mm256_shuffle_epi8(x, _mm256_setr_epi32(
		-2139091186, -2139062144, -2139062144, -2139062144,
		-2139091186, -2139062144, -2139062144, -2139062144));
	d2 = _mm256_shuffle_epi8(x, _mm256_setr_epi32(
		-2139093756, -2139093242, -2139062144, -2139062144,
		-2139093756, -2139093242, -2139062144, -2139062144));
	d3 = _mm256_shuffle_epi8(x, _mm256_setr_epi32(
		-2139091700, -2139091186, -2139062144, -2139062144,
		-2139091700, -2139091186, -2139062144, -2139062144));

	/*
	 * Process a17 and a18.
	 */
	x = _mm256_shuffle_epi8(c2, BCAST32_1);
	t20_28 = _mm256_add_epi32(t20_28, _mm256_mullo_epi32(x, d0));
	t24_32 = _mm256_add_epi32(t24_32, _mm256_mullo_epi32(x, d1));
	x = _mm256_shuffle_epi8(c2, BCAST32_2);
	t20_28 = _mm256_add_epi32(t20_28, _mm256_mullo_epi32(x, d2));
	t24_32 = _mm256_add_epi32(t24_32, _mm256_mullo_epi32(x, d3));
	t36 = _mm256_mullo_epi32(x, d4);

	/*
	 * Do cross-lane moves and adds.
	 */
	t0_8 = _mm256_add_epi32(t0_8,
		_mm256_permute2x128_si256(t8_16, t8_16, 0x08));
	t4_12 = _mm256_add_epi32(t4_12,
		_mm256_permute2x128_si256(t12_20, t12_20, 0x08));
	t16_24 = _mm256_add_epi32(t16_24,
		_mm256_permute2x128_si256(t8_16, t24_32, 0x21));
	t20_28 = _mm256_add_epi32(t20_28,
		_mm256_permute2x128_si256(t12_20, t28, 0x21));
	t32 = _mm256_add_epi32(t32,
		_mm256_permute2x128_si256(t24_32, t24_32, 0x81));

	/*
	 * Unreduced value is in:
	 *   t0_8
	 *   t4_12
	 *   t16_24
	 *   t20_28
	 *   t32
	 *   t36
	 *
	 * We apply reduction modulo z^19-2.
	 */
	t19_27 = _mm256_or_si256(
		_mm256_bsrli_epi128(t16_24, 12),
		_mm256_bslli_epi128(t20_28, 4));
	t23_31 = _mm256_add_epi32(
		_mm256_permute2x128_si256(
			_mm256_bslli_epi128(t16_24, 4),
			_mm256_bslli_epi128(t32, 4),
			0x21),
		_mm256_bsrli_epi128(t20_28, 12));
	t35 = _mm256_or_si256(
		_mm256_bsrli_epi128(t32, 12),
		_mm256_bslli_epi128(t36, 4));
	t19_27 = _mm256_add_epi32(t19_27, t19_27);
	t23_31 = _mm256_add_epi32(t23_31, t23_31);
	t35 = _mm256_add_epi32(t35, t35);
	t0_8 = _mm256_add_epi32(t0_8, t19_27);
	t4_12 = _mm256_add_epi32(t4_12, t23_31);
	t16_24 = _mm256_add_epi32(t16_24, t35);

	/*
	 * Result is in t0_8, t4_12 and t16_24; we still need to apply
	 * Montgomery reduction to the coefficients.
	 *
	 * If p1i = p0:p1, then 32-bit value x0:x1 must first be replaced
	 * with 16-bit value (((x0*p0) >> 16) + x0*p1 + x1*p0) % 2^16.
	 */
	p1i_rev = _mm256_set1_epi32(0x1669D8AF);
	p1i_low = _mm256_set1_epi32(0x00001669);

	t0_8 = _mm256_add_epi16(
		_mm256_mullo_epi16(t0_8, p1i_rev),
		_mm256_mulhi_epu16(t0_8, p1i_low));
	t4_12 = _mm256_add_epi16(
		_mm256_mullo_epi16(t4_12, p1i_rev),
		_mm256_mulhi_epu16(t4_12, p1i_low));
	t16_24 = _mm256_add_epi16(
		_mm256_mullo_epi16(t16_24, p1i_rev),
		_mm256_mulhi_epu16(t16_24, p1i_low));

	/*
	 * We do the additions in the upper halves of each 32-bit word,
	 * followed by a right shift, because we want to zero-extend the
	 * values so that we may use _mm256_packus_epi32().
	 */
	t0_8 = _mm256_add_epi16(t0_8, _mm256_bslli_epi128(t0_8, 2));
	t4_12 = _mm256_add_epi16(t4_12, _mm256_bslli_epi128(t4_12, 2));
	t16_24 = _mm256_add_epi16(t16_24, _mm256_bslli_epi128(t16_24, 2));
	t0_8 = _mm256_srli_epi32(t0_8, 16);
	t4_12 = _mm256_srli_epi32(t4_12, 16);
	t16_24 = _mm256_srli_epi32(t16_24, 16);

	/*
	 * Pack the 16-bit values according to our convention.
	 */
	t0_8 = _mm256_packus_epi32(t0_8, t4_12);
	t16_24 = _mm256_packus_epi32(t16_24, t16_24);

	/*
	 * Finish Montgomery reduction.
	 */
	p = _mm256_set1_epi16(9767);
	one = _mm256_set1_epi16(1);

	d->u0 = _mm256_add_epi16(one, _mm256_mulhi_epu16(t0_8, p));
	d->u1 = _mm256_castsi256_si128(
		_mm256_add_epi16(one, _mm256_mulhi_epu16(t16_24, p)));
}

/*
 * We did not find any way to make squaring substantially faster than
 * multiplication on AVX2 (we can slightly reduce the number of
 * 16x16->32 multiplications, but at the cost of more data movement,
 * especially cross-lane data moves, which are expensive). Best
 * implementation of squaring was less than 5% faster than
 * multiplications. Therefore, we simply use vgf_mul() for squarings.
 */
static inline void
vgf_sqr(vgf *d, const vgf *a)
{
	vgf_mul(d, a, a);
}

/*
 * Frobenius coefficients are pre-multiplied with p1i for Montgomery
 * reduction, and split into low and high halves.
 */
typedef union {
	uint16_t u16[48];
	__m128i u128[6];
	__m256i u256[3];
} frobcoeffs;

static const frobcoeffs vfrob1 = { {
	46526, 65019,  1489, 39112, 20009, 43594, 40279, 29409,
	 9682, 24853, 64522, 53988, 32442, 27322, 30167, 30476,
	    6, 17002, 28430, 17613,  2838, 48841, 32811, 44983,
	20955, 16184, 51216, 22558, 16667, 30248, 37904, 39662,
	26182, 23243, 47036,     0,     0,     0,     0,     0,
	37998, 15560, 42802,     0,     0,     0,     0,     0
} };

static const frobcoeffs vfrob2 = { {
	46526,  1489, 20009, 40279,  9682, 64522, 32442, 30167,
	26182, 47036, 65019, 39112, 43594, 29409, 24853, 53988,
	    6, 28430,  2838, 32811, 20955, 51216, 16667, 37904,
	37998, 42802, 17002, 17613, 48841, 44983, 16184, 22558,
	27322, 30476, 23243,     0,     0,     0,     0,     0,
	30248, 39662, 15560,     0,     0,     0,     0,     0
} };

static const frobcoeffs vfrob4 = { {
	46526, 20009,  9682, 32442, 26182, 65019, 43594, 24853,
	27322, 23243,  1489, 40279, 64522, 30167, 47036, 39112,
	    6,  2838, 20955, 16667, 37998, 17002, 48841, 16184,
	30248, 15560, 28430, 32811, 51216, 37904, 42802, 17613,
	29409, 53988, 30476,     0,     0,     0,     0,     0,
	44983, 22558, 39662,     0,     0,     0,     0,     0
} };

static const frobcoeffs vfrob8 = { {
	46526,  9682, 26182, 43594, 27322,  1489, 64522, 47036,
	29409, 30476, 20009, 32442, 65019, 24853, 23243, 40279,
	    6, 20955, 37998, 48841, 30248, 28430, 51216, 42802,
	44983, 39662,  2838, 16667, 17002, 16184, 15560, 32811,
	30167, 39112, 53988,     0,     0,     0,     0,     0,
	37904, 17613, 22558,     0,     0,     0,     0,     0
} };

static const frobcoeffs vfrob9 = { {
	46526, 24853, 47036,  9682, 23243, 29409, 26182, 40279,
	30476, 43594, 30167, 20009, 27322, 39112, 32442,  1489,
	    6, 16184, 42802, 20955, 15560, 44983, 37998, 32811,
	39662, 48841, 37904,  2838, 30248, 17613, 16667, 28430,
	53988, 65019, 64522,     0,     0,     0,     0,     0,
	22558, 17002, 51216,     0,     0,     0,     0,     0
} };

static inline void
vgf_frob(vgf *d, const vgf *a, const frobcoeffs *kf)
{
	__m256i a0, p, one;
	__m128i a1;

	a0 = a->u0;
	a1 = a->u1;
	a0 = _mm256_add_epi16(
		_mm256_mulhi_epu16(a0, kf->u256[0]),
		_mm256_mullo_epi16(a0, kf->u256[1]));
	a1 = _mm_add_epi16(
		_mm_mulhi_epu16(a1, kf->u128[4]),
		_mm_mullo_epi16(a1, kf->u128[5]));
	p = _mm256_set1_epi16(9767);
	one = _mm256_set1_epi16(1);
	d->u0 = _mm256_add_epi16(one, _mm256_mulhi_epu16(a0, p));
	d->u1 = _mm_add_epi16(_mm256_castsi256_si128(one),
		_mm_mulhi_epu16(a1, _mm256_castsi256_si128(p)));
}

/*
 * Compute the low coefficient of a*b (mod z^19-2). This is:
 *   a0*a0 + 2*(a1*b18 + a2*b17 + a3*b16 + ... + a18*b1)
 * This function is used for products which are already known to be
 * elements of the base field Z_p.
 */
static inline uint32_t
vgf_mul_to_low(const vgf *a, const vgf *b)
{
	__m256i a0, b0, c0, d00, d01, e;
	__m128i a1, b1, c1, zt3, d10, d11, f;

	/*
	 * Read values and mask them appropriately to ensure the ignored
	 * elements are zeros.
	 */
	zt3 = _mm_setr_epi16(-1, -1, -1, 0, 0, 0, 0, 0);
	a0 = a->u0;
	a1 = _mm_and_si128(a->u1, zt3);
	b0 = b->u0;
	b1 = _mm_and_si128(b->u1, zt3);

	/*
	 * Shuffle the elements of b; also multiply them by 2 (except b0):
	 *
	 *   c0 : b0  b18 b17 b16 b15 b14 b13 b12 : b11 b10 b9 b8 b7 b6 b5 b4
	 *   c1 : b3  b2  b1  0   0   0   0   0
	 */
	c0 = _mm256_shuffle_epi8(
		_mm256_blend_epi32(
			_mm256_castsi128_si256(b1),
			_mm256_permute4x64_epi64(b0, 0x6F), 0xFC),
		_mm256_setr_epi8(
			0x80, 0x80, 4, 5, 2, 3, 0, 1,
			14, 15, 12, 13, 10, 11, 8, 9,
			6, 7, 4, 5, 2, 3, 0, 1,
			14, 15, 12, 13, 10, 11, 8, 9));
	c0 = _mm256_add_epi16(c0, c0);
	c0 = _mm256_or_si256(c0,
		_mm256_and_si256(b0, _mm256_setr_epi16(
			-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)));
	c1 = _mm_shuffle_epi8(_mm256_castsi256_si128(b0),
		_mm_setr_epi8(6, 7, 4, 5, 2, 3, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80));
	c1 = _mm_add_epi16(c1, c1);

	/*
	 * Perform all multiplications and add values together.
	 */
	d00 = _mm256_mullo_epi16(a0, c0);
	d01 = _mm256_mulhi_epu16(a0, c0);
	d10 = _mm_mullo_epi16(a1, c1);
	d11 = _mm_mulhi_epu16(a1, c1);

	e = _mm256_add_epi32(
		_mm256_unpacklo_epi16(d00, d01),
		_mm256_unpackhi_epi16(d00, d01));
	f = _mm_add_epi32(
		_mm_add_epi32(
			_mm256_castsi256_si128(e),
			_mm_add_epi32(
				_mm_unpacklo_epi16(d10, d11),
				_mm_unpackhi_epi16(d10, d11))),
		_mm256_extracti128_si256(e, 1));
	f = _mm_add_epi32(f, _mm_bsrli_si128(f, 8));
	f = _mm_add_epi32(f, _mm_bsrli_si128(f, 4));

	/*
	 * Perform Montgomery reduction with integers.
	 */
	return mp_frommonty((uint32_t)_mm_cvtsi128_si32(f));
}

/*
 * Multiply a value by a constant. The constant must be in Montgomery
 * representation.
 */
static inline void
vgf_mul_const(vgf *d, const vgf *a, uint32_t c)
{
	__m256i a0, cc0, cc1, p, one;
	__m128i a1;
	int16_t c0, c1;

	a0 = a->u0;
	a1 = a->u1;
	c *= (uint32_t)P1I;
	*(uint16_t *)&c0 = (uint16_t)c;
	*(uint16_t *)&c1 = (uint16_t)(c >> 16);
	cc0 = _mm256_set1_epi16(c0);
	cc1 = _mm256_set1_epi16(c1);
	a0 = _mm256_add_epi16(
		_mm256_mulhi_epu16(a0, cc0),
		_mm256_mullo_epi16(a0, cc1));
	a1 = _mm_add_epi16(
		_mm_mulhi_epu16(a1, _mm256_castsi256_si128(cc0)),
		_mm_mullo_epi16(a1, _mm256_castsi256_si128(cc1)));

	p = _mm256_set1_epi16(9767);
	one = _mm256_set1_epi16(1);
	d->u0 = _mm256_add_epi16(one, _mm256_mulhi_epu16(a0, p));
	d->u1 = _mm_add_epi16(_mm256_castsi256_si128(one),
		_mm_mulhi_epu16(a1, _mm256_castsi256_si128(p)));
}

/*
 * Similar to vgf_mul_const(), but without the Montgomery reduction.
 * Unreduced result is in d[], an array of three 256-bit values. This
 * is used to mutualize Montgomery reduction when a linear combination
 * of field elements must be computed.
 */
static inline void
vgf_mul_const_noreduce(__m256i *d, const vgf *a, uint32_t c)
{
	int16_t s;
	__m256i a0, a1, b, t00, t01, t10, t11;

	*(uint16_t *)&s = (uint16_t)c;
	a0 = a->u0;
	a1 = _mm256_castsi128_si256(a->u1);
	b = _mm256_set1_epi16(s);
	t00 = _mm256_mullo_epi16(a0, b);
	t01 = _mm256_mulhi_epu16(a0, b);
	t10 = _mm256_mullo_epi16(a1, b);
	t11 = _mm256_mulhi_epu16(a1, b);
	d[0] = _mm256_unpacklo_epi16(t00, t01);
	d[1] = _mm256_unpackhi_epi16(t00, t01);
	d[2] = _mm256_unpacklo_epi16(t10, t11);
}

/*
 * Like vgf_mul_const_noreduce(), but intermediate results are added to
 * the d[] words, instead of replacing them.
 */
static inline void
vgf_mulacc_const_noreduce(__m256i *d, const vgf *a, uint32_t c)
{
	int16_t s;
	__m256i a0, a1, b, t00, t01, t10, t11;

	*(uint16_t *)&s = (uint16_t)c;
	a0 = a->u0;
	a1 = _mm256_castsi128_si256(a->u1);
	b = _mm256_set1_epi16(s);
	t00 = _mm256_mullo_epi16(a0, b);
	t01 = _mm256_mulhi_epu16(a0, b);
	t10 = _mm256_mullo_epi16(a1, b);
	t11 = _mm256_mulhi_epu16(a1, b);
	d[0] = _mm256_add_epi32(d[0], _mm256_unpacklo_epi16(t00, t01));
	d[1] = _mm256_add_epi32(d[1], _mm256_unpackhi_epi16(t00, t01));
	d[2] = _mm256_add_epi32(d[2], _mm256_unpacklo_epi16(t10, t11));
}

/*
 * Applying Montgomery reduction on the a[] words (as produced by
 * vgf_mul_const_noreduce() and vgf_mulacc_const_noreduce()).
 */
static inline void
vgf_montyred(vgf *d, const __m256i *a)
{
	__m256i d0, d1, d2, p1i_rev, p1i_low, p, one;

	d0 = a[0];
	d1 = a[1];
	d2 = a[2];

	/*
	 * Apply Montgomery reduction.
	 *
	 * If p1i = p0:p1, then 32-bit value x0:x1 must first be replaced
	 * with 16-bit value (((x0*p0) >> 16) + x0*p1 + x1*p0) % 2^16.
	 */
	p1i_rev = _mm256_set1_epi32(0x1669D8AF);
	p1i_low = _mm256_set1_epi32(0x00001669);

	d0 = _mm256_add_epi16(
		_mm256_mullo_epi16(d0, p1i_rev),
		_mm256_mulhi_epu16(d0, p1i_low));
	d1 = _mm256_add_epi16(
		_mm256_mullo_epi16(d1, p1i_rev),
		_mm256_mulhi_epu16(d1, p1i_low));
	d2 = _mm256_add_epi16(
		_mm256_mullo_epi16(d2, p1i_rev),
		_mm256_mulhi_epu16(d2, p1i_low));

	/*
	 * We do the additions in the upper halves of each 32-bit word,
	 * followed by a right shift, because we want to zero-extend the
	 * values so that we may use _mm256_packus_epi32().
	 */
	d0 = _mm256_add_epi16(d0, _mm256_bslli_epi128(d0, 2));
	d1 = _mm256_add_epi16(d1, _mm256_bslli_epi128(d1, 2));
	d2 = _mm256_add_epi16(d2, _mm256_bslli_epi128(d2, 2));
	d0 = _mm256_srli_epi32(d0, 16);
	d1 = _mm256_srli_epi32(d1, 16);
	d2 = _mm256_srli_epi32(d2, 16);

	/*
	 * Pack the 16-bit values according to our convention.
	 */
	d0 = _mm256_packus_epi32(d0, d1);
	d1 = _mm256_packus_epi32(d2, d2);

	/*
	 * Finish Montgomery reduction.
	 */
	p = _mm256_set1_epi16(9767);
	one = _mm256_set1_epi16(1);

	d->u0 = _mm256_add_epi16(one, _mm256_mulhi_epu16(d0, p));
	d->u1 = _mm256_castsi256_si128(
		_mm256_add_epi16(one, _mm256_mulhi_epu16(d1, p)));
}

/*
 * Multiply input by z^9, with reduction modulo z^19-2, but without
 * modular reduction of coefficients (i.e. coefficients 0..8 in the
 * result may range up to 2*p).
 */
static inline void
vgf_mulz9(vgf *d, const vgf *a)
{
 	__m256i a0, c0, t0;
	__m128i a1, c1, zt3;

	zt3 = _mm_setr_epi16(-1, -1, -1, 0, 0, 0, 0, 0);
	a0 = a->u0;
	a1 = _mm_and_si128(a->u1, zt3);

	c0 = _mm256_inserti128_si256(
		_mm256_castsi128_si256(
			_mm_or_si128(
				_mm256_extracti128_si256(
					_mm256_bsrli_epi128(a0, 4), 1),
				_mm_bslli_si128(a1, 12))),
		_mm_bsrli_si128(a1, 4), 1);
	c0 = _mm256_add_epi16(c0, c0);

	t0 = _mm256_bslli_epi128(a0, 2);
	c0 = _mm256_or_si256(
		c0,
		_mm256_inserti128_si256(_mm256_setzero_si256(),
			_mm256_castsi256_si128(t0), 1));
	c1 = _mm_or_si128(
		_mm_bsrli_si128(_mm256_castsi256_si128(a0), 14),
		_mm256_extracti128_si256(t0, 1));

	d->u0 = c0;
	d->u1 = _mm_and_si128(c1, zt3);
}

static void
vgf_inv(vgf *d, vgf *a)
{
	vgf t1, t2;
	uint32_t y, yi;

	/* a^(1+p) -> t1 */
	vgf_frob(&t2, a, &vfrob1);
	vgf_mul(&t1, &t2, a);

	/* a^(1+p+p^2+p^3) -> t1 */
	vgf_frob(&t2, &t1, &vfrob2);
	vgf_mul(&t1, &t2, &t1);

	/* a^(1+p+p^2+p^3+p^4+p^5+p^6+p^7) -> t1 */
	vgf_frob(&t2, &t1, &vfrob4);
	vgf_mul(&t1, &t2, &t1);

	/* a^(1+p+p^2+p^3+p^4+p^5+p^6+p^7+p^8) -> t1 */
	vgf_frob(&t1, &t1, &vfrob1);
	vgf_mul(&t1, &t1, a);

	/* a^(1+p+p^2+p^3+..+p^17) -> t1 */
	vgf_frob(&t2, &t1, &vfrob9);
	vgf_mul(&t1, &t2, &t1);

	/* a^(p+p^2+p^3+..+p^17+p^18) = a^(r-1) -> t1 */
	vgf_frob(&t1, &t1, &vfrob1);

	/*
	 * Compute a^r = a*a^(r-1). Since a^r is in GF(p), we just need
	 * to compute the low coefficient (all others are zero).
	 */
	y = vgf_mul_to_low(a, &t1);

	/*
	 * Compute 1/(a^r) = (a^r)^(p-2).
	 */
	yi = mp_inv(y);

	/*
	 * Now compute 1/a = (1/(a^r))*(a^(r-1)).
	 */
	vgf_mul_const(d, &t1, yi);
}

static uint32_t
vgf_sqrt(vgf *d, const vgf *a)
{
	vgf t1, t2, t3;
	uint32_t y, yi, r;

	/* a^(1+p^2) -> t1 */
	vgf_frob(&t2, a, &vfrob2);
	vgf_mul(&t1, &t2, a);

	/* a^(1+p^2+p^4+p^6) -> t1 */
	vgf_frob(&t2, &t1, &vfrob4);
	vgf_mul(&t1, &t2, &t1);

	/* a^(1+p^2+p^4+p^6+p^8+p^10+p^12+p^14) -> t1 */
	vgf_frob(&t2, &t1, &vfrob8);
	vgf_mul(&t1, &t2, &t1);

	/* a^(1+p^2+p^4+p^6+p^8+p^10+p^12+p^14+p^16) = a^d -> t1 */
	vgf_frob(&t2, &t1, &vfrob2);
	vgf_mul(&t1, &t2, a);

	/* (a^d)^p = a^f -> t2 */
	vgf_frob(&t2, &t1, &vfrob1);

	/* a^e = a*((a^f)^p) -> t1 */
	vgf_frob(&t1, &t2, &vfrob1);
	vgf_mul(&t1, &t1, a);

	/*
	 * Compute a^r = (a^e)*(a^f). This is an element of GF(p), so
	 * we compute only the low coefficient.
	 */
	y = vgf_mul_to_low(&t1, &t2);

	/*
	 * a is a QR if and only if a^r is a QR.
	 */
	r = mp_is_qr(y);

	/*
	 * If the square root is not actually needed, but only the QR
	 * status, then return it now.
	 */
	if (d == NULL) {
		return r;
	}

	/*
	 * Compute 1/(a^r).
	 */
	yi = mp_inv(y);

	/* (a^e)^2 -> t1 */
	vgf_sqr(&t1, &t1);

	/* ((a^e)^2)/(a^r) -> t2 */
	vgf_mul_const(&t2, &t1, yi);

	/*
	 * Let x = ((a^e)^2)/(a^r) (currently in t2). All that remains
	 * to do is to raise x to the power (p+1)/4 = 2442.
	 */

	/* x^4 -> t1 */
	vgf_sqr(&t1, &t2);
	vgf_sqr(&t1, &t1);

	/* x^5 -> t3 */
	vgf_mul(&t3, &t1, &t2);

	/* x^9 -> t1 */
	vgf_mul(&t1, &t1, &t3);

	/* x^18 -> t1 */
	vgf_sqr(&t1, &t1);

	/* x^19 -> t1 */
	vgf_mul(&t1, &t1, &t2);

	/* x^(19*64) = x^1216 -> t1 */
	vgf_sqr(&t1, &t1);
	vgf_sqr(&t1, &t1);
	vgf_sqr(&t1, &t1);
	vgf_sqr(&t1, &t1);
	vgf_sqr(&t1, &t1);
	vgf_sqr(&t1, &t1);

	/* x^1221 -> t1 */
	vgf_mul(&t1, &t1, &t3);

	/* x^2442 -> out */
	vgf_sqr(d, &t1);

	/*
	 * Return the quadratic residue status.
	 */
	return r;
}

static inline void
vgf_cubert(vgf *d, const vgf *a)
{
	vgf t1, t2, t3, t4;

	/* a^(1+p^2) -> t1 */
	vgf_frob(&t2, a, &vfrob2);
	vgf_mul(&t1, &t2, a);

	/* a^(1+p^2+p^4+p^6) -> t1 */
	vgf_frob(&t2, &t1, &vfrob4);
	vgf_mul(&t1, &t2, &t1);

	/* a^(1+p^2+p^4+p^6+p^8+p^10+p^12+p^14) -> t1 */
	vgf_frob(&t2, &t1, &vfrob8);
	vgf_mul(&t1, &t2, &t1);

	/* a^(1+p^2+p^4+p^6+p^8+p^10+p^12+p^14+p^16) = a^d -> t1 */
	vgf_frob(&t2, &t1, &vfrob2);
	vgf_mul(&t1, &t2, a);

	/* (a^d)^p = a^f -> t2 */
	vgf_frob(&t2, &t1, &vfrob1);

	/* a^e = a*((a^f)^p) -> t1 */
	vgf_frob(&t1, &t2, &vfrob1);
	vgf_mul(&t1, &t1, a);

	/*
	 * Compute (a^e)^((2*p-1)/3) * (a^f)^((p-2)/3). Since we have:
	 *   (2*p-1)/3 = 2 * ((p-2)/3) + 1
	 * we can simply compute:
	 *   (a^e) * ((a^(2*e+f))^((p-2)/3))
	 */

	/* u = a^(2*e+f) -> t2 */
	vgf_sqr(&t3, &t1);
	vgf_mul(&t2, &t2, &t3);

	/* u^3 -> t3 */
	vgf_sqr(&t3, &t2);
	vgf_mul(&t3, &t2, &t3);
	/* u^12 -> t4 */
	vgf_sqr(&t4, &t3);
	vgf_sqr(&t4, &t4);
	/* u^13 -> t2 */
	vgf_mul(&t2, &t4, &t2);
	/* u^25 -> t4 */
	vgf_mul(&t4, &t2, &t4);
	/* u^813 -> t4 */
	vgf_sqr(&t4, &t4);
	vgf_sqr(&t4, &t4);
	vgf_sqr(&t4, &t4);
	vgf_sqr(&t4, &t4);
	vgf_sqr(&t4, &t4);
	vgf_mul(&t4, &t2, &t4);
	/* u^3255 -> t4 */
	vgf_sqr(&t4, &t4);
	vgf_sqr(&t4, &t4);
	vgf_mul(&t4, &t3, &t4);

	/*
	 * Final product by a^e.
	 */
	vgf_mul(d, &t1, &t4);
}

/*
 * All performance-critical computations in this file use the 'vgf'
 * format directly. But we declare wrappers that work on 'field_element'
 * so that functions in other files may still use computations on the
 * finite field (in particular test functions).
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
curve9767_inner_gf_add(uint16_t *d, const uint16_t *a, const uint16_t *b)
{
	vgf va, vb, vd;

	vgf_decode(&va, a);
	vgf_decode(&vb, b);
	vgf_add(&vd, &va, &vb);
	vgf_encode(d, &vd);
}

/* see inner.h */
void
curve9767_inner_gf_sub(uint16_t *d, const uint16_t *a, const uint16_t *b)
{
	vgf va, vb, vd;

	vgf_decode(&va, a);
	vgf_decode(&vb, b);
	vgf_sub(&vd, &va, &vb);
	vgf_encode(d, &vd);
}

/* see inner.h */
void
curve9767_inner_gf_neg(uint16_t *d, const uint16_t *a)
{
	vgf va, vd;

	vgf_decode(&va, a);
	vgf_neg(&vd, &va);
	vgf_encode(d, &vd);
}

/* see inner.h */
void
curve9767_inner_gf_condneg(uint16_t *d, uint32_t ctl)
{
	vgf vd;

	vgf_decode(&vd, d);
	vgf_condneg(&vd, &vd, ctl);
	vgf_encode(d, &vd);
}

/* see inner.h */
void
curve9767_inner_gf_mul(uint16_t *c, const uint16_t *a, const uint16_t *b)
{
	vgf va, vb, vc;

	vgf_decode(&va, a);
	vgf_decode(&vb, b);
	vgf_mul(&vc, &va, &vb);
	vgf_encode(c, &vc);
}

/* see inner.h */
void
curve9767_inner_gf_sqr(uint16_t *c, const uint16_t *a)
{
	vgf va, vc;

	vgf_decode(&va, a);
	vgf_sqr(&vc, &va);
	vgf_encode(c, &vc);
}

/* see inner.h */
void
curve9767_inner_gf_inv(uint16_t *c, const uint16_t *a)
{
	vgf va, vc;

	vgf_decode(&va, a);
	vgf_inv(&vc, &va);
	vgf_encode(c, &vc);
}

/* see inner.h */
uint32_t
curve9767_inner_gf_sqrt(uint16_t *c, const uint16_t *a)
{
	vgf va, vc;

	vgf_decode(&va, a);
	if (c == NULL) {
		return vgf_sqrt(NULL, &va);
	} else {
		uint32_t r;

		r = vgf_sqrt(&vc, &va);
		vgf_encode(c, &vc);
		return r;
	}
}

/* see inner.h */
void
curve9767_inner_gf_cubert(uint16_t *c, const uint16_t *a)
{
	vgf vc, va;

	vgf_decode(&va, a);
	vgf_cubert(&vc, &va);
	vgf_encode(c, &vc);
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

#define D4_EIGHTEENBm        ((uint32_t)8120)
#define D16_R                ((uint32_t)8995)

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

/*
 * Internal representation of a curve point, using the 'vgf' format for
 * coordinates.
 */
typedef struct {
	vgf x;
	vgf y;
	uint32_t neutral;
} vpoint;

static inline void
vpoint_set_neutral(vpoint *Q)
{
	Q->x.u0 = _mm256_setzero_si256();
	Q->x.u1 = _mm_setzero_si128();
	Q->y.u0 = _mm256_setzero_si256();
	Q->y.u1 = _mm_setzero_si128();
	Q->neutral = 1;
}

static inline void
vpoint_decode(vpoint *vQ, const curve9767_point *Q)
{
	vgf_decode(&vQ->x, Q->x);
	vgf_decode(&vQ->y, Q->y);
	vQ->neutral = Q->neutral;
}

static inline void
vpoint_encode(curve9767_point *Q, const vpoint *vQ)
{
	vgf_encode(Q->x, &vQ->x);
	vgf_encode(Q->y, &vQ->y);
	Q->neutral = vQ->neutral;
}

static inline void
vpoint_neg(vpoint *Q3, const vpoint *Q1)
{
	Q3->x = Q1->x;
	vgf_neg(&Q3->y, &Q1->y);
	Q3->neutral = Q1->neutral;
}

static inline void
vpoint_add(vpoint *Q3, const vpoint *Q1, const vpoint *Q2)
{
	vgf x1, y1, x2, y2, t1, t2, t3;
	__m256i ex, ey, p8, pp8, om, n0, n1, n2;
	__m128i zt3, tt;
	uint32_t iex, iey;

	/*
	 * Get coordinates; ensure ignored elements are zero.
	 */
	zt3 = _mm_setr_epi16(-1, -1, -1, 0, 0, 0, 0, 0);
	x1.u0 = Q1->x.u0;
	x1.u1 = _mm_and_si128(Q1->x.u1, zt3);
	y1.u0 = Q1->y.u0;
	y1.u1 = _mm_and_si128(Q1->y.u1, zt3);
	x2.u0 = Q2->x.u0;
	x2.u1 = _mm_and_si128(Q2->x.u1, zt3);
	y2.u0 = Q2->y.u0;
	y2.u1 = _mm_and_si128(Q2->y.u1, zt3);

	/*
	 * Set ex = -1 if x1 = x2, 0 otherwise.
	 */
	ex = _mm256_cmpeq_epi32(x1.u0, x2.u0);
	tt = _mm_and_si128(
		_mm_and_si128(
			_mm256_castsi256_si128(ex),
			_mm_cmpeq_epi32(x1.u1, x2.u1)),
		_mm256_extracti128_si256(ex, 1));
	tt = _mm_and_si128(tt, _mm_bsrli_si128(tt, 8));
	tt = _mm_and_si128(tt, _mm_bsrli_si128(tt, 4));
	tt = _mm_shuffle_epi32(tt, 0x00);
	ex = _mm256_inserti128_si256(_mm256_castsi128_si256(tt), tt, 1);

	/*
	 * Set ey = -1 if y1 = y2, 0 otherwise.
	 */
	ey = _mm256_cmpeq_epi32(y1.u0, y2.u0);
	tt = _mm_and_si128(
		_mm_and_si128(
			_mm256_castsi256_si128(ey),
			_mm_cmpeq_epi32(y1.u1, y2.u1)),
		_mm256_extracti128_si256(ey, 1));
	tt = _mm_and_si128(tt, _mm_bsrli_si128(tt, 8));
	tt = _mm_and_si128(tt, _mm_bsrli_si128(tt, 4));
	tt = _mm_shuffle_epi32(tt, 0x00);
	ey = _mm256_inserti128_si256(_mm256_castsi128_si256(tt), tt, 1);

	/*
	 * t1 <- (x2-x1)  if x1 != x2
	 *       2*y1     if x1 == x2
	 */
	vgf_sub(&t1, &x2, &x1);
	vgf_add(&t3, &y1, &y1);
	t1.u0 = _mm256_xor_si256(t1.u0,
		_mm256_and_si256(ex, _mm256_xor_si256(t1.u0, t3.u0)));
	t1.u1 = _mm_xor_si128(t1.u1,
		_mm_and_si128(_mm256_castsi256_si128(ex),
			_mm_xor_si128(t1.u1, t3.u1)));

	/*
	 * t3 <- 3*x1^2 + a
	 *
	 * a = 7755 (in Montgomery representation).
	 * Take care that 3*u + 7755, with 1 <= u <= 9767, can range up
	 * to 37056, which fits in 16 bits but not with signed
	 * interpretation; i.e. cmpgt_epi16() would fail to apply the
	 * reduction. To do the reduction properly, we first subtract
	 * pp8 (i.e. 2*9767) if the value is negative (in signed
	 * interpretation). This only applies to t3.u0, not t3.u1,
	 * since all coefficients of non-zero degree are at most
	 * 3*9767 = 29301, which fits in 15 bits.
	 */
	p8 = _mm256_set1_epi16(9767);
	pp8 = _mm256_add_epi16(p8, p8);
	vgf_sqr(&t3, &x1);
	t3.u0 = _mm256_add_epi16(
		_mm256_add_epi16(t3.u0, t3.u0),
		_mm256_add_epi16(t3.u0, _mm256_setr_epi16(
			Am, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0)));
	t3.u1 = _mm_add_epi16(_mm_add_epi16(t3.u1, t3.u1), t3.u1);
	t3.u0 = _mm256_sub_epi16(t3.u0,
		_mm256_and_si256(_mm256_srai_epi16(t3.u0, 15), pp8));
	t3.u0 = _mm256_sub_epi16(t3.u0,
		_mm256_and_si256(_mm256_cmpgt_epi16(t3.u0, pp8), pp8));
	t3.u0 = _mm256_sub_epi16(t3.u0,
		_mm256_and_si256(_mm256_cmpgt_epi16(t3.u0, p8), p8));
	t3.u1 = _mm_sub_epi16(t3.u1,
		_mm_and_si128(_mm_cmplt_epi16(
			_mm256_castsi256_si128(pp8), t3.u1), 
			_mm256_castsi256_si128(pp8)));
	t3.u1 = _mm_sub_epi16(t3.u1,
		_mm_and_si128(_mm_cmplt_epi16(
			_mm256_castsi256_si128(p8), t3.u1), 
			_mm256_castsi256_si128(p8)));

	/*
	 * t2 <- (y2-y1)   if x1 != x2
	 *       3*x1^2+a  if x1 == x2
	 */
	vgf_sub(&t2, &y2, &y1);
	t2.u0 = _mm256_xor_si256(t2.u0,
		_mm256_and_si256(ex, _mm256_xor_si256(t2.u0, t3.u0)));
	t2.u1 = _mm_xor_si128(t2.u1,
		_mm_and_si128(_mm256_castsi256_si128(ex),
			_mm_xor_si128(t2.u1, t3.u1)));

	/*
	 * t1 <- lambda
	 */
	vgf_inv(&t1, &t1);
	vgf_mul(&t1, &t1, &t2);

	/*
	 * x3 = lambda^2 - x1 - x2  (in t2)
	 * y3 = lambda*(x1 - x3) - y1  (in t3)
	 */
	vgf_sqr(&t2, &t1);
	vgf_sub(&t2, &t2, &x1);
	vgf_sub(&t2, &t2, &x2);
	vgf_sub(&t3, &x1, &t2);
	vgf_mul(&t3, &t3, &t1);
	vgf_sub(&t3, &t3, &y1);

	/*
	 * If Q1 != 0, Q2 != 0, and Q1 != -Q2, then (t2,t3) contains the
	 * correct result.
	 *
	 * If Q1 == 0 then we copy the coordinates of Q2.
	 * If Q2 == 0 then we copy the coordinates of Q1.
	 */
	om = _mm256_set1_epi32(-1);
	n1 = _mm256_set1_epi32(-(int)Q1->neutral);
	n2 = _mm256_set1_epi32(-(int)Q2->neutral);
	n0 = _mm256_xor_si256(_mm256_or_si256(n1, n2), om);
	Q3->x.u0 = _mm256_or_si256(
		_mm256_or_si256(
			_mm256_and_si256(n1, x2.u0),
			_mm256_and_si256(n2, x1.u0)),
		_mm256_and_si256(n0, t2.u0));
	Q3->x.u1 = _mm_or_si128(
		_mm_or_si128(
			_mm_and_si128(_mm256_castsi256_si128(n1), x2.u1),
			_mm_and_si128(_mm256_castsi256_si128(n2), x1.u1)),
		_mm_and_si128(_mm256_castsi256_si128(n0), t2.u1));
	Q3->y.u0 = _mm256_or_si256(
		_mm256_or_si256(
			_mm256_and_si256(n1, y2.u0),
			_mm256_and_si256(n2, y1.u0)),
		_mm256_and_si256(n0, t3.u0));
	Q3->y.u1 = _mm_or_si128(
		_mm_or_si128(
			_mm_and_si128(_mm256_castsi256_si128(n1), y2.u1),
			_mm_and_si128(_mm256_castsi256_si128(n2), y1.u1)),
		_mm_and_si128(_mm256_castsi256_si128(n0), t3.u1));

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
	iex = _mm_cvtsi128_si32(_mm256_castsi256_si128(ex)) & 1;
	iey = _mm_cvtsi128_si32(_mm256_castsi256_si128(ey)) & 1;
	Q3->neutral = (Q1->neutral & Q2->neutral)
		| ((1 - Q1->neutral) & (1 - Q2->neutral) & iex & (1 - iey));
}

static inline void
vpoint_mul2k(vpoint *Q3, const vpoint *Q1, unsigned k)
{
	vgf X, Y, Z, XX, XXXX, YYYY, ZZ, S, M;
	__m128i zt3;
	__m256i tx[3], ty[3];
	unsigned cc;

	if (k == 0) {
		*Q3 = *Q1;
		return;
	}
	if (k == 1) {
		vpoint_add(Q3, Q1, Q1);
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
	zt3 = _mm_setr_epi16(-1, -1, -1, 0, 0, 0, 0, 0);
	X.u0 = Q1->x.u0;
	X.u1 = _mm_and_si128(Q1->x.u1, zt3);
	Y.u0 = Q1->y.u0;
	Y.u1 = _mm_and_si128(Q1->y.u1, zt3);
	vgf_add(&Z, &Y, &Y);
	vgf_sqr(&XX, &X);
	vgf_sqr(&XXXX, &XX);
	vgf_sqr(&ZZ, &Z);
	vgf_sqr(&S, &ZZ);
	vgf_mulz9(&YYYY, &X);
	vgf_mulz9(&M, &ZZ);

	vgf_mul_const_noreduce(tx, &XXXX, R);
	vgf_mulacc_const_noreduce(tx, &XX, SIXm);
	vgf_mulacc_const_noreduce(tx, &YYYY, MEIGHTBm);
	tx[0] = _mm256_add_epi32(tx[0],
		_mm256_setr_epi32(NINEmm, 0, 0, 0, 0, 0, 0, 0));
	vgf_montyred(&X, tx);

	vgf_mul_const_noreduce(ty, &S, D16_R);
	vgf_mulacc_const_noreduce(ty, &M, D4_EIGHTEENBm);
	vgf_mulacc_const_noreduce(ty, &XXXX, MNINEm);
	vgf_mulacc_const_noreduce(ty, &XX, MFIFTYFOURm);
	vgf_mulacc_const_noreduce(ty, &YYYY, SEVENTYTWOBm);
	ty[0] = _mm256_add_epi32(ty[0],
		_mm256_setr_epi32(TWENTYSEVENmm, 0, 0, 0, 0, 0, 0, 0));
	ty[2] = _mm256_add_epi32(ty[2],
		_mm256_setr_epi32(0, 0, MTWENTYSEVENBBmm, 0, 0, 0, 0, 0));
	vgf_montyred(&Y, ty);

	/*
	 * We perform all remaining doublings in Jacobian coordinates,
	 * using formulas from:
	 *   https://hyperelliptic.org/EFD/g1p/auto-shortw-jacobian.html
	 * Since squarings have the same cost as multiplications on this
	 * architecture, we use the 4M+4S formulas.
	 *
	 * For the first iteration, we can use the value Z^2 which is
	 * already computed in ZZ (from the first doubling).
	 */
	for (cc = 1; cc < k; cc ++) {
		/* ZZ = Z^2 */
		if (cc != 1) {
			vgf_sqr(&ZZ, &Z);
		}

		/* M = X-ZZ
		   ZZ = X+ZZ
		   M = M*ZZ
		   M = 3*M
		   (ZZ is discarded) */
		vgf_sub(&M, &X, &ZZ);
		vgf_add(&ZZ, &X, &ZZ);
		vgf_mul(&M, &M, &ZZ);
		vgf_add(&ZZ, &M, &M);
		vgf_add(&M, &M, &ZZ);

		/* Y = 2*Y
		   Z = Y*Z */
		vgf_add(&Y, &Y, &Y);
		vgf_mul(&Z, &Y, &Z);

		/* Y = Y^2
		   S = Y*X
		   Y = (Y^2)/2 */
		vgf_sqr(&Y, &Y);
		vgf_mul(&S, &Y, &X);
		vgf_sqr(&Y, &Y);
		vgf_mul_const(&Y, &Y, HALFm);

		/* X = M^2 */
		vgf_sqr(&X, &M);

		/* ZZ = 2*S
		   X = X-ZZ */
		vgf_add(&ZZ, &S, &S);
		vgf_sub(&X, &X, &ZZ);

		/* ZZ = S-X
		   ZZ = ZZ*M
		   Y = ZZ-Y */
		vgf_sub(&ZZ, &S, &X);
		vgf_mul(&ZZ, &ZZ, &M);
		vgf_sub(&Y, &ZZ, &Y);
	}

	/*
	 * Convert back to affine coordinates.
	 * The Q3->neutral flag has already been set.
	 */
	vgf_inv(&Z, &Z);
	vgf_sqr(&ZZ, &Z);
	vgf_mul(&Q3->x, &X, &ZZ);
	vgf_mul(&ZZ, &ZZ, &Z);
	vgf_mul(&Q3->y, &Y, &ZZ);
}

/* see curve9767.h */
void
curve9767_point_add(curve9767_point *Q3,
	const curve9767_point *Q1, const curve9767_point *Q2)
{
	vpoint vQ1, vQ2, vQ3;

	vpoint_decode(&vQ1, Q1);
	vpoint_decode(&vQ2, Q2);
	vpoint_add(&vQ3, &vQ1, &vQ2);
	vpoint_encode(Q3, &vQ3);
}

/* see curve9767.h */
void
curve9767_point_mul2k(curve9767_point *Q3,
	const curve9767_point *Q1, unsigned k)
{
	vpoint vQ1, vQ3;

	vpoint_decode(&vQ1, Q1);
	vpoint_mul2k(&vQ3, &vQ1, k);
	vpoint_encode(Q3, &vQ3);
}

/*
 * Custom type for a "window point", which allows declaration as static
 * constant data. There is no 'neutral' flag (this is for points which
 * are not the point at infinity).
 */
typedef union {
	uint16_t cc[64];
	vgf xy[2];
} win_vpoint;

/*
 * 1*G to 16*G.
 */
static const win_vpoint window5_G[] = {
	/* 1 */
	{ { 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,
	    9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9767,
	    9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,
	    9767, 9767, 5183, 9767, 9767, 9767, 9767,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 2 */
	{ { 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 5449,
	    9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9767,
	    9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,
	    9767, 9767, 4584, 6461, 9767, 9767, 9767,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 3 */
	{ { 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,  827,  976, 9767,
	    9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9767,
	    9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,
	    3199, 3986, 5782, 9767, 9767, 9767, 9767,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 4 */
	{ { 6847, 1976, 9464, 6412, 8169, 5071, 8735, 3293, 7935, 3726, 4904,
	    5698, 9489, 9400, 4154, 8678, 6975, 9238,  441,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 3372,
	    2850, 5801, 4484, 1491, 2780, 4926,  131, 1749, 7077, 1657, 4748,
	     224, 2314, 1505, 6490, 4870, 1223, 9118,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 5 */
	{ { 2210, 6805, 8708, 6537, 3884, 7962, 4288, 4962, 1643, 1027,  137,
	    7547, 2061,  633, 7731, 4163, 5253, 3525, 7420,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 4545,
	    6304, 4229, 2572, 2696, 9639,  630,  626, 6761, 3512, 9591, 6690,
	    4265, 1077, 2897, 7052, 9297, 7036, 4309,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 6 */
	{ { 5915, 7363, 8663, 1274, 8378,  914, 6128, 5336, 1659, 5799, 8881,
	     467, 1031, 4884, 5335,  241, 1478, 5948, 3000,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 4791,
	    6157, 8549, 7733, 7129, 4022,  157, 8626, 6629, 5674, 2437,  813,
	    3090, 1526, 4136, 9027, 6621, 6223, 2711,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 7 */
	{ { 6201, 1567, 2635, 4915, 7852, 5478,   89, 4059, 8126, 5599, 4473,
	    5182, 7517, 1411, 1170, 3882, 7734, 7033, 6451,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 8131,
	    3939, 3355, 1142,  657, 7366, 9633, 3902, 3550, 2644, 9114, 7251,
	    7760, 3809, 9435, 1813, 3885, 3492, 3881,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 8 */
	{ {  719, 4263, 8812, 9287, 1052, 5035, 6303, 4911, 1204, 5345, 1754,
	    1649, 9675, 6594, 5591, 5535, 4659, 7604, 8865,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 4732,
	    4902, 5613, 6248, 7507, 4751, 3381, 4626, 2043, 5131, 4247, 3350,
	     187, 9349, 3258, 2566, 1093, 2328, 4392,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 9 */
	{ {  363, 8932, 3221, 8711, 6270, 2703, 5538, 7030, 7675, 4644,  635,
	     606, 6910, 6333, 3475, 2179, 1877, 3507, 8687,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9675,
	    9445, 1940, 4624, 8972, 5163, 2711, 9537, 4839, 9654, 9763, 2611,
	    7206, 1457, 4841,  640, 2748,  696, 1806,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 10 */
	{ { 4858, 9556, 2732,  629, 3459,  771,  921, 8220, 4186, 2601, 5319,
	    2964, 4913, 4711, 7316, 8706, 5437, 6250, 8520,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 5540,
	    2201,  433, 8290, 1179, 9731, 1637, 9376, 4137, 9021, 3594, 4714,
	    7380, 2516, 7847,  597, 8429, 7675, 4091,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 11 */
	{ { 7650, 9241,  962, 2228, 1594, 3577, 6783, 9424, 1599, 2635, 8045,
	    1344, 4828, 5684, 4114, 1156, 7682, 5903, 9381,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9077,
	      79, 3130, 1773, 7395, 5472, 9573, 3901, 3315, 6687, 1029,  225,
	    8685, 9176, 1656, 8364, 9267, 7339, 8610,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 12 */
	{ { 2851, 4333,  651, 7766, 6338, 7487,  177, 1958, 8316, 2723, 1982,
	    7605, 6336, 2924, 6060, 9398, 9114, 9033, 4937,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 5222,
	    2627, 8022, 9215, 9718, 5821, 3748, 9761,  417, 4777, 2182, 5977,
	    8350,  158, 8008, 4190, 9310, 1482, 6167,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 13 */
	{ { 4629,  168, 5989, 6341, 7443, 1266, 1254, 4985, 6529, 4344, 6293,
	    3899, 5915, 6215, 8149, 6016, 5667, 9333, 1047,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 1029,
	    1598, 6939, 3680, 2190, 4891, 7700, 1863, 7734, 2594, 7503, 6411,
	    1286, 3129, 8966,  980, 9457, 6898, 6219,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 14 */
	{ { 3010, 2691, 3803, 9438, 6971, 8318, 7100, 9362, 6668, 1489, 8691,
	     716, 6429,  611,  929, 8560, 1218, 6305, 8436,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9080,
	    1328, 2760, 2101, 3884, 1414, 3177, 1951, 8316, 7452, 1064, 2537,
	    1952, 1022, 6875, 4927, 9410, 4895, 1990,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 15 */
	{ { 9512, 9233, 4182, 1978, 7278, 5606, 9663, 8472,  639, 3390, 5480,
	    9279, 2692, 3295, 7832, 6774, 9345, 1616, 1767,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 4559,
	    1683, 7874, 2533, 1353, 1371, 6394, 7339, 7591, 3800, 1677,   78,
	    9681, 1379, 4305, 7061,  529, 9533, 9374,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 16 */
	{ { 2499, 2373, 1558, 1595, 9709,  473, 6969,  454, 1934, 9239, 4859,
	    1845, 5757, 8160, 3956, 7035, 8502, 9634,  629,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 3033,
	    7717, 6914, 4261, 2175, 7417, 3752, 9523, 4110, 1007, 8607, 6738,
	    4090, 2173,  228, 6535, 9628,  337, 4888,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } }
};

/*
 * 1*(2^65)*G to 16*(2^65)*G.
 */
static const win_vpoint window5_G65[] = {
	/* 1 */
	{ { 9339, 7619, 1746, 5439, 2068, 8249, 8667,  772, 8629, 2580, 3535,
	    1497, 7659,  942,  418, 4236, 4544, 6106, 3463,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 6331,
	    2046, 4103, 1626, 1236, 7043, 1142,  248, 4893, 7448,  904,  833,
	    1839, 4591, 5095, 4412, 4501, 4022, 1789,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 2 */
	{ { 6468, 6074, 2548, 5799, 2184, 9236, 9087, 8194, 2125, 4482, 1596,
	    4633, 1219, 1728, 8587, 4914, 6813, 7586, 9632,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 7030,
	    6132, 8317, 4360, 4703, 8700, 3474, 3142, 9058, 6083, 8665,  920,
	    5688,  710, 8794, 9433, 8022, 2356, 5285,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 3 */
	{ { 7762, 9352, 6512, 1541, 5866,  565, 1801, 3246, 1697, 5080, 3383,
	    4351,  374, 6823, 4763, 4474, 2885, 9241, 1300,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 6689,
	    7021, 4440, 3976, 3443, 7873, 7187, 3414, 1165, 8823,  777, 9405,
	      54, 5902, 6112, 4515, 7303, 8691, 2848,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 4 */
	{ { 6413, 3230, 7046, 7939, 7788, 2866, 4807, 2771,  431, 3670, 3499,
	    5171, 8340, 8553, 4912, 8246, 3368, 4711, 1096,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 4169,
	    7327,  448,  462, 5552, 5998, 2096, 7221, 8644, 2493, 4292, 4406,
	    6619, 7277, 2268, 8841, 4384, 7040, 7167,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 5 */
	{ {  309, 6483, 4548, 8177, 5795, 4551, 4556,  144, 2807, 2589, 3923,
	    9514, 3600, 6017,  846, 4416,  153, 8827, 8412,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 8797,
	    8035, 5417, 3874, 1338, 4288,  182, 7124, 8907, 1659, 1250, 3072,
	    9097, 2109, 9494, 1700, 2434, 1994, 1508,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 6 */
	{ { 3151, 2283, 4770,  681, 7880, 8957, 8579, 1244, 2048, 8478, 8241,
	    1814, 6047, 1674, 4865, 5947, 5952,  145, 6842,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 5335,
	     836, 5609, 3870, 4658, 8615, 4289,  954, 9059,  424, 3606, 2347,
	    2043, 3209, 2892, 8883, 1621, 8693, 8133,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 7 */
	{ { 3762, 7052,  476, 7214, 9155, 2239, 5361, 7711, 6400, 4221, 1191,
	    9099, 3411,  666, 1921, 8456, 1735, 4799, 1040,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 8282,
	    1861, 1693, 3662,  607, 5331, 5140,   69, 8615, 2850, 7737, 3014,
	    6183, 2650, 8313, 2633, 6289, 2354, 4979,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 8 */
	{ { 1414, 3651, 7071, 1690, 3900, 3990, 4529, 3931, 5503,  230, 4456,
	    7933, 5146, 5898, 6986, 1935, 9562, 8268, 9114,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 5317,
	     902, 7554, 6278, 1949, 1335, 2515, 5858, 4744, 8796, 6320, 2156,
	     899, 8698, 3841, 2822, 2179,  981,  727,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 9 */
	{ {  616, 4305, 9664, 5097,  337, 6149,  456, 9304, 5937, 3094, 2675,
	    6390, 6158, 8813, 8889, 7170,  907, 7277, 8545,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 7114,
	    2148, 1405,  385,  538, 7332, 6699, 7360, 6858, 5429, 1681, 7826,
	    4579, 9011, 3757, 6458, 6541, 8509, 2623,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 10 */
	{ { 5926,  121, 7119, 7374,  251, 1633, 1892, 5302,  341, 9170,   75,
	    3504, 9467, 5834, 3535, 9204, 8908, 7736, 4065,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 5453,
	    8560, 8186, 3136, 7822, 5918, 7077, 4843, 6642, 6718, 5045,  998,
	    7350, 7692, 7138, 8151, 8775, 6932, 1976,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 11 */
	{ { 1179, 5385, 9349, 7084, 8953, 6412, 7070,  959, 4942, 7140, 6278,
	    3199, 6875, 3201, 4311, 9034, 5584, 8717, 6438,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 3950,
	     655,   36,  541,  239, 6383, 7171, 9102, 1539, 2235,  133, 8819,
	    3431, 6693, 1622,  276, 6530, 8448, 4882,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 12 */
	{ { 1558, 7758, 7818, 8409, 9269, 2755, 6142, 4519, 6267, 9326, 7100,
	    4103, 9020, 4535, 4792, 8992, 5884, 2007, 3857,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 6083,
	    8341, 4141, 2465, 3018, 6009, 3873, 2414, 2271, 3458, 1207, 2752,
	      28, 6585, 9250,  540, 3936, 9701, 3471,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 13 */
	{ {  391, 7074, 6309,  498, 1901, 5887, 2245, 4614, 8576, 8278, 7804,
	    9449, 6274, 7462,  664, 2589, 5479, 3265,  748,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 8931,
	    7265, 8153, 1165, 6939, 3673, 4729, 3534, 6736, 8435, 5875, 9668,
	    5157, 5502, 6340, 7991, 8571,   99,  323,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 14 */
	{ { 2262, 5112, 8876, 2456, 2849, 5909,  939, 4185, 2607, 7531, 4915,
	    7141, 8653, 3393,  448, 4864, 4057,  213, 3494,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 1811,
	    3681, 4133, 2175, 1800,  153, 4023, 7845, 5376, 4095, 4450, 6652,
	    9610, 2057, 7942,  256, 9220,  381, 1964,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 15 */
	{ { 6942, 9282, 1758, 7397,  445, 5707, 9060, 2013, 1672, 5709, 2326,
	    3026, 7067, 7730, 7046, 5092, 7301,  796, 2930,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 5490,
	    2773, 5548, 6087, 2649,  602, 8378, 3272, 8793, 6380, 1742, 4690,
	    3384, 1236, 4323, 7396, 4013, 1732, 1468,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 16 */
	{ { 6528, 2453, 9593, 2809, 1106, 2171, 7405, 2776, 5242, 6182, 9111,
	    8585, 8861, 1423, 8777, 4826, 6201, 7148, 9224,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 3719,
	     966, 2271, 9164, 3823, 5659, 5404, 5757, 2630, 8016, 5789, 7307,
	    6119, 3056, 8616, 7550, 9054, 6661, 6373,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } }
};

/*
 * 1*(2^130)*G to 16*(2^130)*G.
 */
static const win_vpoint window5_G130[] = {
	/* 1 */
	{ { 2165, 3108, 9435, 3694, 3344, 9054, 8767, 1948, 6635, 5896, 8631,
	    7602, 4752, 3842, 2097,  612, 5617,   82,  684,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 5040,
	    3982, 8914, 7635, 8796, 4838, 7872,  154, 8305, 9099, 5033, 2716,
	    1936, 8810, 1320, 3126, 9375, 3971, 4511,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 2 */
	{ { 4092, 7211, 1846, 2988, 2103, 3521, 3682,  242, 3157, 3344,  414,
	    1548, 7637,  706, 4324, 4079, 7797,  964, 5944,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 1978,
	    5559, 2543, 4324, 7281, 3230, 1148, 1748, 7880, 2613, 6362, 1623,
	     415, 1560, 4468, 2073, 9072, 3522,  875,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 3 */
	{ { 4479, 8055, 6264, 1857, 7675, 5705,  159, 8437, 5006, 5129,  143,
	    2392, 5997, 4610, 9417, 8463, 9025, 1516, 5506,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 7645,
	    7113, 2440, 9503, 2302, 5809, 6719, 2034, 8108, 8897, 6718, 5592,
	    2794, 3935, 5681, 5071,  324,  876, 6453,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 4 */
	{ { 7288, 4027, 5995, 7530, 2283, 2292, 1104, 6350, 2437, 3478, 6591,
	    7516, 6539, 5633,  944, 7664, 7307, 2558, 7272,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 1349,
	    1713, 2897, 9658, 4564,  651, 8999, 7760, 2000, 7646, 2720, 5574,
	    2690,  784, 9128, 5008, 7401, 8790, 4475,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 5 */
	{ { 3046, 1317, 9247, 1425, 8108, 1364, 4113, 5823, 6162, 6453, 6919,
	    2960, 7107, 2247, 9488, 8666, 2607,  530, 3368,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 7677,
	    8009,  215, 6046, 8686, 9245, 9579,    2, 8237, 7464, 9190, 8469,
	    8694, 6393, 8794, 9742,  209, 4289, 8286,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 6 */
	{ { 9171, 1888, 5274, 7646, 2205, 5714,  977, 6004, 2075,  500, 2635,
	     719, 4947, 5698, 5761, 5917, 5153, 5856,  726,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 7655,
	    3232, 9694, 2040,  704, 6539, 9099, 2049, 4154, 2880, 8674, 7911,
	    6430, 8187, 4337, 7307, 7578, 4850, 2702,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 7 */
	{ { 6387, 7081, 2754, 9751, 8354, 8734, 8712, 7368, 1982, 9316, 3821,
	    9179, 2704, 6077, 8278, 6302,  922, 8976, 9269,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  256,
	    1984, 2620, 7363, 1470, 9710, 1926, 3809, 4371,   54, 3082, 1742,
	    4090, 6225, 3283, 4073, 7017, 1723, 1915,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 8 */
	{ { 8195,  240, 4145, 3135, 3635, 1513, 2881, 1852, 1470,  355, 8478,
	    5583, 7858, 2443, 7998,  808, 3647, 6849, 8791,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 7083,
	    7168, 6937, 9097, 1473, 4037, 5648, 8771, 6995,  231, 3910, 3242,
	    4686, 8908, 7134, 4178,  747, 2902,  677,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 9 */
	{ { 6701, 7369, 9134, 1810, 9505, 5322, 4260, 8965, 2871,  564, 1193,
	    8020,  136, 5692, 7057, 7379, 5982, 1270, 8730,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9373,
	    7791, 2102, 8255, 5784, 6527, 7944, 3518,  808, 1197, 4223, 5799,
	    2705, 7017, 2030, 4719, 6711, 6004, 2991,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 10 */
	{ { 6750,  632, 9285, 5581, 8420, 4348, 8129, 6686, 2839, 4810, 2115,
	    7664, 3941, 5152, 2367, 3575,  908, 5039, 2548,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 2791,
	    5834, 5148, 5395, 7972, 3680, 2834,  517, 5389, 7190, 5989, 3922,
	    5292, 2623, 5891, 2290, 8912, 4179, 3808,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 11 */
	{ { 8631,  348, 5289, 7807, 2775, 7626, 3296, 7883, 1049, 3306, 6194,
	     317, 3071, 7432, 1784, 2957, 5105, 7896, 9055,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 6245,
	    3438, 5630, 6664, 7600, 6321, 8728, 8664, 5459, 3638, 2894, 1172,
	     706, 1884, 7294, 7484, 1979, 2569, 7702,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 12 */
	{ { 9215,  874, 8230, 7066, 2560, 1684, 1660, 4855, 1508, 3140,  200,
	     344, 2385, 6660,  412,  721, 1687, 7109,  319,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 3527,
	    7302, 5126, 2781, 8724,  373, 8949,   46, 3153,   47, 7352,  683,
	    7987, 9188, 9365, 5334, 1595, 3456, 8165,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 13 */
	{ { 8425, 4005, 2859, 5381, 7341, 5095, 3394, 1535, 4156, 7566, 5388,
	    1434, 3475, 1348, 6300, 6556, 4410, 8382, 5928,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 5991,
	    3954, 2119, 4914, 1589, 7303, 1116, 4102, 9509, 2977, 9217, 7003,
	    6690, 6195, 4766, 8189,  347, 3022, 8805,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 14 */
	{ { 3418, 9126, 5210, 7578, 1037, 1407, 7527, 2014, 8869, 5903, 7822,
	    5005, 8878, 7666, 9580, 6829, 3533, 1764,  531,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 3884,
	    4264, 1781, 6163, 8082, 7243, 7896, 5448, 3151, 1015, 6206, 8220,
	    2198, 2269, 1164, 7889,  788, 1956, 4434,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 15 */
	{ { 3783, 1823, 6422, 3611, 4601, 2939, 5093, 8772,  517, 9761, 7427,
	    9424, 5326, 3567, 5282, 7144,  894, 8119, 4186,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   22,
	    6492, 6253, 8915, 9126,  368, 4759, 1234, 5228,  957, 4614, 1065,
	    6642, 6253, 4935, 8897, 9020, 8091, 4495,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 16 */
	{ { 8756, 8412, 3079, 6209, 5447, 9361, 7853, 6692, 1869, 5362, 9440,
	     412, 5108, 3838, 7398, 6825, 3716, 7999, 6317,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 4579,
	    8025, 2522,  831, 6754, 2703, 3053, 5911, 9182, 8777, 5748, 4462,
	    5390, 3242, 3445, 4885, 7436, 6034, 1243,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } }
};

/*
 * 1*(2^195)*G to 16*(2^195)*G.
 */
static const win_vpoint window5_G195[] = {
	/* 1 */
	{ { 2080, 1606, 7613, 1753,  773, 2343, 4365, 9079, 3522, 5307, 7475,
	    9273, 5018, 9428, 7506, 9204, 6734, 1373, 2019,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 7141,
	    2475, 1648, 7094, 7167, 7295, 5689, 5854,  658, 1878, 2724,  132,
	    5521, 9275, 7880, 7598, 5428, 6665, 5452,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 2 */
	{ {  784,  278, 4249, 3118, 8614, 7940, 3583, 1444, 6814, 1000,  398,
	    8345, 2487, 3832, 5563, 2150, 2163, 7863, 9516,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  150,
	    9544, 6859, 6985, 9291, 2035, 2373, 6207, 1578, 6191, 4036, 4298,
	    4652, 8208, 5377, 1142, 1846, 9509, 8094,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 3 */
	{ { 5948, 4344,  215,  389, 3550, 2653, 2234,   48, 9236, 3073, 5466,
	    8118,  820, 5764, 9465, 4030, 1750,  740, 6550,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  295,
	     252,  588, 7149, 9208, 9020, 4432, 7864, 3139, 5112, 6344, 6205,
	     230, 4962, 5821, 8633, 9162, 7103, 8860,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 4 */
	{ { 5413, 3213, 6231, 5944,  506, 5301, 2938, 5358, 7519, 2582,  721,
	    2512, 6030, 6200,  387, 6508, 3370, 5088, 2227,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 6503,
	    7607, 2394, 8296, 4397, 5309, 9767, 1664, 5366,  465, 8558, 5952,
	    2301, 4021, 5700, 1255, 1070, 4944, 6393,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 5 */
	{ {  273, 6221, 4210, 7065, 2371, 7508, 5600, 5596, 2308, 3557, 4975,
	    4244, 8950, 8319, 4261,  801, 6134, 1883, 5408,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 4349,
	    5349,  423, 9517, 9181, 8078, 6629, 2331, 4892, 3851,   14, 4372,
	    8695, 8332, 3516, 1403, 6734, 7986,  484,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 6 */
	{ { 8825, 4180, 1112, 9189,  896, 1312, 2412, 9252, 4555, 5247, 5221,
	    7619, 9132, 4169, 2302, 4132,  564, 2700, 8388,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 6134,
	    6307, 2711, 4537, 6092, 9311, 6840, 1749, 7493,  232, 8671, 8798,
	     671, 2724, 3398, 4144, 1545, 6367, 2868,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 7 */
	{ { 3501, 3618, 1344, 8048, 8846, 7600, 7931,  942, 4797, 3949, 3135,
	    4967, 4257,  355, 7241, 1042,  305, 2101, 5385,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 4997,
	    7629, 4418, 5721, 1160,  365, 9694, 5644, 1276, 7675,  828, 6444,
	    5805, 2323, 1761, 3145, 5494, 8716, 7862,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 8 */
	{ { 6937, 4865, 1653,   44,  685, 8036, 2222, 6142, 7691, 3260, 5347,
	    5315, 5609, 2905, 4521, 6626,  107, 1300, 5552,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 7928,
	    1431, 7861, 5635, 6790, 5131, 9131, 5555, 4595, 8935, 9236, 8305,
	    3458, 9711,  359, 2146,  337, 2673, 4597,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 9 */
	{ { 2232, 4547, 9763, 1357, 6266, 5197, 5176, 5118, 2871, 8569, 5241,
	    5680, 4031, 6859, 9216, 4161, 8898, 1936, 5769,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9025,
	    1557, 5660, 4139, 5316, 4230,   51, 2646, 7463, 5573, 9519,  471,
	      79, 4930,  998, 5354, 3335,  946, 2886,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 10 */
	{ { 5848, 4096,  609, 2712, 2208, 9087, 3787, 8838, 1529, 4885, 5971,
	    9699, 2733, 1548,  524, 4435, 4454, 7730, 4626,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9301,
	    4650, 6531, 8181, 3117, 7526, 6743, 7420, 3408, 1261,  269, 7882,
	     980, 7262, 2700, 2260,  968, 1893, 5373,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 11 */
	{ { 4777, 8162, 3499, 8798, 1897, 5351, 1068, 4614, 5391, 3709, 8252,
	     519, 8078, 4318, 1913, 4794, 1578, 6747,   63,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 3209,
	     212, 8846,  912,  164, 5475, 7803, 4886, 7155, 2997,  362, 1915,
	    7760, 8280, 4349, 2024, 1404, 2990, 5030,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 12 */
	{ { 6410, 6143,  677,  730, 5644, 8412, 5539,  237, 9499, 5455, 7706,
	    3899, 6544, 7219, 3968, 4384, 5708, 5812, 2789,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 8188,
	    4541, 7853, 7561, 5700, 9625, 3749, 1143,  524, 8994, 3957, 8774,
	    6658, 8625, 4004, 8598, 6844, 6227, 1462,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 13 */
	{ { 7858, 7300, 5237,  727, 5857, 6188, 6762,  958, 4873, 1919, 6369,
	    2168, 8026, 3985, 8963, 4725,  182, 7009, 4604,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 2445,
	    3114, 2387, 4852, 1190, 5301, 2221, 2448, 3675, 6592, 7548, 7863,
	    5651, 8272, 4563, 2431, 2065, 7245, 6965,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 14 */
	{ { 1976, 9485,    1, 8756, 6117, 8931, 5438, 6024, 7217, 3376, 2035,
	    5805, 9123, 9380, 7766, 3511, 1709, 9536, 8959,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 4358,
	    1719, 6075, 7688, 5834, 3571, 1266, 3821, 5613, 4989, 6829, 9270,
	    9504,   76, 7138, 2842, 8805, 6688, 5308,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 15 */
	{ { 3514, 5548, 3917, 8682, 9168, 5716, 5595, 4959,  619, 4355, 3957,
	    3146, 6028, 1156, 5929, 2675, 7044, 4084, 9199,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 5808,
	    4062, 2839, 9659, 8428, 6138, 9341, 7041, 5347, 7073, 5899, 6681,
	    2295, 3310, 3423, 2688, 3111, 1902, 3428,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 16 */
	{ { 9005, 4408, 5332, 8756, 8395,  683, 1171, 1243, 3471, 1243, 3200,
	    8389, 5189, 6867, 6883,  843, 4890, 5399,  695,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 7740,
	    3594, 6910, 1635, 9049, 7458,  694, 5622, 5017, 6471, 4126, 3675,
	    1142, 7288, 1503, 1449,   10, 2500, 3486,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } }
};

/*
 * 1*G to 63*G (odd multiples only).
 */
static const win_vpoint window_odd7_G[] = {
	/* 1 */
	{ { 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,
	    9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9767,
	    9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,
	    9767, 9767, 5183, 9767, 9767, 9767, 9767,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 3 */
	{ { 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,  827,  976, 9767,
	    9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9767,
	    9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767, 9767,
	    3199, 3986, 5782, 9767, 9767, 9767, 9767,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 5 */
	{ { 2210, 6805, 8708, 6537, 3884, 7962, 4288, 4962, 1643, 1027,  137,
	    7547, 2061,  633, 7731, 4163, 5253, 3525, 7420,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 4545,
	    6304, 4229, 2572, 2696, 9639,  630,  626, 6761, 3512, 9591, 6690,
	    4265, 1077, 2897, 7052, 9297, 7036, 4309,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 7 */
	{ { 6201, 1567, 2635, 4915, 7852, 5478,   89, 4059, 8126, 5599, 4473,
	    5182, 7517, 1411, 1170, 3882, 7734, 7033, 6451,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 8131,
	    3939, 3355, 1142,  657, 7366, 9633, 3902, 3550, 2644, 9114, 7251,
	    7760, 3809, 9435, 1813, 3885, 3492, 3881,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 9 */
	{ {  363, 8932, 3221, 8711, 6270, 2703, 5538, 7030, 7675, 4644,  635,
	     606, 6910, 6333, 3475, 2179, 1877, 3507, 8687,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9675,
	    9445, 1940, 4624, 8972, 5163, 2711, 9537, 4839, 9654, 9763, 2611,
	    7206, 1457, 4841,  640, 2748,  696, 1806,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 11 */
	{ { 7650, 9241,  962, 2228, 1594, 3577, 6783, 9424, 1599, 2635, 8045,
	    1344, 4828, 5684, 4114, 1156, 7682, 5903, 9381,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9077,
	      79, 3130, 1773, 7395, 5472, 9573, 3901, 3315, 6687, 1029,  225,
	    8685, 9176, 1656, 8364, 9267, 7339, 8610,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 13 */
	{ { 4629,  168, 5989, 6341, 7443, 1266, 1254, 4985, 6529, 4344, 6293,
	    3899, 5915, 6215, 8149, 6016, 5667, 9333, 1047,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 1029,
	    1598, 6939, 3680, 2190, 4891, 7700, 1863, 7734, 2594, 7503, 6411,
	    1286, 3129, 8966,  980, 9457, 6898, 6219,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 15 */
	{ { 9512, 9233, 4182, 1978, 7278, 5606, 9663, 8472,  639, 3390, 5480,
	    9279, 2692, 3295, 7832, 6774, 9345, 1616, 1767,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 4559,
	    1683, 7874, 2533, 1353, 1371, 6394, 7339, 7591, 3800, 1677,   78,
	    9681, 1379, 4305, 7061,  529, 9533, 9374,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 17 */
	{ {  282,  953,  123, 5681, 1494, 3168,  695,  398, 3375, 1798,   41,
	    7178, 2960, 3121, 9112, 9067, 5196, 5361, 2482,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9110,
	    2713, 9205, 8996, 2729,  539, 5881, 1581, 4334,  770, 5366, 9544,
	    5347, 3285, 2702, 3169, 6071, 8029, 5453,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 19 */
	{ { 8911, 2678, 1439, 4541, 8516, 8233, 5576, 4548,  302, 9414, 5295,
	    4055, 4362, 9376, 3646, 8502, 7187, 5688, 1348,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 8820,
	    5155, 4208, 9202, 5509, 5202,  273, 2698,  211, 1557, 1273, 5448,
	    3199, 4479, 6386,  333, 8011, 1904, 1986,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 21 */
	{ { 7378, 3849, 1731, 9035, 4122, 2508, 8560, 9188, 7622, 6708, 7890,
	    3134, 5238, 2612,  264, 1432, 7516, 7382, 8932,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 6954,
	     504, 7410, 7322, 2380, 3501, 5987, 5018, 4217, 8140, 5838, 4541,
	    3340, 5554, 8110, 1896, 5152, 6797, 6051,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 23 */
	{ { 5908, 8243, 8334, 3447, 7371,  920, 5463, 2387, 8204, 6970, 2696,
	    7346, 4475, 5073, 5105, 3328, 4896, 1183, 3109,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  717,
	    1399, 8513, 5564, 9184,  553, 4819,  547, 4226, 2517, 4719, 9048,
	      26, 8630, 2652, 6913, 8889, 4808, 1067,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 25 */
	{ { 5146, 4974, 6290, 5541, 6777, 8469, 6726, 9208, 5752, 5452, 1153,
	     439, 4369, 8533, 7682, 9528, 5366, 4140, 4845,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 2637,
	      81, 6091, 2056, 3801, 4906, 3939,  666, 1098, 7442, 2627, 4483,
	    1723, 8315, 2310, 7378, 5038, 5643, 9072,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 27 */
	{ {  665, 7747,  909, 8981, 9455, 7942, 9500, 2645,  817, 7005,   72,
	    6459, 1631,  958, 4305, 3641, 7714, 7566, 8976,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 2525,
	    6227, 8492, 7633, 2229,  309, 9522,  307, 6189, 8812, 5103, 6133,
	    7098, 7010,  672, 2940, 8639,  650,  213,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 29 */
	{ { 5995, 4959, 3323, 2779, 4074, 4584, 4113, 7218, 8484, 5069, 3162,
	    1633, 8674, 1789, 7012, 7004, 5305, 2607, 1680,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 4903,
	    6437, 3552, 8578, 1895, 2150, 7909, 5107, 6543, 8374,  177, 7917,
	    2441, 1577, 1553, 9745, 2896, 5939, 5246,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 31 */
	{ { 1010, 4720, 1236, 5418,  235, 6252, 9307, 4431, 5193, 5345, 2594,
	    1359, 6277, 9466, 3274, 7068, 6194, 8046, 6040,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  985,
	    4326, 6455, 1481, 1570, 6741, 7767, 6646, 4431, 4691, 2350, 6284,
	    9277, 1469, 3891, 8410, 8800, 4717, 4235,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 33 */
	{ { 5748, 3081, 5432, 4504, 9295, 8446, 1943, 1350, 1746, 5566, 5828,
	    8727, 1022, 3985, 5876, 5902, 8999, 9398, 5305,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 5741,
	    7034, 5767, 4599, 6303, 1083, 5721, 2101, 2052, 1946,  369,  110,
	    4112, 3214, 7330, 4382,  878, 6632,   41,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 35 */
	{ { 3346, 4718, 9432, 2214, 5287, 9312, 2245, 6428, 1690,  552,  507,
	    1551,  374, 8099, 1846, 2512, 9191,   38, 4761,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 6868,
	    5708, 3506, 2037,  486, 2439, 4072, 4375, 4208, 4849, 3145, 7904,
	    3490, 2144, 8817, 6898, 4752,  529, 3800,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 37 */
	{ { 4501, 1086,   97, 5050, 2295, 2239, 2325, 5002, 8093, 9323, 7372,
	    9679, 5604, 7743, 2812, 9149, 2247, 7904, 2375,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9071,
	    6951, 1016, 1883, 9224, 2953, 8259, 2252,  276, 6391, 5076, 2763,
	    3222, 3276, 2828, 8389, 6798, 7271, 7673,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 39 */
	{ { 3437, 8065, 3454, 3266, 2088, 7434, 4764, 7294, 1621, 3245, 5143,
	     154, 3465, 7335, 3741, 8204, 2823, 4386, 1533,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 7186,
	     756, 7389, 6339, 4662, 2757, 5800, 9087, 1825, 8873, 2426, 2080,
	    8192, 3927, 6532, 6045, 4484, 8319, 3437,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 41 */
	{ { 8056, 2208, 1753, 2270, 5395,  770, 2403, 7501, 8960, 8695, 2683,
	    6043, 7245, 9744, 1572,  181, 7509, 6929, 8841,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 1963,
	    4961, 7455, 4654, 5593, 5351, 6163,  212,  833, 4786, 5676, 3757,
	    5383,  297, 4054, 8621, 3726, 2678, 8546,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 43 */
	{ { 1556, 3426, 5712, 1621, 6340, 1718, 9168, 5316, 3027, 8111, 2620,
	    8137, 5997, 5827, 4635, 8877, 6083, 6316, 8511,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 1572,
	    5777, 3157,  720, 7041, 5490, 2246, 9229, 9425,  228, 7581, 4302,
	    3952, 9630,  177, 5956, 4669, 6163, 5371,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 45 */
	{ { 8855, 4775, 3099, 7170, 8556, 4421, 7553, 3398, 6882, 6779, 3860,
	    1677, 1687,  996, 8738, 6541, 9259, 2449,  364,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 6391,
	    6094, 2786, 2581, 2967, 3511, 4018, 6052, 3620, 7482, 1064, 8165,
	    3628,  268, 4104, 6874, 2289, 3041, 3321,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 47 */
	{ { 8389, 6352, 7836, 7474, 3741, 7898, 9427, 3511, 6654, 3032, 1137,
	    1557, 3590, 1349, 6128,  462,   19, 1124, 4335,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 5866,
	    5880, 9172, 3005,  103, 6340, 4697, 2060, 3073, 8757,  387, 9052,
	    7332, 4228, 7461, 3989, 7805,   65,  645,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 49 */
	{ { 6895,  118,  920, 2014, 2754, 8296, 3169, 9721, 5577, 2556, 9349,
	    7646, 2412, 1506, 6378, 3148, 5597, 9138, 4791,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 3499,
	     541, 4773, 4864, 8105, 6145, 6780, 8140, 9124, 6864, 1492, 4973,
	    4868, 9660, 3883,  754, 5947, 5020, 7153,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 51 */
	{ { 3367, 8610, 1410,  324, 4943, 2444, 2850, 3776, 1076, 7612, 8301,
	     201, 6390, 1196, 9007, 9481, 5382, 2824, 7447,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 2457,
	    4011, 3666, 2513, 3361, 6972, 8023, 9032, 3458, 9074, 8175, 1248,
	    2680,  546,  442,  392, 4969, 7817, 4857,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 53 */
	{ { 8965, 3045, 1341, 8998, 5041, 6893, 1724,  182, 1289, 5257, 4445,
	    1489, 4606, 8532, 6766, 4024, 5681, 7397, 8720,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 5877,
	    1547, 3140, 8349, 6326, 4446, 5918, 1222, 3447, 7741, 3720, 8890,
	    5360, 2525, 7625, 4924, 1761, 2089, 8605,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 55 */
	{ { 4792, 6404, 4677, 6186, 3218, 3163, 5360, 5373, 6496, 1872, 5974,
	    6072, 5010, 3060,   34,  217, 9173,  350, 8228,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 3265,
	      60, 3955, 3258, 2328, 1685, 6165, 5844, 4311, 1555, 3603, 7350,
	    6780, 9053, 4140, 3787, 3804, 5632, 1554,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 57 */
	{ {  510, 5502, 7888, 5538, 7718, 1254,  674, 4925, 5153, 3115, 4608,
	    3350, 7235, 7889, 8000, 8953, 9342, 6304, 7573,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 7488,
	    6616, 9500, 3605, 1578, 1515, 8627, 9649, 7926,  458, 4288, 1789,
	    9667, 4176, 6176,  835, 9086, 8084, 6591,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 59 */
	{ { 5289, 6559, 3006, 3634, 4561, 3924,  764, 1400, 5350, 3954,  316,
	    6196, 7538, 2382, 3945, 8189, 3873, 8505, 6394,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 7029,
	    8100, 4333, 2174, 6942,  932, 7496, 4471, 9718, 7556, 1936,  861,
	     338, 6280, 7396, 2315, 4995, 8863, 1665,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 61 */
	{ { 7645, 9524, 4159, 2060, 1771, 3372, 4450, 6502, 7690, 9593, 7419,
	    1705, 7999, 4390, 1190,  225, 1045, 7944,  214,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 8943,
	    1644, 4651, 3391, 9707, 1108, 4385, 7010, 4725, 3895, 4651, 7187,
	    4331, 7877, 5083, 9122, 3565, 6097, 5876,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 63 */
	{ {  750, 3085, 8148, 7754, 9561, 1218, 8448, 7870, 1720, 5957, 1481,
	    9195, 9535, 3674,  384, 2345, 8886, 8309, 1496,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 7662,
	    5369, 5922, 9323,  583, 3460, 2692, 2832,  298, 1613,  963,  587,
	    8774, 4849,   38,  567, 8443, 4654, 3179,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
};

/*
 * 1*(2^128)*G to 63*(2^128)*G (odd multiples only).
 */
static const win_vpoint window_odd7_G128[] = {
	/* 1 */
	{ {  380,  263, 4759, 4097,  181,  189, 5006, 4610, 9254, 6379, 6272,
	    5845, 9415, 3047, 1596, 8881, 7183, 5423, 2235,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 6163,
	    9431, 4357, 9676, 4711, 5555, 3662, 5607, 2967, 7973, 4860, 4592,
	    6575, 7155, 1170, 4774, 1910, 5227, 2442,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 3 */
	{ { 6654, 5694, 4667, 1476, 4683, 5380, 6665, 3646, 4183, 6378, 1804,
	    3321, 6321, 2515, 3203,  463, 9604, 7417, 4611,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 3296,
	    9223, 7168, 8235, 3896, 2560, 2981, 7937, 4510, 5427,  108, 2987,
	    6512, 2105, 5825, 2720, 2364, 1742, 7087,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 5 */
	{ { 3733, 2716, 7734,  246,  636, 4902, 6509, 5967, 3480,  387,   31,
	    7474, 6791, 8214,  733, 9550,   13,  941,  897,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 7173,
	    4712, 7342, 8073, 5986, 3164, 7062, 8929, 5495, 1703,   98, 4721,
	    4988, 5517,  609, 8663, 5716, 4256, 2286,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 7 */
	{ { 1209, 1752, 8277, 2095, 2861, 3406, 9001, 7385, 1214, 8626, 1568,
	    8438, 9444, 2164,  109, 5503,  880, 5453, 5670,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  145,
	    1939, 1647, 4249,  400, 8246, 8978, 6814, 6635, 8142, 3097, 3837,
	    4908, 8642,  423, 2757, 6341, 2466, 2473,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 9 */
	{ { 6631, 7588, 1952, 4374, 8217, 8672, 5188, 1936, 7566,  375, 6815,
	    7315, 3722, 4584, 8873, 6057,  489, 5733, 1093,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 1229,
	    7837,  739, 5943, 3608, 5875, 6885,  726, 4885, 3608, 1216, 4182,
	     357, 2637, 7653, 1176, 4836, 9068, 5765,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 11 */
	{ { 4654, 3775, 6645, 6370, 5153, 5726, 8294, 5693, 1114, 5363, 8356,
	    1933, 2539, 2708, 9116, 8695,  169, 3959, 7314,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9451,
	    7628, 8982, 5735, 4808, 8199, 4164, 1030, 8346, 8643, 5476, 9020,
	    2621, 5566, 7917, 6041, 3438, 8972, 2822,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 13 */
	{ {  943,  239, 2994, 7226, 4656, 2110, 5835, 1272, 5042, 2600,  990,
	    5338, 3774, 7370,  234, 4208, 7439, 3914, 2208,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9466,
	    5076, 2796, 9013, 8794, 7555, 5417, 7292, 9051, 9048, 1895, 6041,
	     802, 6809, 7064, 5828, 7251, 3444, 6933,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 15 */
	{ { 1304, 2731, 6661, 9618, 7689,  121,  991, 1683, 5627, 3143, 2891,
	    4724, 5853, 3174, 8571, 7021, 2925, 5461,  409,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 8072,
	    5485, 6915, 5742, 5583, 1904, 8913,  678, 9327, 6739, 7675, 1134,
	    7284, 8485, 7235, 1210, 2261, 6781,  360,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 17 */
	{ { 3347, 2580, 6479, 9075, 9441, 2031, 7911, 8534, 3353, 4906, 2936,
	    5955, 1093, 2007, 8134, 8235, 1234, 2276, 7165,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 4702,
	    2756,  535, 7982, 8144, 6586, 3177, 1752, 9606, 7312, 6122, 5210,
	    2943, 4464, 5368, 7847, 2861, 3541, 5323,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 19 */
	{ { 6350, 8856, 8794, 5814, 6244, 3023, 5949, 1871, 2823, 9392, 9694,
	    3536, 3927, 8808, 5485, 6391, 6871, 6583, 9296,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 6527,
	    3706, 5298, 2958, 8002, 9655, 8015, 8592, 2877, 2847, 8294, 7280,
	    7339, 6180, 1228, 4391, 2276, 9325, 4185,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 21 */
	{ { 7251, 2362, 7196, 6531, 6151, 9574, 7288, 3315, 8460, 2348, 7292,
	    6768, 8508, 9300, 1431, 7181, 3135, 9321, 2624,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 8019,
	     708, 3056, 4310, 6765, 4782, 4718, 8861, 2700, 8241, 3261, 8495,
	    7643, 3693, 1537, 9367, 1795, 6314, 2565,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 23 */
	{ { 8404, 4855, 2270, 1873, 5256,  311, 2815,  386, 5684, 1605, 2497,
	    9012, 4006, 5414, 2834, 7471, 2015, 3403, 2816,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 3409,
	    6989,  262, 9572, 7583, 5924, 5601, 6152, 6258,  719, 3288, 7801,
	    4586, 6236, 5511,  679, 3107, 6575, 6712,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 25 */
	{ { 2327, 7419, 9466, 5011, 1970, 7252, 1999, 2739,  976, 5877, 5577,
	    2685, 3605, 9017,  999, 4849, 8684, 5236, 4809,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9705,
	    5587, 7882, 3797, 3373, 1482, 9007,  472, 1889, 1991, 7671, 1676,
	    4671, 1317, 9266, 6116, 4106, 8298, 3398,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 27 */
	{ { 8788, 6337, 4544, 2365, 1940, 1032,   94, 8596, 7284, 6614,  167,
	     276, 7062, 6000,  365, 2903, 6433, 5155, 9514,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 5829,
	    7591, 1493, 8804, 3263, 7478, 3480,  525, 3028, 4046, 1771, 7087,
	    6335, 1346, 4805, 2004, 7346, 4547, 8371,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 29 */
	{ { 5517, 3011, 5552, 1727,  653, 2389,  296, 4961, 2974, 3198, 5690,
	    7247, 2587, 2899, 4016, 7514, 9421, 3066, 8188,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 5433,
	    1682, 6387, 4262, 2436, 8793, 3017,  216, 8117, 7179, 4226, 4805,
	    9507, 7199, 6636, 3156, 1770, 6438, 6650,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 31 */
	{ { 4234, 5656, 2237, 9642, 3884,  378, 7064, 3871,  424, 1004,  561,
	    3019, 1419,  190, 2164, 3438, 7990, 3950, 7262,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 3469,
	    3073, 8186, 8387,  101, 2245, 4400, 3333, 1908, 2998, 9627,   39,
	    5295, 7934, 2946, 1172, 2092, 1988, 8483,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 33 */
	{ {  771, 6508, 5844, 4530, 2605, 8281,  155, 1095, 8208, 3098, 2135,
	    8912, 9042, 4431, 9354, 3697, 4147, 3468, 4439,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 2118,
	    1472, 9521, 2390, 7379, 6581, 8880, 5220,  779, 5226, 1091, 8542,
	    2666, 3880, 1927, 2506, 3926, 4653, 5048,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 35 */
	{ { 2100, 7789, 6967,  449, 2465, 5705, 4921, 7645, 2761, 7948,  982,
	      86, 6199,  883, 5084, 4603, 5419, 3869, 2608,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 6961,
	    3503, 7781, 8210, 5083, 3119, 2474, 3366, 7839, 9373, 9285, 6055,
	    8189, 3258, 8126, 5279, 2852, 6654, 5975,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 37 */
	{ { 5011, 2440, 2859, 2754, 7677, 6956, 8394,   83,  778, 3070, 2078,
	    3933, 5804, 8720, 7016, 5891, 2672, 6734, 3284,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 6693,
	    8905, 1234, 4839, 2871, 4582, 8497, 9724, 5872,  808, 8906, 3794,
	    4328, 3988, 6862, 5217, 1309, 3986, 7770,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 39 */
	{ { 6902, 3999,  799, 7044, 4158, 5360, 1858, 7625, 3594, 4775, 5745,
	    6062, 6614, 5789, 7205, 6737, 5781, 6442, 3409,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9694,
	    4621, 7609, 8297, 5900, 5104, 7191, 1354,  494,   41, 4202, 3174,
	    6245, 5872, 4309, 5121, 2103, 7186, 1305,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 41 */
	{ {  137, 2413, 2097, 3858, 7308, 3249, 9171, 6322, 1738, 4904,  203,
	    4148, 1373, 2201,  332, 3426,   56,  433, 1563,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 8245,
	    6330,  974, 3368, 2791, 4034, 1461, 5746, 8358,   64, 8772, 6977,
	    4560,  677, 9423, 7725, 8318, 3250, 8915,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 43 */
	{ { 7406, 7680, 9567, 1272, 3012, 7976, 4323, 6782, 2551,  635, 7069,
	    2145, 8180, 7907, 1867, 5010, 2124,  868, 5291,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 1280,
	    9445, 3758, 7433, 7707, 3297,  244, 4761, 5962, 4996, 5992, 7445,
	    7238, 6568, 5371, 3924, 8775, 1889, 4040,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 45 */
	{ { 1890,  658, 6090, 9700, 6932, 9193, 6423, 4561, 5504, 7114, 2863,
	    3692, 2050, 8051, 3501, 8460, 5405, 3306, 9395,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  506,
	     385, 3291, 6057, 4290, 2573, 4426, 8545, 2850, 9115, 6949, 3920,
	    5630, 8324, 7871, 4946,  678, 6271, 1753,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 47 */
	{ { 9674, 4475, 8448, 7824, 1855, 1477, 5325, 2070, 9049, 3859, 2746,
	    7893, 1323, 6444, 1183, 5998,  722, 6007, 6239,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 2439,
	    6632, 3858, 5907, 2913, 3066, 9115, 5111, 8795, 7649, 6467, 3533,
	     659, 2757, 9714, 1217, 2359, 3686, 8515,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 49 */
	{ { 1752, 5251, 3476, 2935, 5882, 7169, 2421, 4583,  517, 8112, 7274,
	    3964, 7557, 2474, 3460, 2283,   11, 6088,  207,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 3943,
	    6438, 2464,  274, 3299, 3945,  537, 1801,  509, 9385, 5102, 1734,
	    2837, 7230, 5279, 6627, 3275, 5170, 5730,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 51 */
	{ { 2728, 2184, 9385, 7530, 8182, 5751, 1513, 5472, 8132, 3306, 5119,
	    1339, 1740, 4959, 8620, 1993,  797, 5104, 6842,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 1837,
	    2752, 6635, 2785, 1132, 5036,  324, 4291, 1764, 6783, 2755, 9248,
	      19, 4235, 4935, 5889, 3263, 9437, 6584,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 53 */
	{ { 8778, 2693, 3366, 1774, 5629, 1124, 7632, 5025, 5829, 9274, 3745,
	    8735, 9573, 3426, 2299,  496, 9344, 5516, 7415,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 6667,
	    8268, 5468, 5177, 2875, 2186, 2329, 8252,  511, 6419, 4440,  543,
	    9474,  209, 9300, 6751, 7626, 7073, 1065,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 55 */
	{ { 9500, 1655, 9348, 1551, 9600, 3297, 6571, 6222,  398, 8178, 3216,
	    5474, 3845, 6925, 4952, 2282, 7512, 8484, 4745,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 6748,
	    1097, 9702, 4977, 4523, 7327, 1967, 5130, 4922, 1820, 8285, 4684,
	    9564, 2802, 4594, 4025, 9167,  356, 6900,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 57 */
	{ { 6096,  249, 4955, 6964, 7580, 6074, 9053, 1478, 8539, 4405, 7856,
	    4313, 7964, 5643, 8730, 6753, 7023, 5828, 6215,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   85,
	    9073, 8044, 9730, 1682, 6381, 5918, 1234, 3871, 9402,  992, 5724,
	    7587, 7993, 6092, 2847, 8026,  580, 6084,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 59 */
	{ { 5300, 3875, 6086, 2317, 4582, 9548, 4699, 4962, 6134, 7250, 2946,
	    4383, 6913, 1725, 4845,  513, 8450, 9368, 4561,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 3502,
	    2934, 5025, 5735, 9040, 5500,  554, 3772, 7945, 4960, 3804, 3659,
	    5001, 2177, 9291, 3136, 3314, 4907, 4333,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 61 */
	{ { 3450, 8574, 9225, 4688, 8687, 9056,  902, 7777, 8865, 8483, 5601,
	    4448, 2864,  950, 2764, 8757, 7806, 8202, 3451,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 9080,
	    7731, 2949, 9115, 1660, 6441, 3261, 6189, 8673, 6711, 4111, 4808,
	    1799, 8446, 4580, 5769, 7575, 2158, 2852,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
	/* 63 */
	{ { 1271, 7191, 5710, 2310, 6447, 4473, 6831,   14, 3118, 4910, 8585,
	    7966,  180, 3580, 8044,  321, 1203,  698,  715,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 1707,
	    3058,  671, 6177, 6465, 4418, 2176, 2805, 3334, 5607, 9169, 6986,
	     537, 5730, 9248, 7456, 3878, 4090, 8596,    0,    0,    0,    0,
	       0,    0,    0,    0,    0,    0,    0,    0,    0 } },
};

/* see inner.h */
void
curve9767_inner_Icart_map(curve9767_point *Q, const uint16_t *u)
{
	/*
	 * TODO: convert to 'vgf' format.
	 */
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
static const uint8_t scalar_win5_off[] = {
	0x4C, 0xC8, 0x6A, 0x8C, 0xEA, 0x49, 0x7A, 0x1B,
	0xB1, 0xF1, 0x20, 0x3C, 0x33, 0x27, 0x3A, 0x43,
	0x8D, 0x37, 0x4F, 0x2B, 0xD5, 0x89, 0xDB, 0xA2,
	0x35, 0x93, 0x52, 0x4E, 0x20, 0x58, 0x8F, 0x09
};

static inline void
vpoint_lookup(vpoint *T, const vgf *win, uint32_t e)
{
	uint32_t e16, index, r;
	vgf x, y;
	__m256i m, cc, one;
	int i;

	/*
	 * Set e16 to 1 if e == 16, to 0 otherwise.
	 */
	e16 = (e & -e) >> 4;

	/*
	 * If e >= 17, lookup index must be e - 17.
	 * If e <= 15, lookup index must be 15 - e.
	 * If e == 16, lookup index is unimportant, as long as it is in
	 * the proper range (0..15). Code below sets index to 0 in that case.
	 *
	 * We set r to 1 if e <= 16, 0 otherwise. This conditions the
	 * final negation.
	 */
	index = e - 17;
	r = index >> 31;
	index = (index ^ -r) - r;
	index &= (e16 - 1);

	/*
	 * cc starts at index+1 and is decremented at each iteration.
	 * The mask m is all-ones then cc == 1, all-zeros otherwise.
	 */
	one = _mm256_set1_epi32(1);
	cc = _mm256_set1_epi32((int)(index + 1));
	m = _mm256_cmpeq_epi32(cc, one);
	x.u0 = _mm256_and_si256(m, win[0].u0);
	x.u1 = _mm_and_si128(_mm256_castsi256_si128(m), win[0].u1);
	y.u0 = _mm256_and_si256(m, win[1].u0);
	y.u1 = _mm_and_si128(_mm256_castsi256_si128(m), win[1].u1);
	for (i = 2; i < 32; i += 2) {
		cc = _mm256_sub_epi32(cc, one);
		m = _mm256_cmpeq_epi32(cc, one);
		x.u0 = _mm256_or_si256(x.u0,
			_mm256_and_si256(m, win[i + 0].u0));
		x.u1 = _mm_or_si128(x.u1, _mm_and_si128(
			_mm256_castsi256_si128(m), win[i + 0].u1));
		y.u0 = _mm256_or_si256(y.u0,
			_mm256_and_si256(m, win[i + 1].u0));
		y.u1 = _mm_or_si128(y.u1, _mm_and_si128(
			_mm256_castsi256_si128(m), win[i + 1].u1));
	}

	/*
	 * Set neutral flag depending on e == 16.
	 * Conditionally negate the y coordinate.
	 */
	T->neutral = e16;
	T->x = x;
	vgf_condneg(&T->y, &y, r);
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
	 *  - We store only 1*Q1, 2*Q1,... 16*Q1. The point to add will
	 *    be either one of these points, or the opposite of one of
	 *    these points. This "shifts" the window, and must be
	 *    counterbalanced by a constant offset applied to the scalar.
	 *
	 * Therefore:
	 *
	 *  1. Add 0x4210842108...210 to the scalar s, and normalize it to
	 *     0..n-1 (we do this by encoding the scalar to bytes).
	 *  2. Compute the window: j*Q1 (for j = 1..8).
	 *  3. Start with point Q3 = 0.
	 *  4. For i in 0..50:
	 *      - Compute Q3 <- 32*Q3
	 *      - Let e = bits[(250-5*i)..(254-5*i)] of s
	 *      - Let: T = -(16-e)*Q1  if 0 <= e <= 15
	 *             T = 0           if e == 16
	 *             T = (e-16)*Q1   if 17 <= e <= 31
	 *      - Compute Q3 <- Q3 + T
	 *
	 * All lookups should be done in constant-time, as well as additions
	 * and conditional negation of T.
	 *
	 * For the first iteration (i == 0), since Q3 is still 0 at that
	 * point, we can omit the multiplication by 32 and the addition,
	 * and simply set Q3 to T.
	 *
	 * To further speed up processing, we use Jabcobian coordinates
	 * for the temporary result (Q3). Formulas for point addition
	 * in Jacobian coordinates are not complete. However, the skewed
	 * scalar s is normalized (reduced modulo the curve order n), and
	 * the curve order is prime, which gives us some guarantees:
	 *
	 *  - A result of 0 is possible only if the source point is 0
	 *    and/or the scalar is 0.
	 *
	 *  - Suppose that the source point Q is not 0. After j iterations,
	 *    the current sum is:
	 *       A = \sum_{i=0}^{j-1} m_i*(2^(5*(j-1-i))*Q
	 *    where m_0 is the multiplier at the first iteration, and so
	 *    on. Each m_i is in the -16..+15 range. Therefore, the current
	 *    sum is k_j*Q for some integer k_j such that:
	 *       -16*m <= k_j <= +15*m
	 *    with m = (2^(5*j)-1)/31. The next iteration will compute:
	 *       32*A + m_j*Q
	 *    If j <= 49, then |32*k_j| is less than n/6.84, which implies
	 *    that 32*A cannot be equal to either m_j*Q or -m_j*Q (the
	 *    current discrete logarithm of A relatively to Q is too low
	 *    to have reached the "wrap-around" state). Thus, the j+1-th
	 *    iteration, with m_j != 0, will involve a "true" addition
	 *    which will not be a doubling or the addition of A with -A.
	 *
	 * Therefore, all iterations except the last one can be done without
	 * hitting a problematic case for the point addition formulas,
	 * provided that we handle (with conditional copies) the cases of
	 * A = 0 and m_j = 0.
	 *
	 * To handle the possible special cases at the last iteration, we
	 * simply convert back to affine coordinates and use the generic
	 * point addition routine for the last addition.
	 */
	curve9767_scalar ss;
	uint8_t sb[32];
	vpoint T, U;
	vgf win[32];
	vgf jX, jY, jZ, one;
	int i, j, k;
	uint32_t qz, rz;
	unsigned eb;
	int eb_len;

	/*
	 * Apply offset on the scalar and encode it into bytes. This
	 * involves normalization to 0..n-1.
	 */
	curve9767_scalar_decode_strict(&ss,
		scalar_win5_off, sizeof scalar_win5_off);
	curve9767_scalar_add(&ss, &ss, s);
	curve9767_scalar_encode(sb, &ss);

	/*
	 * Create window contents.
	 */
	vpoint_decode(&U, Q1);
	T = U;
	win[0] = T.x;
	win[1] = T.y;
	for (i = 2; i < 32; i += 2) {
		vpoint_add(&T, &T, &U);
		win[i + 0] = T.x;
		win[i + 1] = T.y;
	}

	/*
	 * Accumulator point coordinates are kept in (jX:jY:jZ). We
	 * initialize them explicitly just to appease static code
	 * analyzers (we won't read these values before overwriting them).
	 */
	memset(&jX, 0, sizeof jX);
	memset(&jY, 0, sizeof jY);
	memset(&jZ, 0, sizeof jZ);
	rz = 1;

	one.u0 = _mm256_setr_epi16(
		R, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P);
	one.u1 = _mm_setr_epi16(
		P, P, P, 0, 0, 0, 0, 0);

	/*
	 * Perform the chunk-by-chunk computation.
	 */
	qz = Q1->neutral;
	j = 31;
	eb = sb[j];
	eb_len = 7;
	for (i = 0; i < 51; i ++) {
		uint32_t e;
		vgf T1, T2, T3, T4, X3, Y3, Z3;
		__m256i md0, md1, md2;
		__m128i ms0, ms1, ms2;

		/*
		 * Extract exponent bits.
		 */
		if (eb_len < 5) {
			eb = (eb << 8) | sb[-- j];
			eb_len += 8;
		}
		eb_len -= 5;
		e = (eb >> eb_len) & 0x1F;

		/*
		 * Window lookup. Don't forget to adjust the neutral flag
		 * to account for the case of Q1 = infinity.
		 */
		vpoint_lookup(&T, win, e);
		T.neutral |= qz;

		/*
		 * Q3 <- 32*Q3 + T.
		 *
		 * If i == 0, then Q3 is necessarily 0 and we can omit
		 * the doublings and the addition.
		 */
		if (i == 0) {
			jX = T.x;
			jY = T.y;
			jZ = one;
			rz = T.neutral;
			continue;
		}

		/*
		 * Do the 5 successive doublings.
		 * We use the 4M+4S formulas from:
		 * https://hyperelliptic.org/EFD/g1p/auto-shortw-jacobian-3.html
		 */
		for (k = 0; k < 5; k ++) {
			vgf ZZ, M, S;

			/* ZZ = Z^2 */
			vgf_sqr(&ZZ, &jZ);

			/* M = X-ZZ
			   ZZ = X+ZZ
			   M = M*ZZ
			   M = 3*M
			   (ZZ is discarded) */
			vgf_sub(&M, &jX, &ZZ);
			vgf_add(&ZZ, &jX, &ZZ);
			vgf_mul(&M, &M, &ZZ);
			vgf_add(&ZZ, &M, &M);
			vgf_add(&M, &M, &ZZ);

			/* Y = 2*Y
			   Z = Y*Z */
			vgf_add(&jY, &jY, &jY);
			vgf_mul(&jZ, &jY, &jZ);

			/* Y = Y^2
			   S = Y*X
			   Y = (Y^2)/2 */
			vgf_sqr(&jY, &jY);
			vgf_mul(&S, &jY, &jX);
			vgf_sqr(&jY, &jY);
			vgf_mul_const(&jY, &jY, HALFm);

			/* X = M^2 */
			vgf_sqr(&jX, &M);

			/* ZZ = 2*S
			   X = X-ZZ */
			vgf_add(&ZZ, &S, &S);
			vgf_sub(&jX, &jX, &ZZ);

			/* ZZ = S-X
			   ZZ = ZZ*M
			   Y = ZZ-Y */
			vgf_sub(&ZZ, &S, &jX);
			vgf_mul(&ZZ, &ZZ, &M);
			vgf_sub(&jY, &ZZ, &jY);
		}

		/*
		 * If this is the last iteration, then we convert back
		 * to affine coordinates _before_ the last addition,
		 * and use the generic point addition routine, which
		 * handles the possible edge cases cleanly.
		 */
		if (i == 50) {
			vgf_inv(&T1, &jZ);
			vgf_sqr(&T2, &T1);
			vgf_mul(&T3, &T1, &T2);
			vgf_mul(&U.x, &jX, &T2);
			vgf_mul(&U.y, &jY, &T3);
			U.neutral = rz;
			vpoint_add(&U, &U, &T);
			vpoint_encode(Q3, &U);
			return;
		}

		/*
		 * Perform the addition (mixed addition of the current
		 * accumulator in Jacobian coordinates, and the point
		 * from the lookup, which is in affine coordinates).
		 *
		 * Formulas are 8M+3S (fewer additions than 7M+4S, and
		 * squarings are not faster than multiplications in the
		 * AVX2 code).
		 *
		 * Addition result is written in (X3:Y3:Z3) which we'll
		 * ultimately discard if either T or the current
		 * accumulator is the point-at-infinity.
		 */

		/* T1 = Z1^2
		   T2 = T1*Z1 */
		vgf_sqr(&T1, &jZ);
		vgf_mul(&T2, &T1, &jZ);

		/* T1 = T1*X2
		   T2 = T2*Y2 */
		vgf_mul(&T1, &T1, &T.x);
		vgf_mul(&T2, &T2, &T.y);

		/* T1 = T1-X1
		   T2 = T2-Y1 */
		vgf_sub(&T1, &T1, &jX);
		vgf_sub(&T2, &T2, &jY);

		/* Z3 = Z1*T1 */
		vgf_mul(&Z3, &jZ, &T1);

		/* T3 = T1^2
		   T4 = T3*T1
		   T3 = T3*X1 */
		vgf_sqr(&T3, &T1);
		vgf_mul(&T4, &T3, &T1);
		vgf_mul(&T3, &T3, &jX);

		/* T1 = 2*T3 */
		vgf_add(&T1, &T3, &T3);

		/* X3 = T2^2
		   X3 = X3-T1
		   X3 = X3-T4 */
		vgf_sqr(&X3, &T2);
		vgf_sub(&X3, &X3, &T1);
		vgf_sub(&X3, &X3, &T4);

		/* T3 = T3-X3
		   T3 = T3*T2 */
		vgf_sub(&T3, &T3, &X3);
		vgf_mul(&T3, &T3, &T2);

		/* T4 = T4*Y1
		   Y3 = T3-T4 */
		vgf_mul(&T4, &T4, &jY);
		vgf_sub(&Y3, &T3, &T4);

		/*
		 * If rz == 0 and T.neutral == 0: keep (X3:Y3:Z3)
		 * If rz == 1 and T.neutral == 0: keep (T.x:T.y:1)
		 * If rz == 0 and T.neutral == 1: keep (jX:jY:jZ)
		 * If rz == 1 and T.neutral == 1: no importance
		 */
		md0 = _mm256_set1_epi32(((int)rz - 1) & ((int)T.neutral - 1));
		md1 = _mm256_set1_epi32(-(int)rz & ((int)T.neutral - 1));
		md2 = _mm256_set1_epi32(((int)rz - 1) & -(int)T.neutral);
		ms0 = _mm256_castsi256_si128(md0);
		ms1 = _mm256_castsi256_si128(md1);
		ms2 = _mm256_castsi256_si128(md2);
		jX.u0 = _mm256_or_si256(
			_mm256_or_si256(
				_mm256_and_si256(md2, jX.u0),
				_mm256_and_si256(md1, T.x.u0)),
			_mm256_and_si256(md0, X3.u0));
		jX.u1 = _mm_or_si128(
			_mm_or_si128(
				_mm_and_si128(ms2, jX.u1),
				_mm_and_si128(ms1, T.x.u1)),
			_mm_and_si128(ms0, X3.u1));
		jY.u0 = _mm256_or_si256(
			_mm256_or_si256(
				_mm256_and_si256(md2, jY.u0),
				_mm256_and_si256(md1, T.y.u0)),
			_mm256_and_si256(md0, Y3.u0));
		jY.u1 = _mm_or_si128(
			_mm_or_si128(
				_mm_and_si128(ms2, jY.u1),
				_mm_and_si128(ms1, T.y.u1)),
			_mm_and_si128(ms0, Y3.u1));
		jZ.u0 = _mm256_or_si256(
			_mm256_or_si256(
				_mm256_and_si256(md2, jZ.u0),
				_mm256_and_si256(md1, one.u0)),
			_mm256_and_si256(md0, Z3.u0));
		jZ.u1 = _mm_or_si128(
			_mm_or_si128(
				_mm_and_si128(ms2, jZ.u1),
				_mm_and_si128(ms1, one.u1)),
			_mm_and_si128(ms0, Z3.u1));

		/*
		 * We can have a neutral point here only if the accumulator
		 * was still the neutral point, AND the looked up point
		 * is the neutral point.
		 */
		rz &= T.neutral;
	}
}

/* see curve9767.h */
void
curve9767_point_mulgen(curve9767_point *Q3, const curve9767_scalar *s)
{
	/*
	 * We apply the same algorithm as curve9767_point_mul(), but
	 * with four 65-bit scalars instead of one 252-bit scalar,
	 * thus mutualizing the point doublings. This requires four
	 * precomputed windows (each has size 2048 bytes; this is
	 * 8 kB in total, which should fit nicely in L1 cache).
	 */
	curve9767_scalar ss;
	union {
		uint8_t b[32];
		uint64_t w[4];
	} se;
	vpoint T, U;
	int i;

	/*
	 * Apply offset on the scalar and encode it into bytes. This
	 * involves normalization to 0..n-1.
	 */
	curve9767_scalar_decode_strict(&ss,
		scalar_win5_off, sizeof scalar_win5_off);
	curve9767_scalar_add(&ss, &ss, s);
	curve9767_scalar_encode(se.b, &ss);

	/*
	 * Perform the chunk-by-chunk computation. For each iteration,
	 * we use 4 chunks of 5 bits from the scalar, to make 5 lookups
	 * in the relevant precomputed windows.
	 *
	 * Source scalar is 252 bits; thus, the three low chunks are
	 * 65 bits each, but the high chunk is only 57 bits; we can skip
	 * one lookup and one addition for bits 60..64 of that chunk.
	 */

	/*
	 * Bits 60..64 are in sb[7] and sb[8]
	 * Bits 125..129 are in sb[15] and sb[16]
	 * Bits 190..194 are in sb[23] and sb[24]
	 */
	vpoint_lookup(&U, (const vgf *)&window5_G,
		((se.b[7] >> 4) | (se.b[8] << 4)) & 0x1F);
	vpoint_lookup(&T, (const vgf *)&window5_G65,
		((se.b[15] >> 5) | (se.b[16] << 3)) & 0x1F);
	vpoint_add(&U, &U, &T);
	vpoint_lookup(&T, (const vgf *)&window5_G130,
		((se.b[23] >> 6) | (se.b[24] << 2)) & 0x1F);
	vpoint_add(&U, &U, &T);

	for (i = 1; i < 13; i ++) {
		uint32_t e0, e1, e2, e3;
		int j;

		/*
		 * Extract exponent bits.
		 */
		j = 5 * (12 - i);
		e0 = (se.w[0] >> j) & 0x1F;
		e1 = (se.w[1] >> (j + 1)) & 0x1F;
		e2 = (se.w[2] >> (j + 2)) & 0x1F;
		e3 = (se.w[3] >> (j + 3)) & 0x1F;

		/*
		 * Window lookups and additions.
		 */
		vpoint_mul2k(&U, &U, 5);
		vpoint_lookup(&T, (const vgf *)&window5_G, e0);
		vpoint_add(&U, &U, &T);
		vpoint_lookup(&T, (const vgf *)&window5_G65, e1);
		vpoint_add(&U, &U, &T);
		vpoint_lookup(&T, (const vgf *)&window5_G130, e2);
		vpoint_add(&U, &U, &T);
		vpoint_lookup(&T, (const vgf *)&window5_G195, e3);
		vpoint_add(&U, &U, &T);
	}
	vpoint_encode(Q3, &U);
}

/* see curve9767.h */
void
curve9767_point_mul_mulgen_add(curve9767_point *Q3,
	const curve9767_point *Q1, const curve9767_scalar *s1,
	const curve9767_scalar *s2)
{
	curve9767_scalar ss;
	uint8_t sb1[32], sb2[32];
	vpoint T, U;
	vgf win[32];
	int i, j, eb_len;
	uint32_t qz, eb1, eb2;

	/*
	 * Apply offset on both scalars and encode them into bytes. This
	 * involves normalization to 0..n-1.
	 */
	curve9767_scalar_decode_strict(&ss,
		scalar_win5_off, sizeof scalar_win5_off);
	curve9767_scalar_add(&ss, &ss, s1);
	curve9767_scalar_encode(sb1, &ss);
	curve9767_scalar_decode_strict(&ss,
		scalar_win5_off, sizeof scalar_win5_off);
	curve9767_scalar_add(&ss, &ss, s2);
	curve9767_scalar_encode(sb2, &ss);

	/*
	 * Create window contents (for Q1).
	 */
	vpoint_decode(&U, Q1);
	T = U;
	win[0] = T.x;
	win[1] = T.y;
	for (i = 2; i < 32; i += 2) {
		vpoint_add(&T, &T, &U);
		win[i + 0] = T.x;
		win[i + 1] = T.y;
	}

	/*
	 * Perform the chunk-by-chunk computation.
	 */
	qz = Q1->neutral;
	j = 31;
	eb1 = sb1[j];
	eb2 = sb2[j];
	eb_len = 7;
	for (i = 0; i < 51; i ++) {
		uint32_t e1, e2;

		/*
		 * Extract exponent bits.
		 */
		if (eb_len < 5) {
			j --;
			eb1 = (eb1 << 8) | sb1[j];
			eb2 = (eb2 << 8) | sb2[j];
			eb_len += 8;
		}
		eb_len -= 5;
		e1 = (eb1 >> eb_len) & 0x1F;
		e2 = (eb2 >> eb_len) & 0x1F;

		/*
		 * Window lookup. Don't forget to adjust the neutral flag
		 * to account for the case of Q1 = infinity.
		 */
		vpoint_lookup(&T, win, e1);
		T.neutral |= qz;

		/*
		 * Q3 <- 32*Q3 + T.
		 *
		 * If i == 0, then we know that Q3 is (conceptually) 0,
		 * and we can simply set Q3 to T.
		 */
		if (i == 0) {
			U = T;
		} else {
			vpoint_mul2k(&U, &U, 5);
			vpoint_add(&U, &U, &T);
		}

		/*
		 * Do lookup in the generator window as well.
		 */
		vpoint_lookup(&T, (const vgf *)&window5_G, e2);
		vpoint_add(&U, &U, &T);
	}
	vpoint_encode(Q3, &U);
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
 * Apply NAF_w recoding. Input c[] must consist of 'len' bytes, in
 * unsigned little-endian notation; the top bit of the last byte must be
 * zero. rc[] will be filled with 8*len values, each either zero or an
 * odd integer in the -2^(w-1)..+2^(w-1) range. On average, only 1/(w+1)
 * rc[] values will be non-zero. Window size w must be between 2 and 8
 * (inclusive).
 */
static void
recode_NAFw(int8_t *rc, const uint8_t *c, size_t len, int w)
{
	unsigned acc, mask;
	int acc_len;
	size_t u, v, rclen;

	acc = 0;
	acc_len = 0;
	u = 0;
	mask = (1u << w) - 1u;
	rclen = len << 3;
	for (v = 0; v < rclen; v ++) {
		if (acc_len < w && u < len) {
			acc += (unsigned)c[u ++] << acc_len;
			acc_len += 8;
		}
		if ((acc & 1) == 0) {
			rc[v] = 0;
		} else {
			unsigned t;

			t = acc & mask;
			acc &= ~mask;
			if ((t >> (w - 1)) != 0) {
				rc[v] = (int)t - (1 << w);
				acc += 1u << w;
			} else {
				rc[v] = (int)t;
			}
		}
		acc >>= 1;
		acc_len --;
	}
}

/*
 * Inner function for curve9767_point_verify_mul_mulgen_add_vartime();
 * made as a separate function so that stack allocation for the point
 * windows does not cumulate with that of lattice basis reduction.
 *
 * This computes Q3 = c0*Q0 + c1*Q1 + c2*G. Values c0 and c1 are provided
 * as their absolute value (less than 2^127) and sign (1 for negative, 0
 * for zero and positive); value c2 is unsigned and over 252 bits.
 *
 * THIS IS NOT CONSTANT-TIME.
 */
void
mul2_mulgen_add_vartime(vpoint *Q3,
	const vpoint *Q0, const uint8_t *c0, int neg0,
	const vpoint *Q1, const uint8_t *c1, int neg1,
	const uint8_t *c2)
{
	/*
	 * Algorithm: we use NAF_w representation for multipliers;
	 *   x = \sum x_i*2^i
	 * with x_i = 1 mod 2 and |x_i| < 2^(w-1). On average, only
	 * 1 in w+1 coefficients x_i will be non-zero. With w = 5,
	 * the coefficients x_i are odd integers in the -15..+15 range.
	 * Since we can efficiently negate points, we only need a
	 * window of 8 points (Q, 3*Q, 5*Q,... 15*Q).
	 *
	 * For c2 (multiplier of G), two differences apply:
	 *  - We split c2 into two 128-bit halves:
	 *       c2*G = (c2 mod 2^128)*G + floor(c2 / 2^128)*(2^128)*G
	 *  - Since the windows for G and (2^128)*G are precomputed,
	 *    we can use larger windows with 32 points each, i.e. w = 7
	 *    (grand total size is 8 kB, i.e. still fitting with ease
	 *    into L1 cache).
	 */

	int8_t rc0[128], rc1[128], rc2[256];
	vpoint W0[8], W1[8], T;
	int i, dbl;

	/*
	 * Recode multipliers with NAF_w. Values in rc0, rc1 and rc2 will
	 * be odd integers in the -15..+15 range; for rc2, values will be
	 * odd integers in the -63..+63 range.
	 */
	recode_NAFw(rc0, c0, 16, 5);
	recode_NAFw(rc1, c1, 16, 5);
	recode_NAFw(rc2, c2, 32, 7);

	/*
	 * Make window for Q0. If c0 is negative (neg0 == 1), we set:
	 *   W0[k] = -(2*k+1)*Q0
	 * Otherwise, we set:
	 *   W0[k] = (2*k+1)*Q0
	 */
	W0[0] = *Q0;
	if (neg0) {
		vgf_neg(&W0[0].y, &W0[0].y);
	}
	vpoint_add(&T, &W0[0], &W0[0]);
	for (i = 1; i < 8; i ++) {
		vpoint_add(&W0[i], &W0[i - 1], &T);
	}

	/*
	 * Similar window construction for Q1.
	 */
	W1[0] = *Q1;
	if (neg1) {
		vgf_neg(&W1[0].y, &W1[0].y);
	}
	vpoint_add(&T, &W1[0], &W1[0]);
	for (i = 1; i < 8; i ++) {
		vpoint_add(&W1[i], &W1[i - 1], &T);
	}

	/*
	 * Do the window based point multiplication. Q0 and Q1 use the
	 * computed windows W0 and W1, with multipliers from rc0[] and
	 * rc1[]. c2*G uses the multipliers from rc2[], in two parallel
	 * tracks, one with G, the other with (2^128)*G. The windows
	 * for G and (2^128)*G are precomputed.
	 */
	vpoint_set_neutral(Q3);
	dbl = 0;
	for (i = 127; i >= 0; i --) {
		int m0, m1, m2, m3;

		dbl ++;
		m0 = rc0[i];
		m1 = rc1[i];
		m2 = rc2[i];
		m3 = rc2[i + 128];
		if ((m0 | m1 | m2 | m3) == 0) {
			continue;
		}
		if (!Q3->neutral) {
			vpoint_mul2k(Q3, Q3, dbl);
		}
		dbl = 0;

		if (m0 > 0) {
			vpoint_add(Q3, Q3, &W0[m0 >> 1]);
		} else if (m0 < 0) {
			vpoint_neg(&T, &W0[(-m0) >> 1]);
			vpoint_add(Q3, Q3, &T);
		}

		if (m1 > 0) {
			vpoint_add(Q3, Q3, &W1[m1 >> 1]);
		} else if (m1 < 0) {
			vpoint_neg(&T, &W1[(-m1) >> 1]);
			vpoint_add(Q3, Q3, &T);
		}

		if (m2 > 0) {
			T.x = ((const vgf *)window_odd7_G)[m2 - 1];
			T.y = ((const vgf *)window_odd7_G)[m2];
			T.neutral = 0;
			vpoint_add(Q3, Q3, &T);
		} else if (m2 < 0) {
			T.x = ((const vgf *)window_odd7_G)[-m2 - 1];
			vgf_neg(&T.y, &((const vgf *)window_odd7_G)[-m2]);
			T.neutral = 0;
			vpoint_add(Q3, Q3, &T);
		}

		if (m3 > 0) {
			T.x = ((const vgf *)window_odd7_G128)[m3 - 1];
			T.y = ((const vgf *)window_odd7_G128)[m3];
			T.neutral = 0;
			vpoint_add(Q3, Q3, &T);
		} else if (m3 < 0) {
			T.x = ((const vgf *)window_odd7_G128)[-m3 - 1];
			vgf_neg(&T.y, &((const vgf *)window_odd7_G128)[-m3]);
			T.neutral = 0;
			vpoint_add(Q3, Q3, &T);
		}
	}

	if (dbl > 0 && Q3->neutral == 0) {
		vpoint_mul2k(Q3, Q3, dbl);
	}
}

/* see inner.h */
void
curve9767_inner_mul2_mulgen_add_vartime(curve9767_point *Q3,
	const curve9767_point *Q0, const uint8_t *c0, int neg0,
	const curve9767_point *Q1, const uint8_t *c1, int neg1,
	const uint8_t *c2)
{
	vpoint vQ0, vQ1, vQ3;

	vpoint_decode(&vQ0, Q0);
	vpoint_decode(&vQ1, Q1);
	mul2_mulgen_add_vartime(&vQ3, &vQ0, c0, neg0, &vQ1, c1, neg1, c2);
	vpoint_encode(Q3, &vQ3);
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
	 * as implemented by curve9767_inner_vartime_reduce_basis(),
	 * does exactly that, and returns c1 and c0 = c1*s1 as two
	 * signed integers whose absolute value is less than 2^127.
	 */
	uint8_t c0[16], c1[16], c2[32];
	curve9767_scalar ss;
	vpoint vQ1, vQ2, vT;
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
	vpoint_decode(&vQ1, Q1);
	vpoint_decode(&vQ2, Q2);
	mul2_mulgen_add_vartime(&vT, &vQ1, c0, neg0, &vQ2, c1, 1 - neg1, c2);

	/*
	 * The equation is verified if and only if the result if the
	 * point at infinity.
	 */
	return vT.neutral;
}

/*
 * Dummy function used for calibration in benchmarks.
 */
void
curve9767_inner_gf_dummy_1(uint16_t *c, const uint16_t *a)
{
	vgf va, vc;

	vgf_decode(&va, a);
	vc.u0 = va.u0;
	vc.u1 = va.u1;
	vgf_encode(c, &vc);
}

/*
 * Dummy function used for calibration in benchmarks.
 */
void
curve9767_inner_gf_dummy_2(uint16_t *c, const uint16_t *a, const uint16_t *b)
{
	vgf va, vb, vc;

	vgf_decode(&va, a);
	vgf_decode(&vb, b);
	vc.u0 = _mm256_add_epi16(va.u0, vb.u0);
	vc.u1 = _mm_add_epi16(va.u1, vb.u1);
	vgf_encode(c, &vc);
}
