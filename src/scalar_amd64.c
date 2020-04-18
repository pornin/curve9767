#include "inner.h"

/*
 * We use the intrinsic functions:
 *   _lzcnt_u32(), _lzcnt_u64(), _addcarry_u64(), _subborrow_u64().
 */
#include <immintrin.h>

/*
 * Operation on scalars (reference implementation).
 *
 * We represent a scalar as a sequence of 17 words of 15 bits each, in
 * little-endian order. Each word is held in a uint16_t; the top bit is
 * always zero. Scalars may use a slightly larger-than-necessary range.
 *
 * Montgomery multiplication is used: if sR = 2^255 mod n, then Montgomery
 * multiplication of a and b yields (a*b)/sR mod n. The "Montgomery
 * representation" of x is x*sR mod n. In general we do not keep scalars
 * in Montgomery representation, because we expect encoding/decoding
 * operations to be more frequent and multiplications (keeping values
 * in Montgomery representation is a gain when several multiplications are
 * performed successively).
 *
 * Each function has its own acceptable ranges for input values, and a
 * guaranteed output range. In general, we require scalar values at the
 * API level to be lower than 1.27*n.
 */

/*
 * Curve order, in base 2^15 (little-endian order).
 */
static const uint16_t order[] = {
	24177, 19022, 18073, 22927, 18879, 12156,  7504, 10559, 11571,
	26856, 15192, 22896, 14840, 31722,  2974,  9600,  3616
};

/*
 * sR2 = 2^510 mod n.
 */
static const uint16_t sR2[] = { 
	14755,  1449,  7175,  1324, 11384, 15866, 31249, 13920, 17944,
	 6728,  3858,  5900, 25302,   432,  5554, 29779,  1646
};

/*
 * sD = 2^503 mod n  (Montgomery representation of 2^248).
 */
static const uint16_t sD[] = {
	  167,  1579, 26634, 10886, 24646, 12845, 32322,  7660,  8304,
	12054, 20731,  3487, 26407,  9107, 22337,  7191,  1284
};

/*
 * n mod 2^15
 */
#define N0    24177

/*
 * -1/n mod 2^15
 */
#define N0I   23919

/* see curve9767.h */
const curve9767_scalar curve9767_scalar_zero = {
	{ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }
};

