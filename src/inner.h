#ifndef INNER_H__
#define INNER_H__

#include "curve9767.h"

/* ==================================================================== */
/*
 * Finite field functions (GF(9767^19)).
 *
 * Each field element is represented as an array of 19 uint16_t values.
 * Exact contents are implementation-specific.
 *
 * IMPORTANT RULES:
 *
 *  - All arrays must be 32-bit aligned (for architectures that enforce
 *    32-bit alignment), even though they only contain 16-bit values.
 *    The caller is expected to ensure alignment, e.g. with a union
 *    type. The type field_element below can be used to that effect,
 *    especially for local variables.
 *
 *  - For functions that receive field elements as parameters and return
 *    field elements, the destination array may be the same as any of
 *    the source arrays; but partial overlap is not supported.
 *
 *  - All the functions defined in this API are fully constant-time.
 *    This applies even when array contents are indeterminate (i.e.
 *    timing-based side channels will not reveal anything about source
 *    array contents even if they are not valid).
 *
 *  - For the "conditional functions" (*condneg(), *select3()), the
 *    control value MUST be in the defined range (e.g. 0, 1 or 2 for
 *    *select3()); no other value is supported.
 */

/*
 * Helper type that can be used to declare arrays for field elements with
 * 32-bit alignment.
 */
typedef union {
	uint16_t v[20];
	uint32_t w[10];
} field_element;

/*
 * Field element of value 0.
 */
extern const field_element curve9767_inner_gf_zero;

/*
 * Field element of value 1.
 */
extern const field_element curve9767_inner_gf_one;

/*
 * Compute c = a + b.
 */
void curve9767_inner_gf_add(uint16_t *c, const uint16_t *a, const uint16_t *b);

/*
 * Compute c = a - b.
 */
void curve9767_inner_gf_sub(uint16_t *c, const uint16_t *a, const uint16_t *b);

/*
 * Compute c = -a.
 */
void curve9767_inner_gf_neg(uint16_t *c, const uint16_t *a);

/*
 * Conditionally negate value c. If ctl == 1, c is negated; otherwise, it
 * is kept unmodified.
 */
void curve9767_inner_gf_condneg(uint16_t *c, uint32_t ctl);

/*
 * Compute c = a * b.
 */
void curve9767_inner_gf_mul(uint16_t *c, const uint16_t *a, const uint16_t *b);

/*
 * Compute c = a * a.
 * In general, curve_inner_gf_sqr(c, a) returns the same result as, but is
 * more efficient than, curve_inner_gf_mul(c, a, a).
 */
void curve9767_inner_gf_sqr(uint16_t *c, const uint16_t *a);

/*
 * Compute c = 1 / a.
 * If a is zero, then c is set to zero.
 */
void curve9767_inner_gf_inv(uint16_t *c, const uint16_t *a);

/*
 * Compute c = sqrt(a).
 *
 * Which of the two possible square roots is obtained is not specified.
 * If a is not a quadratic residue, then -a is a quadratic residue, and
 * this function writes into c one of the square roots of -a.
 *
 * Returned value is 1 if the input was a quadratic residue, 0 otherwise.
 * Zero is a quadratic residue.
 *
 * Special case: if c == NULL, then no square root is produced, but the
 * quadratic residue status is still computed and returned.
 */
uint32_t curve9767_inner_gf_sqrt(uint16_t *c, const uint16_t *a);

/*
 * Test for a quadratic residue. This is simply a wrapper macro for
 * curve9767_inner_gf_sqrt().
 */
#define curve9767_inner_gf_is_qr(a)   curve9767_inner_gf_sqrt(NULL, (a))

/*
 * Compute the cube root of a, result in c.
 * In the field, every element has a single cube root.
 */
void curve9767_inner_gf_cubert(uint16_t *c, const uint16_t *a);

/*
 * Tell whether a field element is "negative". We define a field element
 * to be negative if its most significant non-zero coefficient has value
 * (modulo p) in the (p+1)/2..(p-1) range. Zero is not negative. For
 * every non-zero element x, exactly one of x and -x is negative.
 *
 * Returned value is 1 if a is negative, 0 otherwise.
 */
