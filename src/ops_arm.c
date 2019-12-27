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
