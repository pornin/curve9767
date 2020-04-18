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