uint32_t curve9767_inner_gf_is_neg(const uint16_t *a);

/*
 * Compare a with b for equality. Returned value is 1 if they are equal,
 * 0 otherwise.
 */
uint32_t curve9767_inner_gf_eq(const uint16_t *a, const uint16_t *b);

/*
 * Encode a field element into exactly 32 bytes. The two top bits of the
 * last byte (byte offset 31) are set to zero.
 */
void curve9767_inner_gf_encode(void *dst, const uint16_t *a);

/*
 * Decode a field element from bytes. Exactly 32 bytes are read. The two
 * top bits of the last byte (byte offset 31) are ignored. Returned
 * value is 1 on success, 0 on error. If an error is returned, then the
 * destination array contents are indeterminate.
 */
uint32_t curve9767_inner_gf_decode(uint16_t *c, const void *src);

/*
 * Create a field element from a sequence of 48 bytes. The 48 bytes
 * are interpreted as an integer x in the 0..2^384-1 range (little-endian
 * encoding). The field element coefficients c[i] (0 <= i <= 18) are
 * obtained as:
 *   c[i] = floor(x / (p^i)) mod p
 * If the 48 bytes are the output of a cryptographically secure hash
 * function or KDF, then the output will be selected among the field with
 * maximum bias less than 2^(-128).
 */
void curve9767_inner_gf_map_to_base(uint16_t *c, const void *src);

/* ==================================================================== */
/*
 * Curve9767 functions.
 *
 * Here are support functions that are not part of the API but allow
 * writing some of the public functions generically (i.e. shared between
 * core implementations).
 */

/*
 * Given a putative point coordinate X, compute the corresponding Y
 * coordinate. The rebuilt Y coordinate with have the same "sign" as
 * the provided "neg" value (0 or 1). This function returns 1 on success,
 * 0 on error. An error is reported if there is no curve point with the
 * provided X coordinate. When an error is reported, the contents of y[]
 * are indeterminate.
 */
uint32_t curve9767_inner_make_y(uint16_t *y, const uint16_t *x, uint32_t neg);

/*
 * The window contains the X and Y coordinates of eight points,
 * referenced by index (0 to 7); they are internally stored in an
 * implementation-specific format and order. The window does not keep
 * track of neutral points.
 */
typedef struct {
	field_element v[16];
} window_point8;

/*
 * Put the coordinates of point Q at index k in the window.
 */
void curve9767_inner_window_put(window_point8 *window,
	const curve9767_point *Q, uint32_t k);

/*
 * Constant-time window lookup. Index k must be between 0 and 7 (inclusive);
 * the corresponding window point coordinates are copied into the provided
 * Q structure.
 * CAUTION: the value of the Q->neutral flag is NOT set.
 */
void curve9767_inner_window_lookup(curve9767_point *Q,
	const window_point8 *window, uint32_t k);

/*
 * Precomputed windows for some multiples of the generator.
 *    G           curve9767_inner_window_G
 *    (2^64)*G    curve9767_inner_window_G64
 *    (2^128)*G   curve9767_inner_window_G128
 *    (2^192)*G   curve9767_inner_window_G192
 */
extern const window_point8 curve9767_inner_window_G;
extern const window_point8 curve9767_inner_window_G64;
extern const window_point8 curve9767_inner_window_G128;
extern const window_point8 curve9767_inner_window_G192;

/*
 * Apply Icart's map on an input field element u. Map is described in
 * section 2 of: https://eprint.iacr.org/2009/226
 *
 * Namely:
 *  - If u is zero, then Q is set to the neutral point.
 *  - If u is not zero, then Q is set to the point (x,y) where:
 *       v = (3*a - u^4) / (6*u)
 *       x = (v^2 - b - (u^6)/27)^(1/3) + (u^2)/3
 *       y = u*x + v
 *
 * IMPORTANT: array u[] MUST be disjoint from Q (e.g. it should not be
 * and alias for Q->x or Q->y).
 */
void curve9767_inner_Icart_map(curve9767_point *Q, const uint16_t *u);

/* ==================================================================== */

#endif
