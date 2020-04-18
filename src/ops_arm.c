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
	return 1 + ((((uint32_t)(x * P1I) >> 16) * P) >> 16);
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
	 * Extract Y as a square root (y). A failure is reported if the
	 * value t1 is not a quadratic resiude.
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

/* see inner.h */
void
curve9767_inner_window_put(window_point8 *window,
	const curve9767_point *Q, uint32_t k)
{
	/*
	 * We interleave the X and Y coordinates of the points so that
	 * lookups are a bit faster. Nameley, in memory order, the
	 * window contains:
	 *
	 *  - X0[0] | (X0[1] << 16)
	 *  - X1[0] | (X1[1] << 16)
	 *  - X2[0] | (X2[1] << 16)
	 *   ...
	 *  - X7[0] | (X7[1] << 16)
	 *  - X0[2] | (X0[3] << 16)
	 *  - X1[2] | (X1[3] << 16)
	 *   ...
	 *  - X7[2] | (X7[3] << 16)
	 *   ...
	 *
	 * The Y coordinates come after the X coordinates.
	 */
	uint32_t *ww;
	uint32_t u;

	ww = &window->v[0].w[0];
	for (u = 0; u < 9; u ++) {
		ww[(u << 3) + k +  0] = Q->x[(u << 1) + 0]
			+ ((uint32_t)Q->x[(u << 1) + 1] << 16);
		ww[(u << 3) + k + 80] = Q->y[(u << 1) + 0]
			+ ((uint32_t)Q->y[(u << 1) + 1] << 16);
	}
	ww[72 + k +  0] = Q->x[18];
	ww[72 + k + 80] = Q->y[18];
}

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

/*
 * This function must not be made inline, not only because it's invoked
 * from the test code, but also because if the compiler inlines it into
 * curve9767_point_verify_mul_mulgen_add_vartime() then the stack use
 * will add up with the one induced by
 * curve9767_inner_reduce_basis_vartime(), and that can be a bit too
 * much on constrained architectures with very scarce RAM resources.
 */
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