/* see curve9767.h */
const curve9767_scalar curve9767_scalar_one = {
	{ { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }
};

/*
 * Addition.
 * Input operands must be less than 1.56*n each.
 * Output is lower than 2^252, hence lower than 1.14*n.
 */
static void
scalar_add(uint16_t *c, const uint16_t *a, const uint16_t *b)
{
	/*
	 * Since a < 1.56*n and b < 1.56*n, we have a+b < 3.12*n.
	 * We subtract n repeatedly as long as the value is not
	 * lower than 2^252. Since 3.12*n < 2^252 + 2*n, two
	 * conditional subtractions are enough. Moreover, since
	 * 2^252 < 1.14*n, the result is in the appropriate range.
	 */
	uint16_t d[17];
	int i, j;
	uint32_t cc;

	cc = 0;
	for (i = 0; i < 17; i ++) {
		uint32_t w;

		w = a[i] + b[i] + cc;
		d[i] = (uint16_t)(w & 0x7FFF);
		cc = w >> 15;
	}

	/*
	 * No possible carry at this point (cc = 0). We now subtract n
	 * conditionally on d >= 2^252 (and we do it twice).
	 */
	for (j = 0; j < 2; j ++) {
		uint32_t m;

		/*
		 * d / (2^252) is equal to 0, 1 or 2 at this point. We
		 * set m to 0xFFFFFFFF is the value is 1 or 2, to
		 * 0x00000000 otherwise.
		 */
		m = (uint32_t)d[16] >> 12;
		m = -(-m >> 31);

		cc = 0;
		for (i = 0; i < 17; i ++) {
			uint32_t wd, wn;

			wd = d[i];
			wn = m & order[i];
			wd = wd - wn - cc;
			d[i] = (uint16_t)(wd & 0x7FFF);
			cc = wd >> 31;
		}
	}

	memcpy(c, d, sizeof d);
}

/*
 * Subtraction.
 * Input operand 'a' must be less than 2*n.
 * Input operand 'b' must be less than 2*n.
 * Output is lower than 'a'.
 */
static void
scalar_sub(uint16_t *c, const uint16_t *a, const uint16_t *b)
{
	/*
	 * We compute a-b, then add n repeatedly until the result is
	 * nonnegative. Since b < 2*n, two additions are sufficient.
	 */
	uint16_t d[17];
	int i, j;
	uint32_t cc;

	cc = 0;
	for (i = 0; i < 17; i ++) {
		uint32_t w;

		w = (uint32_t)a[i] - (uint32_t)b[i] - cc;
		d[i] = (uint16_t)(w & 0x7FFF);
		cc = w >> 31;
	}

	/*
	 * Condition on adding n is that the value is negative. This shows
	 * up as the top bit of the top word being 1.
	 */
	for (j = 0; j < 2; j ++) {
		uint32_t m;

		m = -((uint32_t)d[16] >> 14);
		cc = 0;
		for (i = 0; i < 17; i ++) {
			uint32_t wd, wn;

			wd = d[i];
			wn = m & order[i];
			wd = wd + wn + cc;
			d[i] = (uint16_t)(wd & 0x7FFF);
			cc = wd >> 15;
		}
	}

	memcpy(c, d, sizeof d);
}

/*
 * Normalize a scalar into the 0..n-1 range.
 * Input must be less than 2*n.
 *
 * Returned value is 1 if the source value was already in the 0..n-1 range,
 * 0 otherwise.
 */
static uint32_t
scalar_normalize(uint16_t *c, const uint16_t *a)
{
	uint16_t d[17];
	int i;
	uint32_t cc;

	/*
	 * We subtract n. If the result is nonnegative, we keep it;
	 * otherwise, we use the source value.
	 */
	cc = 0;
	for (i = 0; i < 17; i ++) {
		uint32_t w;

		w = (uint32_t)a[i] - (uint32_t)order[i] - cc;
		d[i] = (uint16_t)(w & 0x7FFF);
		cc = w >> 31;
	}

	/*
	 * If cc == 1, then the result was negative, and we must add
	 * back the modulus n. Otherwise, cc == 0.
	 */
	cc = -cc;
	for (i = 0; i < 17; i ++) {
		uint32_t wa, wd;

		wa = a[i];
		wd = d[i];
		c[i] = (uint16_t)(wd ^ (cc & (wa ^ wd)));
	}

	/*
	 * If the value was in the proper range, then the first subtraction
	 * yielded a negative value, and cc == -1; otherwise, the first
	 * subtraction did not yield a negative value, and cc == 0 here.
	 */
	return -cc;
}

/*
 * Decode bytes into a scalar value. At most 32 bytes will be read, and,
 * in the 32nd byte, only the low 4 bits are considered; the upper bits
 * and subsequent bytes are ignored (in effect, this truncates the value
 * modulo 2^252).
 *
 * Output is lower than 2^252, which is lower than 1.14*n.
 */
static void
scalar_decode_trunc(uint16_t *c, const void *src, size_t len)
{
	const uint8_t *buf;
	int i;
	size_t u;
	uint32_t acc;
	int acc_len;

	buf = src;
	i = 0;
	acc = 0;
	acc_len = 0;
	for (u = 0; u < len; u ++) {
		if (u == 31) {
			acc |= (uint32_t)(buf[31] & 0x0F) << 8;
			c[16] = (uint16_t)acc;
			return;
		} else {
			acc |= (uint32_t)buf[u] << acc_len;
			acc_len += 8;
			if (acc_len >= 15) {
				c[i ++] = (uint16_t)(acc & 0x7FFF);
				acc >>= 15;
				acc_len -= 15;
			}
		}
	}
	if (acc_len > 0) {
		c[i ++] = (uint16_t)acc;
	}
	while (i < 17) {
		c[i ++] = 0;
	}
}

/*
 * Perform Montgomery multiplication: c = (a*b)/sR.
 *
 * Input values must be lower than 1.27*n.
 * Output value is lower than 1.18*n.
 */
static void
scalar_mmul(uint16_t *c, const uint16_t *a, const uint16_t *b)
{
	int i, j;
	uint16_t d[17];
	uint32_t dh;

	memset(d, 0, sizeof d);
	dh = 0;
	for (i = 0; i < 17; i ++) {
		uint32_t f, g, t, cc;

		f = a[i];
		t = d[0] + f * b[0];
		g = (t * N0I) & 0x7FFF;
		cc = (t + g * N0) >> 15;
		for (j = 1; j < 17; j ++) {
			uint32_t h;

			/*
			 * If cc <= 2^16, then:
			 *   h <= 2^15-1 + 2*(2^15-1)^2 + 2^16
			 * Thus:
			 *   h < ((2^15)+1)*(2^16)
			 * Therefore, cc will be at most 2^16 on output.
			 */
			h = d[j] + f * b[j] + g * order[j] + cc;
			d[j - 1] = (uint16_t)(h & 0x7FFF);
			cc = h >> 15;
		}

		/*
		 * If dh <= 1, then dh+cc <= 1+2^16 < 2^17, and the new
		 * dh value can be only 0 or 1.
		 */
		dh += cc;
		d[16] = dh & 0x7FFF;
		dh >>= 15;
	}

	/*
	 * The loop above computed:
	 *   d = (a*b + k*n) / (2^255)
	 * with k < 2^255 (the successive values 'g' in the loop were the
	 * representation of 'k' in base 2^15). Since we assumed that,
	 * on input:
	 *   a < 1.27*n
	 *   b < 1.27*n
	 * it follows that:
	 *   d < (1.27*n)^2 + (2^255)*n) / (2^255)
	 * This latter bound is about 1.178*n, hence d < 1.18*n. This
	 * means that d is already in the proper range, and, in
	 * particular, that dh = 0.
	 */
	memcpy(c, d, sizeof d);
}

/* see curve9767.h */
uint32_t
curve9767_scalar_decode_strict(curve9767_scalar *s, const void *src, size_t len)
{
	/*
	 * Set dummy alignment word (to appease some sanitizing tools).
	 */
	s->v.w16[17] = 0;

	/*
	 * Decode the first 252 bits into a value.
	 */
	scalar_decode_trunc(s->v.w16, src, len);

	/*
	 * If the input length was at most 31 bytes, then the value is
	 * less than 2^248, hence necessarily correct. If it is 32 bytes
	 * or more, then we must verify that the value as decoded above is
	 * indeed lower than n, AND that all the ignored bits were zero.
	 */
	if (len >= 32) {
		const uint8_t *buf;
		size_t u;
		uint32_t r;

		buf = src;
		r = buf[31] >> 4;
		for (u = 32; u < len; u ++) {
			r |= buf[u];
		}
		r = (r - 1) >> 31;
		r &= scalar_normalize(s->v.w16, s->v.w16);
		return r;
	} else {
		return 1;
	}
}

/* see curve9767.h */
void
curve9767_scalar_decode_reduce(curve9767_scalar *s, const void *src, size_t len)
{
	/*
	 * Set dummy alignment word (to appease some sanitizing tools).
	 */
	s->v.w16[17] = 0;

	/*
	 * Principle: we decode the input by chunks of 31 bytes, in
	 * big-endian order (the bytes are in little-endian order, but we
	 * process the chunks from most to least significant). For each
	 * new chunk to process, we multiply the current value by 2^248,
	 * then add the chunk. Multiplication by 2^248 is done with a
	 * Montgomery multiplication by sD = 2^248*2^255 mod n.
	 */
	if (len <= 31) {
		scalar_decode_trunc(s->v.w16, src, len);
	} else {
		const uint8_t *buf;
		size_t u;

		buf = src;
		for (u = 0; u + 31 < len; u += 31);
		scalar_decode_trunc(s->v.w16, buf + u, len - u);

		/*
		 * At the entry of each iteration, we have s < 1.27*n.
		 * Montgomery multiplication returns a value less
		 * than 1.18*n, and decoding a 31-byte chunk yields
		 * a value less than 2^248 which is approximately 0.071*n.
		 * Therefore, the addition yields a value less than 1.27*n.
		 */
		while (u > 0) {
			uint16_t t[17];

			u -= 31;
			scalar_mmul(s->v.w16, s->v.w16, sD);
			scalar_decode_trunc(t, buf + u, 31);
			scalar_add(s->v.w16, s->v.w16, t);
		}
	}
}

/* see curve9767.h */
void
curve9767_scalar_encode(void *dst, const curve9767_scalar *s)
{
	uint16_t t[17];
	int i;
	unsigned acc;
	int acc_len;
	uint8_t *buf;
	size_t u;

	scalar_normalize(t, s->v.w16);
	buf = dst;
	u = 0;
	acc = 0;
	acc_len = 0;
	for (i = 0; i < 17; i ++) {
		acc |= (uint32_t)t[i] << acc_len;
		acc_len += 15;
		while (acc_len >= 8) {
			buf[u ++] = (uint8_t)acc;
			acc >>= 8;
			acc_len -= 8;
		}
	}
	buf[31] = (uint8_t)acc;
}

/* see curve9767.h */
int
curve9767_scalar_is_zero(const curve9767_scalar *s)
{
	uint16_t t[17];
	uint32_t r;
	int i;

	scalar_normalize(t, s->v.w16);
	r = 0;
	for (i = 0; i < 17; i ++) {
		r |= t[i];
	}
	return 1 - (-r >> 31);
}

/* see curve9767.h */
int
curve9767_scalar_eq(const curve9767_scalar *a, const curve9767_scalar *b)
{
	/*
	 * We cannot simply compare the two representations because they
	 * might need normalization.
	 */
	curve9767_scalar c;

	curve9767_scalar_sub(&c, a, b);
	return curve9767_scalar_is_zero(&c);
}

/* see curve9767.h */
void
curve9767_scalar_add(curve9767_scalar *c,
	const curve9767_scalar *a, const curve9767_scalar *b)
{
	/*
	 * Set dummy alignment word (to appease some sanitizing tools).
	 */
	c->v.w16[17] = 0;

	scalar_add(c->v.w16, a->v.w16, b->v.w16);
}

/* see curve9767.h */
void
curve9767_scalar_sub(curve9767_scalar *c,
	const curve9767_scalar *a, const curve9767_scalar *b)
{
	/*
	 * Set dummy alignment word (to appease some sanitizing tools).
	 */
	c->v.w16[17] = 0;

	scalar_sub(c->v.w16, a->v.w16, b->v.w16);
}

/* see curve9767.h */
void
curve9767_scalar_neg(curve9767_scalar *c, const curve9767_scalar *a)
{
	/*
	 * Set dummy alignment word (to appease some sanitizing tools).
	 */
	c->v.w16[17] = 0;

	scalar_sub(c->v.w16, curve9767_scalar_zero.v.w16, a->v.w16);
}

/* see curve9767.h */
void
curve9767_scalar_mul(curve9767_scalar *c,
	const curve9767_scalar *a, const curve9767_scalar *b)
{
	uint16_t t[17];

	/*
	 * Set dummy alignment word (to appease some sanitizing tools).
	 */
	c->v.w16[17] = 0;

	scalar_mmul(t, a->v.w16, sR2);
	scalar_mmul(c->v.w16, t, b->v.w16);
}

/* see curve9767.h */
void
curve9767_scalar_condcopy(curve9767_scalar *d,
	const curve9767_scalar *s, uint32_t ctl)
{
	int i;

	ctl = -ctl;
	for (i = 0; i < 9; i ++) {
		d->v.w32[i] ^= ctl & (d->v.w32[i] ^ s->v.w32[i]);
	}
}

/*
 * For lattice basis reduction, we use an other representation with
 * 64-bit limbs, in 64-bit words (no padding bit). "Small integers"
 * fit on 4 limbs, "large integers" use 8 limbs. Values are signed;
 * two's complement is used for negative values.
 */

/*
 * Convert a value from 15-bit limbs to 64-bit limbs (small value). The
 * source value is supposed to have length exactly 17 words of 15 bits.
 * The source value MUST be lower than 2^255.
 */
static void
to_small(uint64_t *d, const uint16_t *a)
{
	d[0] = (uint64_t)a[ 0]
		| ((uint64_t)a[ 1] << 15) | ((uint64_t)a[ 2] << 30)
		| ((uint64_t)a[ 3] << 45) | ((uint64_t)a[ 4] << 60);
	d[1] = ((uint64_t)a[ 4] >>  4)
		| ((uint64_t)a[ 5] << 11) | ((uint64_t)a[ 6] << 26)
		| ((uint64_t)a[ 7] << 41) | ((uint64_t)a[ 8] << 56);
	d[2] = ((uint64_t)a[ 8] >>  8)
		| ((uint64_t)a[ 9] <<  7) | ((uint64_t)a[10] << 22)
		| ((uint64_t)a[11] << 37) | ((uint64_t)a[12] << 52);
	d[3] = ((uint64_t)a[12] >> 12)
		| ((uint64_t)a[13] <<  3) | ((uint64_t)a[14] << 18)
		| ((uint64_t)a[15] << 33) | ((uint64_t)a[16] << 48);
}

/*
 * Encode a value into bytes (little-endian signed convention). The
 * output length MUST be at most 32 bytes. The source value is
 * assumed to numerically fit in the output (it will be truncated
 * otherwise).
 */
static inline void
encode_small(uint8_t *d, size_t d_len, const uint64_t *x)
{
	/*
	 * x86 are little-endian.
	 */
	memcpy(d, x, d_len);
}

/*
 * Multiply two small values, result in a large value. The two source
 * values MUST be nonnegative and in the proper range.
 */
static void
mul(uint64_t *d, const uint64_t *a, const uint64_t *b)
{
	size_t i, j;
	uint64_t t[8];

	memset(t, 0, sizeof t);
	for (i = 0; i < 4; i ++) {
		uint64_t cc;

		cc = 0;
		for (j = 0; j < 4; j ++) {
			unsigned __int128 z;

			z = (unsigned __int128)a[i] * (unsigned __int128)b[j]
				+ (unsigned __int128)t[i + j] + cc;
			t[i + j] = (uint64_t)z;
			cc = (uint64_t)(z >> 64);
		}
		t[i + 4] = cc;
	}
	memcpy(d, t, sizeof t);
}

/*
 * This macro defines a function with prototype:
 *  static inline void name(uint64_t *a, const uint64_t *b, unsigned s)
 * It adds lshift(b,s) to a (if intrinsic_op is _addcarry_u64), or
 * subtracts lshift(b,s) from a (if intrinsic_op is _subborrow_u64).
 * Both values consist of 'size' limbs. Truncation happens if the result
 * does not fit in the output.
 */
#define DEF_OP_LSHIFT(name, intrinsic_op, size) \
static void \
name(uint64_t *a, const uint64_t *b, unsigned s) \
{ \
	uint64_t b2[(size)]; \
	unsigned char cc; \
	unsigned long long w, w2, e; \
	size_t i; \
 \
 	if (s >= 64) { \
		unsigned k; \
 \
		k = s >> 6; \
		s &= 63; \
		if (k >= (size)) { \
			return; \
		} \
		memset(b2, 0, k * sizeof b2[0]); \
		memcpy(b2 + k, b, ((size) - k) * sizeof b2[0]); \
		b = b2; \
	} \
	if (s == 0) { \
		cc = 0; \
		for (i = 0; i < (size); i ++) { \
			cc = intrinsic_op(cc, a[i], b[i], &w); \
			a[i] = w; \
		} \
	} else { \
		cc = 0; \
		e = 0; \
		for (i = 0; i < (size); i ++) { \
			w = b[i]; \
			cc = intrinsic_op(cc, a[i], (w << s) | e, &w2); \
			e = w >> (64 - s); \
			a[i] = w2; \
		} \
	} \
}

DEF_OP_LSHIFT(add_lshift_small, _addcarry_u64, 2)
DEF_OP_LSHIFT(sub_lshift_small, _subborrow_u64, 2)
DEF_OP_LSHIFT(add_lshift_large, _addcarry_u64, 8)
DEF_OP_LSHIFT(sub_lshift_large, _subborrow_u64, 8)

/* see inner.h */
void
curve9767_inner_reduce_basis_vartime(
	uint8_t *c0, uint8_t *c1, const curve9767_scalar *b)
{
	unsigned long long u0_0, u0_1;
	unsigned long long u1_0, u1_1;
	unsigned long long v0_0, v0_1;
	unsigned long long v1_0, v1_1;
	unsigned long long nu_0, nu_1, nu_2, nu_3, nu_4, nu_5, nu_6, nu_7;
	unsigned long long nv_0, nv_1, nv_2, nv_3, nv_4, nv_5, nv_6, nv_7;
	unsigned long long sp_0, sp_1, sp_2, sp_3, sp_4, sp_5, sp_6, sp_7;
	unsigned long long w;
	uint64_t tmp_b[4], tmp_l[8];
	uint16_t bw[17];
	unsigned char cc;

	static const uint64_t order_u64[] = {
		18100514074342678129ull,  3698157286099510427ull,
		11496333115051504685ull,  1017895979937685331ull
	};

	/*
	 * Normalize b to 0..n-1.
	 */
	scalar_normalize(bw, b->v.w16);
	to_small(tmp_b, bw);

	/*
	 * Init:
	 *   u = [n, 0]
	 *   v = [b, 1]
	 */
	u0_0 = order_u64[0];
	u0_1 = order_u64[1];
	u1_0 = 0;
	u1_1 = 0;
	v0_0 = tmp_b[0];
	v0_1 = tmp_b[1];
	v1_0 = 1;
	v1_1 = 0;

	/*
	 * nu = <u, u> = n^2
	 * nv = <v, v> = b^2 + 1
	 * sp = <u, v> = n*b
	 */
	nu_0 =  3754123157322411489ull;
	nu_1 = 10232387653326882741ull;
	nu_2 = 10179565171905379182ull;
	nu_3 = 16526651211446439755ull;
	nu_4 = 13834574781821822809ull;
	nu_5 = 11683723223577970406ull;
	nu_6 = 10927662901995248357ull;
	nu_7 =    56167756316952225ull;
	mul(tmp_l, tmp_b, tmp_b);
	cc = _addcarry_u64(0, tmp_l[0], 1, &nv_0);
	cc = _addcarry_u64(cc, tmp_l[1], 0, &nv_1);
	cc = _addcarry_u64(cc, tmp_l[2], 0, &nv_2);
	cc = _addcarry_u64(cc, tmp_l[3], 0, &nv_3);
	cc = _addcarry_u64(cc, tmp_l[4], 0, &nv_4);
	cc = _addcarry_u64(cc, tmp_l[5], 0, &nv_5);
	cc = _addcarry_u64(cc, tmp_l[6], 0, &nv_6);
	(void)_addcarry_u64(cc, tmp_l[7], 0, &nv_7);
	mul(tmp_l, order_u64, tmp_b);
	sp_0 = tmp_l[0];
	sp_1 = tmp_l[1];
	sp_2 = tmp_l[2];
	sp_3 = tmp_l[3];
	sp_4 = tmp_l[4];
	sp_5 = tmp_l[5];
	sp_6 = tmp_l[6];
	sp_7 = tmp_l[7];

	/*
	 * Algorithm:
	 *   - If nu < nv, Then:
	 *         swap(u, v)
	 *         swap(nu, nv)
	 *   - If bitlength(nv) <= target, Then:
	 *         c0 <- v0
	 *         c1 <- v1
	 *         return success
	 *   - Set s <- max(0, bitlength(sp) - bitlength(nv))
	 *   - If sp > 0, Then:
	 *         u <- u - lshift(v, s)
	 *         nu <- nu + lshift(nv, 2*s) - lshift(sp, s+1)
	 *         sp <- sp - lshift(nv, s)
	 *     Else:
	 *         u <- u + lshift(v, s)
	 *         nu <- nu + lshift(nv, 2*s) + lshift(sp, s+1)
	 *         sp <- sp + lshift(nv, s)
	 */
	for (;;) {
		unsigned s;
		unsigned long long m;
		unsigned bl_nv, bl_sp;

		/*
		 * If nu < nv, then swap(u,v) and swap(nu,nv).
		 */
		cc = _subborrow_u64(0, nu_0, nv_0, &w);
		cc = _subborrow_u64(cc, nu_1, nv_1, &w);
		cc = _subborrow_u64(cc, nu_2, nv_2, &w);
		cc = _subborrow_u64(cc, nu_3, nv_3, &w);
		cc = _subborrow_u64(cc, nu_4, nv_4, &w);
		cc = _subborrow_u64(cc, nu_5, nv_5, &w);
		cc = _subborrow_u64(cc, nu_6, nv_6, &w);
		cc = _subborrow_u64(cc, nu_7, nv_7, &w);
		m = -(unsigned long long)cc;

#define SWAPCOND(x, y, ctlmask)   do { \
		unsigned long long swap_mask = (ctlmask) & ((x) ^ (y)); \
		(x) ^= swap_mask; \
		(y) ^= swap_mask; \
	} while (0)

		SWAPCOND(u0_0, v0_0, m);
		SWAPCOND(u0_1, v0_1, m);
		SWAPCOND(u1_0, v1_0, m);
		SWAPCOND(u1_1, v1_1, m);
		SWAPCOND(nu_0, nv_0, m);
		SWAPCOND(nu_1, nv_1, m);
		SWAPCOND(nu_2, nv_2, m);
		SWAPCOND(nu_3, nv_3, m);
		SWAPCOND(nu_4, nv_4, m);
		SWAPCOND(nu_5, nv_5, m);
		SWAPCOND(nu_6, nv_6, m);
		SWAPCOND(nu_7, nv_7, m);

		/*
		 * u is now the largest vector; its square norm is nu.
		 * We know that:
		 *   N(u-v) = N(u) + N(v) - 2*<u,v>
		 *   N(u+v) = N(u) + N(v) + 2*<u,v>
		 * Since all squared norms are positive, and since
		 * N(u) >= N(v), it is guaranteed that |sp| <= nu.
		 * Thus, if nu fits on 6 words (including the sign bit),
		 * then we can jump to phase 2, where 'large' values use
		 * 6 words.
		 */
		if ((nu_6 | nu_7) == 0 && nu_5 <= 0x7FFFFFFFFFFFFFFFull) {
			break;
		}

#define BITLENGTH(size, bb)   do { \
		unsigned bitlength_acc = 8; \
		int bitlength_flag = 1; \
		unsigned long long bitlength_mask = -(bb ## 7 >> 63); \
		unsigned long long bitlength_top = 0; \
		unsigned long long bitlength_word; \
		bitlength_word = bb ## 7 ^ bitlength_mask; \
		bitlength_top |= \
			bitlength_word & -(unsigned long long)bitlength_flag; \
		bitlength_flag &= (bitlength_word == 0); \
		bitlength_word = bb ## 6 ^ bitlength_mask; \
		bitlength_top |= \
			bitlength_word & -(unsigned long long)bitlength_flag; \
		bitlength_acc -= bitlength_flag; \
		bitlength_flag &= (bitlength_word == 0); \
		bitlength_word = bb ## 5 ^ bitlength_mask; \
		bitlength_top |= \
			bitlength_word & -(unsigned long long)bitlength_flag; \
		bitlength_acc -= bitlength_flag; \
		bitlength_flag &= (bitlength_word == 0); \
		bitlength_word = bb ## 4 ^ bitlength_mask; \
		bitlength_top |= \
			bitlength_word & -(unsigned long long)bitlength_flag; \
		bitlength_acc -= bitlength_flag; \
		bitlength_flag &= (bitlength_word == 0); \
		bitlength_word = bb ## 3 ^ bitlength_mask; \
		bitlength_top |= \
			bitlength_word & -(unsigned long long)bitlength_flag; \
		bitlength_acc -= bitlength_flag; \
		bitlength_flag &= (bitlength_word == 0); \
		bitlength_word = bb ## 2 ^ bitlength_mask; \
		bitlength_top |= \
			bitlength_word & -(unsigned long long)bitlength_flag; \
		bitlength_acc -= bitlength_flag; \
		bitlength_flag &= (bitlength_word == 0); \
		bitlength_word = bb ## 1 ^ bitlength_mask; \
		bitlength_top |= \
			bitlength_word & -(unsigned long long)bitlength_flag; \
		bitlength_acc -= bitlength_flag; \
		bitlength_flag &= (bitlength_word == 0); \
		bitlength_word = bb ## 0 ^ bitlength_mask; \
		bitlength_top |= \
			bitlength_word & -(unsigned long long)bitlength_flag; \
		bitlength_acc -= bitlength_flag; \
		(size) = (bitlength_acc << 6) - _lzcnt_u64(bitlength_top); \
	} while (0)

		BITLENGTH(bl_nv, nv_);
		BITLENGTH(bl_sp, sp_);

		/*
		 * If v is small enough, return.
		 */
		if (bl_nv <= 253) {
			uint64_t tmp_v[4];

			tmp_v[0] = v0_0;
			tmp_v[1] = v0_1;
			encode_small(c0, 16, tmp_v);
			tmp_v[0] = v1_0;
			tmp_v[1] = v1_1;
			encode_small(c1, 16, tmp_v);
			return;
		}

		/*
		 * Compute shift amount.
		 */
		s = bl_sp - bl_nv;
		s &= ~-(s >> 31);

		/*
		 * It is very rare that s > 31. We handle it with some
		 * generic code; branch prediction will soon learn that
		 * this path is normally not taken.
		 */
		if (s > 31) {
			uint64_t tu0[2], tu1[2], tv0[2], tv1[2];
			uint64_t tnu[8], tnv[8], tsp[8];

			tu0[0] = u0_0;
			tu0[1] = u0_1;
			tu1[0] = u1_0;
			tu1[1] = u1_1;
			tv0[0] = v0_0;
			tv0[1] = v0_1;
			tv1[0] = v1_0;
			tv1[1] = v1_1;
			tnu[0] = nu_0;
			tnu[1] = nu_1;
			tnu[2] = nu_2;
			tnu[3] = nu_3;
			tnu[4] = nu_4;
			tnu[5] = nu_5;
			tnu[6] = nu_6;
			tnu[7] = nu_7;
			tnv[0] = nv_0;
			tnv[1] = nv_1;
			tnv[2] = nv_2;
			tnv[3] = nv_3;
			tnv[4] = nv_4;
			tnv[5] = nv_5;
			tnv[6] = nv_6;
			tnv[7] = nv_7;
			tsp[0] = sp_0;
			tsp[1] = sp_1;
			tsp[2] = sp_2;
			tsp[3] = sp_3;
			tsp[4] = sp_4;
			tsp[5] = sp_5;
			tsp[6] = sp_6;
			tsp[7] = sp_7;

			if ((sp_7 >> 63) == 0) {
				sub_lshift_small(tu0, tv0, s);
				sub_lshift_small(tu1, tv1, s);
				add_lshift_large(tnu, tnv, 2 * s);
				sub_lshift_large(tnu, tsp, s + 1);
				sub_lshift_large(tsp, tnv, s);
			} else {
				add_lshift_small(tu0, tv0, s);
				add_lshift_small(tu1, tv1, s);
				add_lshift_large(tnu, tnv, 2 * s);
				add_lshift_large(tnu, tsp, s + 1);
				add_lshift_large(tsp, tnv, s);
			}

			u0_0 = tu0[0];
			u0_1 = tu0[1];
			u1_0 = tu1[0];
			u1_1 = tu1[1];
			v0_0 = tv0[0];
			v0_1 = tv0[1];
			v1_0 = tv1[0];
			v1_1 = tv1[1];
			nu_0 = tnu[0];
			nu_1 = tnu[1];
			nu_2 = tnu[2];
			nu_3 = tnu[3];
			nu_4 = tnu[4];
			nu_5 = tnu[5];
			nu_6 = tnu[6];
			nu_7 = tnu[7];
			nv_0 = tnv[0];
			nv_1 = tnv[1];
			nv_2 = tnv[2];
			nv_3 = tnv[3];
			nv_4 = tnv[4];
			nv_5 = tnv[5];
			nv_6 = tnv[6];
			nv_7 = tnv[7];
			sp_0 = tsp[0];
			sp_1 = tsp[1];
			sp_2 = tsp[2];
			sp_3 = tsp[3];
			sp_4 = tsp[4];
			sp_5 = tsp[5];
			sp_6 = tsp[6];
			sp_7 = tsp[7];
			continue;
		}

		if ((sp_7 >> 63) == 0) {
			if (s == 0) {
				cc = _subborrow_u64(0, u0_0, v0_0, &u0_0);
				(void)_subborrow_u64(cc, u0_1, v0_1, &u0_1);

				cc = _subborrow_u64(0, u1_0, v1_0, &u1_0);
				(void)_subborrow_u64(cc, u1_1, v1_1, &u1_1);

				cc = _addcarry_u64(0, nu_0, nv_0, &nu_0);
				cc = _addcarry_u64(cc, nu_1, nv_1, &nu_1);
				cc = _addcarry_u64(cc, nu_2, nv_2, &nu_2);
				cc = _addcarry_u64(cc, nu_3, nv_3, &nu_3);
				cc = _addcarry_u64(cc, nu_4, nv_4, &nu_4);
				cc = _addcarry_u64(cc, nu_5, nv_5, &nu_5);
				cc = _addcarry_u64(cc, nu_6, nv_6, &nu_6);
				(void)_addcarry_u64(cc, nu_7, nv_7, &nu_7);

				cc = _subborrow_u64(0, nu_0,
					(sp_0 << 1), &nu_0);
				cc = _subborrow_u64(cc, nu_1,
					(sp_1 << 1) | (sp_0 >> 63), &nu_1);
				cc = _subborrow_u64(cc, nu_2,
					(sp_2 << 1) | (sp_1 >> 63), &nu_2);
				cc = _subborrow_u64(cc, nu_3,
					(sp_3 << 1) | (sp_2 >> 63), &nu_3);
				cc = _subborrow_u64(cc, nu_4,
					(sp_4 << 1) | (sp_3 >> 63), &nu_4);
				cc = _subborrow_u64(cc, nu_5,
					(sp_5 << 1) | (sp_4 >> 63), &nu_5);
				cc = _subborrow_u64(cc, nu_6,
					(sp_6 << 1) | (sp_5 >> 63), &nu_6);
				(void)_subborrow_u64(cc, nu_7,
					(sp_7 << 1) | (sp_6 >> 63), &nu_7);

				cc = _subborrow_u64(0, sp_0, nv_0, &sp_0);
				cc = _subborrow_u64(cc, sp_1, nv_1, &sp_1);
				cc = _subborrow_u64(cc, sp_2, nv_2, &sp_2);
				cc = _subborrow_u64(cc, sp_3, nv_3, &sp_3);
				cc = _subborrow_u64(cc, sp_4, nv_4, &sp_4);
				cc = _subborrow_u64(cc, sp_5, nv_5, &sp_5);
				cc = _subborrow_u64(cc, sp_6, nv_6, &sp_6);
				(void)_subborrow_u64(cc, sp_7, nv_7, &sp_7);
			} else {
				unsigned rs, s1, rs1, s2, rs2;

				rs = 64 - s;
				s1 = s + 1;
				rs1 = 64 - s1;
				s2 = s << 1;
				rs2 = 64 - s2;

				cc = _subborrow_u64(0, u0_0,
					(v0_0 << s), &u0_0);
				(void)_subborrow_u64(cc, u0_1,
					(v0_1 << s) | (v0_0 >> rs), &u0_1);

				cc = _subborrow_u64(0, u1_0,
					(v1_0 << s), &u1_0);
				(void)_subborrow_u64(cc, u1_1,
					(v1_1 << s) | (v1_0 >> rs), &u1_1);

				cc = _addcarry_u64(0, nu_0,
					(nv_0 << s2), &nu_0);
				cc = _addcarry_u64(cc, nu_1,
					(nv_1 << s2) | (nv_0 >> rs2), &nu_1);
				cc = _addcarry_u64(cc, nu_2,
					(nv_2 << s2) | (nv_1 >> rs2), &nu_2);
				cc = _addcarry_u64(cc, nu_3,
					(nv_3 << s2) | (nv_2 >> rs2), &nu_3);
				cc = _addcarry_u64(cc, nu_4,
					(nv_4 << s2) | (nv_3 >> rs2), &nu_4);
				cc = _addcarry_u64(cc, nu_5,
					(nv_5 << s2) | (nv_4 >> rs2), &nu_5);
				cc = _addcarry_u64(cc, nu_6,
					(nv_6 << s2) | (nv_5 >> rs2), &nu_6);
				(void)_addcarry_u64(cc, nu_7,
					(nv_7 << s2) | (nv_6 >> rs2), &nu_7);

				cc = _subborrow_u64(0, nu_0,
					(sp_0 << s1), &nu_0);
				cc = _subborrow_u64(cc, nu_1,
					(sp_1 << s1) | (sp_0 >> rs1), &nu_1);
				cc = _subborrow_u64(cc, nu_2,
					(sp_2 << s1) | (sp_1 >> rs1), &nu_2);
				cc = _subborrow_u64(cc, nu_3,
					(sp_3 << s1) | (sp_2 >> rs1), &nu_3);
				cc = _subborrow_u64(cc, nu_4,
					(sp_4 << s1) | (sp_3 >> rs1), &nu_4);
				cc = _subborrow_u64(cc, nu_5,
					(sp_5 << s1) | (sp_4 >> rs1), &nu_5);
				cc = _subborrow_u64(cc, nu_6,
					(sp_6 << s1) | (sp_5 >> rs1), &nu_6);
				(void)_subborrow_u64(cc, nu_7,
					(sp_7 << s1) | (sp_6 >> rs1), &nu_7);

				cc = _subborrow_u64(0, sp_0,
					(nv_0 << s), &sp_0);
				cc = _subborrow_u64(cc, sp_1,
					(nv_1 << s) | (nv_0 >> rs), &sp_1);
				cc = _subborrow_u64(cc, sp_2,
					(nv_2 << s) | (nv_1 >> rs), &sp_2);
				cc = _subborrow_u64(cc, sp_3,
					(nv_3 << s) | (nv_2 >> rs), &sp_3);
				cc = _subborrow_u64(cc, sp_4,
					(nv_4 << s) | (nv_3 >> rs), &sp_4);
				cc = _subborrow_u64(cc, sp_5,
					(nv_5 << s) | (nv_4 >> rs), &sp_5);
				cc = _subborrow_u64(cc, sp_6,
					(nv_6 << s) | (nv_5 >> rs), &sp_6);
				(void)_subborrow_u64(cc, sp_7,
					(nv_7 << s) | (nv_6 >> rs), &sp_7);
			}
		} else {
			if (s == 0) {
				cc = _addcarry_u64(0, u0_0, v0_0, &u0_0);
				(void)_addcarry_u64(cc, u0_1, v0_1, &u0_1);

				cc = _addcarry_u64(0, u1_0, v1_0, &u1_0);
				(void)_addcarry_u64(cc, u1_1, v1_1, &u1_1);

				cc = _addcarry_u64(0, nu_0, nv_0, &nu_0);
				cc = _addcarry_u64(cc, nu_1, nv_1, &nu_1);
				cc = _addcarry_u64(cc, nu_2, nv_2, &nu_2);
				cc = _addcarry_u64(cc, nu_3, nv_3, &nu_3);
				cc = _addcarry_u64(cc, nu_4, nv_4, &nu_4);
				cc = _addcarry_u64(cc, nu_5, nv_5, &nu_5);
				cc = _addcarry_u64(cc, nu_6, nv_6, &nu_6);
				(void)_addcarry_u64(cc, nu_7, nv_7, &nu_7);

				cc = _addcarry_u64(0, nu_0,
					(sp_0 << 1), &nu_0);
				cc = _addcarry_u64(cc, nu_1,
					(sp_1 << 1) | (sp_0 >> 63), &nu_1);
				cc = _addcarry_u64(cc, nu_2,
					(sp_2 << 1) | (sp_1 >> 63), &nu_2);
				cc = _addcarry_u64(cc, nu_3,
					(sp_3 << 1) | (sp_2 >> 63), &nu_3);
				cc = _addcarry_u64(cc, nu_4,
					(sp_4 << 1) | (sp_3 >> 63), &nu_4);
				cc = _addcarry_u64(cc, nu_5,
					(sp_5 << 1) | (sp_4 >> 63), &nu_5);
				cc = _addcarry_u64(cc, nu_6,
					(sp_6 << 1) | (sp_5 >> 63), &nu_6);
				(void)_addcarry_u64(cc, nu_7,
					(sp_7 << 1) | (sp_6 >> 63), &nu_7);

				cc = _addcarry_u64(0, sp_0, nv_0, &sp_0);
				cc = _addcarry_u64(cc, sp_1, nv_1, &sp_1);
				cc = _addcarry_u64(cc, sp_2, nv_2, &sp_2);
				cc = _addcarry_u64(cc, sp_3, nv_3, &sp_3);
				cc = _addcarry_u64(cc, sp_4, nv_4, &sp_4);
				cc = _addcarry_u64(cc, sp_5, nv_5, &sp_5);
				cc = _addcarry_u64(cc, sp_6, nv_6, &sp_6);
				(void)_addcarry_u64(cc, sp_7, nv_7, &sp_7);
			} else {
				unsigned rs, s1, rs1, s2, rs2;

				rs = 64 - s;
				s1 = s + 1;
				rs1 = 64 - s1;
				s2 = s << 1;
				rs2 = 64 - s2;

				cc = _addcarry_u64(0, u0_0,
					(v0_0 << s), &u0_0);
				(void)_addcarry_u64(cc, u0_1,
					(v0_1 << s) | (v0_0 >> rs), &u0_1);

				cc = _addcarry_u64(0, u1_0,
					(v1_0 << s), &u1_0);
				(void)_addcarry_u64(cc, u1_1,
					(v1_1 << s) | (v1_0 >> rs), &u1_1);

				cc = _addcarry_u64(0, nu_0,
					(nv_0 << s2), &nu_0);
				cc = _addcarry_u64(cc, nu_1,
					(nv_1 << s2) | (nv_0 >> rs2), &nu_1);
				cc = _addcarry_u64(cc, nu_2,
					(nv_2 << s2) | (nv_1 >> rs2), &nu_2);
				cc = _addcarry_u64(cc, nu_3,
					(nv_3 << s2) | (nv_2 >> rs2), &nu_3);
				cc = _addcarry_u64(cc, nu_4,
					(nv_4 << s2) | (nv_3 >> rs2), &nu_4);
				cc = _addcarry_u64(cc, nu_5,
					(nv_5 << s2) | (nv_4 >> rs2), &nu_5);
				cc = _addcarry_u64(cc, nu_6,
					(nv_6 << s2) | (nv_5 >> rs2), &nu_6);
				(void)_addcarry_u64(cc, nu_7,
					(nv_7 << s2) | (nv_6 >> rs2), &nu_7);

				cc = _addcarry_u64(0, nu_0,
					(sp_0 << s1), &nu_0);
				cc = _addcarry_u64(cc, nu_1,
					(sp_1 << s1) | (sp_0 >> rs1), &nu_1);
				cc = _addcarry_u64(cc, nu_2,
					(sp_2 << s1) | (sp_1 >> rs1), &nu_2);
				cc = _addcarry_u64(cc, nu_3,
					(sp_3 << s1) | (sp_2 >> rs1), &nu_3);
				cc = _addcarry_u64(cc, nu_4,
					(sp_4 << s1) | (sp_3 >> rs1), &nu_4);
				cc = _addcarry_u64(cc, nu_5,
					(sp_5 << s1) | (sp_4 >> rs1), &nu_5);
				cc = _addcarry_u64(cc, nu_6,
					(sp_6 << s1) | (sp_5 >> rs1), &nu_6);
				(void)_addcarry_u64(cc, nu_7,
					(sp_7 << s1) | (sp_6 >> rs1), &nu_7);

				cc = _addcarry_u64(0, sp_0,
					(nv_0 << s), &sp_0);
				cc = _addcarry_u64(cc, sp_1,
					(nv_1 << s) | (nv_0 >> rs), &sp_1);
				cc = _addcarry_u64(cc, sp_2,
					(nv_2 << s) | (nv_1 >> rs), &sp_2);
				cc = _addcarry_u64(cc, sp_3,
					(nv_3 << s) | (nv_2 >> rs), &sp_3);
				cc = _addcarry_u64(cc, sp_4,
					(nv_4 << s) | (nv_3 >> rs), &sp_4);
				cc = _addcarry_u64(cc, sp_5,
					(nv_5 << s) | (nv_4 >> rs), &sp_5);
				cc = _addcarry_u64(cc, sp_6,
					(nv_6 << s) | (nv_5 >> rs), &sp_6);
				(void)_addcarry_u64(cc, sp_7,
					(nv_7 << s) | (nv_6 >> rs), &sp_7);
			}
		}
	}

	/*
	 * The part below is reached when large values have shrunk to 6
	 * words.
	 */
	for (;;) {
		unsigned s;
		unsigned long long m;
		unsigned bl_nv, bl_sp;

		/*
		 * If nu < nv, then swap(u,v) and swap(nu,nv).
		 */
		cc = _subborrow_u64(0, nu_0, nv_0, &w);
		cc = _subborrow_u64(cc, nu_1, nv_1, &w);
		cc = _subborrow_u64(cc, nu_2, nv_2, &w);
		cc = _subborrow_u64(cc, nu_3, nv_3, &w);
		cc = _subborrow_u64(cc, nu_4, nv_4, &w);
		cc = _subborrow_u64(cc, nu_5, nv_5, &w);
		m = -(unsigned long long)cc;

		SWAPCOND(u0_0, v0_0, m);
		SWAPCOND(u0_1, v0_1, m);
		SWAPCOND(u1_0, v1_0, m);
		SWAPCOND(u1_1, v1_1, m);
		SWAPCOND(nu_0, nv_0, m);
		SWAPCOND(nu_1, nv_1, m);
		SWAPCOND(nu_2, nv_2, m);
		SWAPCOND(nu_3, nv_3, m);
		SWAPCOND(nu_4, nv_4, m);
		SWAPCOND(nu_5, nv_5, m);

#define BITLENGTH_SHRUNK(size, bb)   do { \
		unsigned bitlength_acc = 6; \
		int bitlength_flag = 1; \
		unsigned long long bitlength_mask = -(bb ## 5 >> 63); \
		unsigned long long bitlength_top = 0; \
		unsigned long long bitlength_word; \
		bitlength_word = bb ## 5 ^ bitlength_mask; \
		bitlength_top |= \
			bitlength_word & -(unsigned long long)bitlength_flag; \
		bitlength_flag &= (bitlength_word == 0); \
		bitlength_word = bb ## 4 ^ bitlength_mask; \
		bitlength_top |= \
			bitlength_word & -(unsigned long long)bitlength_flag; \
		bitlength_acc -= bitlength_flag; \
		bitlength_flag &= (bitlength_word == 0); \
		bitlength_word = bb ## 3 ^ bitlength_mask; \
		bitlength_top |= \
			bitlength_word & -(unsigned long long)bitlength_flag; \
		bitlength_acc -= bitlength_flag; \
		bitlength_flag &= (bitlength_word == 0); \
		bitlength_word = bb ## 2 ^ bitlength_mask; \
		bitlength_top |= \
			bitlength_word & -(unsigned long long)bitlength_flag; \
		bitlength_acc -= bitlength_flag; \
		bitlength_flag &= (bitlength_word == 0); \
		bitlength_word = bb ## 1 ^ bitlength_mask; \
		bitlength_top |= \
			bitlength_word & -(unsigned long long)bitlength_flag; \
		bitlength_acc -= bitlength_flag; \
		bitlength_flag &= (bitlength_word == 0); \
		bitlength_word = bb ## 0 ^ bitlength_mask; \
		bitlength_top |= \
			bitlength_word & -(unsigned long long)bitlength_flag; \
		bitlength_acc -= bitlength_flag; \
		(size) = (bitlength_acc << 6) - _lzcnt_u64(bitlength_top); \
	} while (0)

		BITLENGTH_SHRUNK(bl_nv, nv_);
		BITLENGTH_SHRUNK(bl_sp, sp_);

		/*
		 * If v is small enough, return.
		 */
		if (bl_nv <= 253) {
			uint64_t tmp_v[4];

			tmp_v[0] = v0_0;
			tmp_v[1] = v0_1;
			encode_small(c0, 16, tmp_v);
			tmp_v[0] = v1_0;
			tmp_v[1] = v1_1;
			encode_small(c1, 16, tmp_v);
			return;
		}

		/*
		 * Compute shift amount.
		 */
		s = bl_sp - bl_nv;
		s &= ~-(s >> 31);

		/*
		 * It is very rare that s > 31. We handle it with some
		 * generic code; branch prediction will soon learn that
		 * this path is normally not taken.
		 */
		if (s > 31) {
			uint64_t tu0[4], tu1[4], tv0[4], tv1[4];
			uint64_t tnu[8], tnv[8], tsp[8];

			tu0[0] = u0_0;
			tu0[1] = u0_1;
			tu1[0] = u1_0;
			tu1[1] = u1_1;
			tv0[0] = v0_0;
			tv0[1] = v0_1;
			tv1[0] = v1_0;
			tv1[1] = v1_1;
			tnu[0] = nu_0;
			tnu[1] = nu_1;
			tnu[2] = nu_2;
			tnu[3] = nu_3;
			tnu[4] = nu_4;
			tnu[5] = nu_5;
			tnu[6] = 0;
			tnu[7] = 0;
			tnv[0] = nv_0;
			tnv[1] = nv_1;
			tnv[2] = nv_2;
			tnv[3] = nv_3;
			tnv[4] = nv_4;
			tnv[5] = nv_5;
			tnv[6] = 0;
			tnv[7] = 0;
			tsp[0] = sp_0;
			tsp[1] = sp_1;
			tsp[2] = sp_2;
			tsp[3] = sp_3;
			tsp[4] = sp_4;
			tsp[5] = sp_5;
			tsp[6] = -(sp_5 >> 63);
			tsp[7] = -(sp_5 >> 63);

			if ((sp_7 >> 63) == 0) {
				sub_lshift_small(tu0, tv0, s);
				sub_lshift_small(tu1, tv1, s);
				add_lshift_large(tnu, tnv, 2 * s);
				sub_lshift_large(tnu, tsp, s + 1);
				sub_lshift_large(tsp, tnv, s);
			} else {
				add_lshift_small(tu0, tv0, s);
				add_lshift_small(tu1, tv1, s);
				add_lshift_large(tnu, tnv, 2 * s);
				add_lshift_large(tnu, tsp, s + 1);
				add_lshift_large(tsp, tnv, s);
			}

			u0_0 = tu0[0];
			u0_1 = tu0[1];
			u1_0 = tu1[0];
			u1_1 = tu1[1];
			v0_0 = tv0[0];
			v0_1 = tv0[1];
			v1_0 = tv1[0];
			v1_1 = tv1[1];
			nu_0 = tnu[0];
			nu_1 = tnu[1];
			nu_2 = tnu[2];
			nu_3 = tnu[3];
			nu_4 = tnu[4];
			nu_5 = tnu[5];
			nv_0 = tnv[0];
			nv_1 = tnv[1];
			nv_2 = tnv[2];
			nv_3 = tnv[3];
			nv_4 = tnv[4];
			nv_5 = tnv[5];
			sp_0 = tsp[0];
			sp_1 = tsp[1];
			sp_2 = tsp[2];
			sp_3 = tsp[3];
			sp_4 = tsp[4];
			sp_5 = tsp[5];
			continue;
		}

		if ((sp_5 >> 63) == 0) {
			if (s == 0) {
				cc = _subborrow_u64(0, u0_0, v0_0, &u0_0);
				(void)_subborrow_u64(cc, u0_1, v0_1, &u0_1);

				cc = _subborrow_u64(0, u1_0, v1_0, &u1_0);
				(void)_subborrow_u64(cc, u1_1, v1_1, &u1_1);

				cc = _addcarry_u64(0, nu_0, nv_0, &nu_0);
				cc = _addcarry_u64(cc, nu_1, nv_1, &nu_1);
				cc = _addcarry_u64(cc, nu_2, nv_2, &nu_2);
				cc = _addcarry_u64(cc, nu_3, nv_3, &nu_3);
				cc = _addcarry_u64(cc, nu_4, nv_4, &nu_4);
				(void)_addcarry_u64(cc, nu_5, nv_5, &nu_5);

				cc = _subborrow_u64(0, nu_0,
					(sp_0 << 1), &nu_0);
				cc = _subborrow_u64(cc, nu_1,
					(sp_1 << 1) | (sp_0 >> 63), &nu_1);
				cc = _subborrow_u64(cc, nu_2,
					(sp_2 << 1) | (sp_1 >> 63), &nu_2);
				cc = _subborrow_u64(cc, nu_3,
					(sp_3 << 1) | (sp_2 >> 63), &nu_3);
				cc = _subborrow_u64(cc, nu_4,
					(sp_4 << 1) | (sp_3 >> 63), &nu_4);
				(void)_subborrow_u64(cc, nu_5,
					(sp_5 << 1) | (sp_4 >> 63), &nu_5);

				cc = _subborrow_u64(0, sp_0, nv_0, &sp_0);
				cc = _subborrow_u64(cc, sp_1, nv_1, &sp_1);
				cc = _subborrow_u64(cc, sp_2, nv_2, &sp_2);
				cc = _subborrow_u64(cc, sp_3, nv_3, &sp_3);
				cc = _subborrow_u64(cc, sp_4, nv_4, &sp_4);
				(void)_subborrow_u64(cc, sp_5, nv_5, &sp_5);
			} else {
				unsigned rs, s1, rs1, s2, rs2;

				rs = 64 - s;
				s1 = s + 1;
				rs1 = 64 - s1;
				s2 = s << 1;
				rs2 = 64 - s2;

				cc = _subborrow_u64(0, u0_0,
					(v0_0 << s), &u0_0);
				(void)_subborrow_u64(cc, u0_1,
					(v0_1 << s) | (v0_0 >> rs), &u0_1);

				cc = _subborrow_u64(0, u1_0,
					(v1_0 << s), &u1_0);
				(void)_subborrow_u64(cc, u1_1,
					(v1_1 << s) | (v1_0 >> rs), &u1_1);

				cc = _addcarry_u64(0, nu_0,
					(nv_0 << s2), &nu_0);
				cc = _addcarry_u64(cc, nu_1,
					(nv_1 << s2) | (nv_0 >> rs2), &nu_1);
				cc = _addcarry_u64(cc, nu_2,
					(nv_2 << s2) | (nv_1 >> rs2), &nu_2);
				cc = _addcarry_u64(cc, nu_3,
					(nv_3 << s2) | (nv_2 >> rs2), &nu_3);
				cc = _addcarry_u64(cc, nu_4,
					(nv_4 << s2) | (nv_3 >> rs2), &nu_4);
				(void)_addcarry_u64(cc, nu_5,
					(nv_5 << s2) | (nv_4 >> rs2), &nu_5);

				cc = _subborrow_u64(0, nu_0,
					(sp_0 << s1), &nu_0);
				cc = _subborrow_u64(cc, nu_1,
					(sp_1 << s1) | (sp_0 >> rs1), &nu_1);
				cc = _subborrow_u64(cc, nu_2,
					(sp_2 << s1) | (sp_1 >> rs1), &nu_2);
				cc = _subborrow_u64(cc, nu_3,
					(sp_3 << s1) | (sp_2 >> rs1), &nu_3);
				cc = _subborrow_u64(cc, nu_4,
					(sp_4 << s1) | (sp_3 >> rs1), &nu_4);
				(void)_subborrow_u64(cc, nu_5,
					(sp_5 << s1) | (sp_4 >> rs1), &nu_5);

				cc = _subborrow_u64(0, sp_0,
					(nv_0 << s), &sp_0);
				cc = _subborrow_u64(cc, sp_1,
					(nv_1 << s) | (nv_0 >> rs), &sp_1);
				cc = _subborrow_u64(cc, sp_2,
					(nv_2 << s) | (nv_1 >> rs), &sp_2);
				cc = _subborrow_u64(cc, sp_3,
					(nv_3 << s) | (nv_2 >> rs), &sp_3);
				cc = _subborrow_u64(cc, sp_4,
					(nv_4 << s) | (nv_3 >> rs), &sp_4);
				(void)_subborrow_u64(cc, sp_5,
					(nv_5 << s) | (nv_4 >> rs), &sp_5);
			}
		} else {
			if (s == 0) {
				cc = _addcarry_u64(0, u0_0, v0_0, &u0_0);
				(void)_addcarry_u64(cc, u0_1, v0_1, &u0_1);

				cc = _addcarry_u64(0, u1_0, v1_0, &u1_0);
				(void)_addcarry_u64(cc, u1_1, v1_1, &u1_1);

				cc = _addcarry_u64(0, nu_0, nv_0, &nu_0);
				cc = _addcarry_u64(cc, nu_1, nv_1, &nu_1);
				cc = _addcarry_u64(cc, nu_2, nv_2, &nu_2);
				cc = _addcarry_u64(cc, nu_3, nv_3, &nu_3);
				cc = _addcarry_u64(cc, nu_4, nv_4, &nu_4);
				(void)_addcarry_u64(cc, nu_5, nv_5, &nu_5);

				cc = _addcarry_u64(0, nu_0,
					(sp_0 << 1), &nu_0);
				cc = _addcarry_u64(cc, nu_1,
					(sp_1 << 1) | (sp_0 >> 63), &nu_1);
				cc = _addcarry_u64(cc, nu_2,
					(sp_2 << 1) | (sp_1 >> 63), &nu_2);
				cc = _addcarry_u64(cc, nu_3,
					(sp_3 << 1) | (sp_2 >> 63), &nu_3);
				cc = _addcarry_u64(cc, nu_4,
					(sp_4 << 1) | (sp_3 >> 63), &nu_4);
				(void)_addcarry_u64(cc, nu_5,
					(sp_5 << 1) | (sp_4 >> 63), &nu_5);

				cc = _addcarry_u64(0, sp_0, nv_0, &sp_0);
				cc = _addcarry_u64(cc, sp_1, nv_1, &sp_1);
				cc = _addcarry_u64(cc, sp_2, nv_2, &sp_2);
				cc = _addcarry_u64(cc, sp_3, nv_3, &sp_3);
				cc = _addcarry_u64(cc, sp_4, nv_4, &sp_4);
				(void)_addcarry_u64(cc, sp_5, nv_5, &sp_5);
			} else {
				unsigned rs, s1, rs1, s2, rs2;

				rs = 64 - s;
				s1 = s + 1;
				rs1 = 64 - s1;
				s2 = s << 1;
				rs2 = 64 - s2;

				cc = _addcarry_u64(0, u0_0,
					(v0_0 << s), &u0_0);
				(void)_addcarry_u64(cc, u0_1,
					(v0_1 << s) | (v0_0 >> rs), &u0_1);

				cc = _addcarry_u64(0, u1_0,
					(v1_0 << s), &u1_0);
				(void)_addcarry_u64(cc, u1_1,
					(v1_1 << s) | (v1_0 >> rs), &u1_1);

				cc = _addcarry_u64(0, nu_0,
					(nv_0 << s2), &nu_0);
				cc = _addcarry_u64(cc, nu_1,
					(nv_1 << s2) | (nv_0 >> rs2), &nu_1);
				cc = _addcarry_u64(cc, nu_2,
					(nv_2 << s2) | (nv_1 >> rs2), &nu_2);
				cc = _addcarry_u64(cc, nu_3,
					(nv_3 << s2) | (nv_2 >> rs2), &nu_3);
				cc = _addcarry_u64(cc, nu_4,
					(nv_4 << s2) | (nv_3 >> rs2), &nu_4);
				(void)_addcarry_u64(cc, nu_5,
					(nv_5 << s2) | (nv_4 >> rs2), &nu_5);

				cc = _addcarry_u64(0, nu_0,
					(sp_0 << s1), &nu_0);
				cc = _addcarry_u64(cc, nu_1,
					(sp_1 << s1) | (sp_0 >> rs1), &nu_1);
				cc = _addcarry_u64(cc, nu_2,
					(sp_2 << s1) | (sp_1 >> rs1), &nu_2);
				cc = _addcarry_u64(cc, nu_3,
					(sp_3 << s1) | (sp_2 >> rs1), &nu_3);
				cc = _addcarry_u64(cc, nu_4,
					(sp_4 << s1) | (sp_3 >> rs1), &nu_4);
				(void)_addcarry_u64(cc, nu_5,
					(sp_5 << s1) | (sp_4 >> rs1), &nu_5);

				cc = _addcarry_u64(0, sp_0,
					(nv_0 << s), &sp_0);
				cc = _addcarry_u64(cc, sp_1,
					(nv_1 << s) | (nv_0 >> rs), &sp_1);
				cc = _addcarry_u64(cc, sp_2,
					(nv_2 << s) | (nv_1 >> rs), &sp_2);
				cc = _addcarry_u64(cc, sp_3,
					(nv_3 << s) | (nv_2 >> rs), &sp_3);
				cc = _addcarry_u64(cc, sp_4,
					(nv_4 << s) | (nv_3 >> rs), &sp_4);
				(void)_addcarry_u64(cc, sp_5,
					(nv_5 << s) | (nv_4 >> rs), &sp_5);
			}
		}
	}

#undef SWAPCOND
#undef BITLENGTH
#undef BITLENGTH_SHRUNK
}
