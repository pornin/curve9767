#ifndef CURVE9767_H__
#define CURVE9767_H__

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ===================================================================== */
/*
 * High-level operations use SHAKE; normally, we use SHAKE256 (since the
 * curve has "128-bit security", SHAKE128 should be fine, but for all
 * the input and output sizes involved here, there is no penalty to using
 * SHAKE256 instead, and the higher number is better for marketing
 * purposes).
 *
 * A generic SHA-3 and SHAKE implementation is provided; the API is
 * described in the sha3.h file.
 */

#include "sha3.h"

/* ===================================================================== */
/*
 * Scalar operations.
 *
 * A scalar is an integer modulo the curve order (about 2^251.82).
 * Private keys are scalars. Some basic operations are provided for
 * scalars; they are not thoroughly optimized, but the implementations
 * are constant-time.
 */

/*
 * In-memory structure for a scalar (integer modulo the curve order n).
 * Contents are opaque (don't access them directly; internal values
 * depend on the implementation).
 */
typedef struct {
	union {
		uint16_t w16[18];
		uint32_t w32[9];
	} v;
} curve9767_scalar;

/*
 * Decode a sequence of 'len' bytes into a scalar. The sequence of bytes
 * is interpreted in unsigned little-endian convention. If the value is
 * not lower than the curve order, then this function returns 0 and the
 * scalar contents are indeterminate. Otherwise, the function returns 1.
 */
uint32_t curve9767_scalar_decode_strict(curve9767_scalar *s,
	const void *src, size_t len);

/*
 * Decode a sequence of 'len' bytes into a scalar. The sequence of bytes
 * is interpreted in unsigned little-endian convention. It is automatically
 * reduced modulo the curve order. There is no limit to the source length.
 */
void curve9767_scalar_decode_reduce(curve9767_scalar *s,
	const void *src, size_t len);

/*
 * Encode a scalar into bytes. This yields exactly 32 bytes; the scalar
 * is encoded in unsigned little-endian convention (since scalars are
 * lower than 2^252, the top four bits of the 32nd byte are necessarily
 * equal to zero).
 */
void curve9767_scalar_encode(void *dst, const curve9767_scalar *s);

/*
 * Test whether a scalar is zero. Returned value is 1 if it is zero, 0
 * otherwise.
 */
int curve9767_scalar_is_zero(const curve9767_scalar *s);

/*
 * Compare two scalars together. Returned value is 1 if the scalars
 * are equal, 0 otherwise.
 */
int curve9767_scalar_eq(const curve9767_scalar *a, const curve9767_scalar *b);

/*
 * Add scalar a to scalar b, result in c. The c structure may be the
 * same as structure a or structure b (or both).
 */
void curve9767_scalar_add(curve9767_scalar *c,
	const curve9767_scalar *a, const curve9767_scalar *b);

/*
 * Subtract scalar b from scalar a, result in c. The c structure may be
 * the same as structure a or structure b (or both).
 */
void curve9767_scalar_sub(curve9767_scalar *c,
	const curve9767_scalar *a, const curve9767_scalar *b);

/*
 * Negate scalar a, result in c. The c structure may be the same as
 * structure a.
 */
void curve9767_scalar_neg(curve9767_scalar *c, const curve9767_scalar *a);

/*
 * Multiply scalars a and b, result in c. The c structure may be the
 * same as structure a or structure b (or both).
 */
void curve9767_scalar_mul(curve9767_scalar *c,
	const curve9767_scalar *a, const curve9767_scalar *b);

/*
 * Conditional copy of a scalar (constant-time). If ctl is 0, then d 
 * is unmodified; if ctl is 1, then d is set to the value of s.
 * ctl MUST be either 0 or 1.
 */
void curve9767_scalar_condcopy(curve9767_scalar *d,
	const curve9767_scalar *s, uint32_t ctl);

/*
 * Statically allocated constant scalars with values zero, one and 2^128.
 */
extern const curve9767_scalar curve9767_scalar_zero;
extern const curve9767_scalar curve9767_scalar_one;

/* ===================================================================== */
/*
 * Elliptic curve operations.
 */

/*
 * In-memory structure for a decoded curve point. Contents are opaque
 * (don't access them directly; internal values depend on the
 * implementation).
 */
typedef struct {
	uint32_t neutral;
	uint16_t x[19];
	uint16_t dummy1;  /* for alignment */
	uint16_t y[19];
	uint16_t dummy2;  /* for alignment */
} curve9767_point;

/*
 * Conventional generator for the curve.
 */
extern const curve9767_point curve9767_generator;

/*
 * Set a curve point to the point-at-infinity.
 */
static inline void
curve9767_point_set_neutral(curve9767_point *Q)
{
	/*
	 * The uint16_t and uint32_t types have exact width and no
	 * padding; thus, they always contain valid values in the C
	 * sense, i.e. values that can be read without triggering an
	 * undefined behaviour. However, some static analysis tool
	 * can still take issue about reading such values (these tools
	 * don't know that they will be ultimately ignored, since the
	 * point is taged as "neutral") and the memset() calls below
	 * will placate them.
	 */
	Q->neutral = 1;
	memset(Q->x, 0, sizeof Q->x);
	memset(Q->y, 0, sizeof Q->y);
}

/*
 * Test whether a curve point is the point-at-infinity. Returned value
 * is 1 if it is, 0 otherwise.
 */
static inline int
curve9767_point_is_neutral(const curve9767_point *Q)
{
	return (int)Q->neutral;
}

/*
 * Encode a curve point into a sequence of 32 bytes. If the source point
 * is the point-at-infinity, then the 32 bytes written in dst[] are
 * equal to 0xFF (except the 32nd byte, which is set of 0x7F), and the
 * function returns 0. Otherwise, the function returns 1. Note that the
 * output obtained for the point-at-infinity is not valid (it will be
 * rejected by curve9767_point_decode()). In all cases, the top bit
 * of the last byte is zero.
 *
 * This function is constant-time, including if the point is the
 * point-at-infinity.
 */
int curve9767_point_encode(void *dst, const curve9767_point *Q);

/*
 * Encode the X coordinate of a curve point into a sequence of 32 bytes.
 * This is equivalent to curve9767_point_encode(), except that Q and -Q
 * encode into the same sequence. If Q is the point-at-infinity, an
 * invalid encoding is obtained, with all bytes set to 0xFF except the
 * 32nd byte, which is set to 0x3F. This is meant to be used for ECDH.
 * In all cases, the top two bits of the last byte are zero.
 */
int curve9767_point_encode_X(void *dst, const curve9767_point *Q);

/*
 * Decode a curve point. The source array (src[]) must have length
 * exactly 32 bytes. Returned value is 1 on success, 0 on error. An
 * error is reported if the source point is not on the expected curve.
 * If an error is returned, then the destination structure contents are
 * set to the point-at-infinity. Successful decoding cannot yield the
 * point-at-infinity.
 *
 * This is constant-time, even if the source value is incorrect (i.e.
 * timing-based side channels do not leak information on the decoded
 * point, even if it is not on the curve).
 */
int curve9767_point_decode(curve9767_point *Q, const void *src);

/*
 * Point negation: set Q2 to -Q1. This is constant-time and works for
 * all points, including the point-at-infinity. Destination point Q2
 * is allowed to be the same structure as Q1.
 */
void curve9767_point_neg(curve9767_point *Q2, const curve9767_point *Q1);

/*
 * Point addition: set Q3 to the sum of Q1 and Q2. This is constant-time
 * and handles all edge cases (e.g. Q1 == Q2, Q1 == -Q2, Q1 == infinity,
 * Q2 == infinity). Destination point Q3 is allowed to be the same
 * structure as Q1, or Q2, or both.
 */
void curve9767_point_add(curve9767_point *Q3,
	const curve9767_point *Q1, const curve9767_point *Q2);

/*
 * Point subtraction: set Q3 to the sum of Q1 and -Q2. Like curve9767_add(),
 * this function is constant-time and handles all edge cases. Destination
 * point Q3 is allowed to be the same structure as Q1, or Q2, or both.
 */
void curve9767_point_sub(curve9767_point *Q3,
	const curve9767_point *Q1, const curve9767_point *Q2);

/*
 * Point negation: set Q3 to -Q1. This function is constant-time and handles
 * all edge cases. Destination point Q3 is allowed to be the same structure
 * as Q1.
 */
void curve9767_point_neg(curve9767_point *Q3, const curve9767_point *Q1);

/*
 * Point multi-doubling: set Q3 to 2^k times Q1. This is constant-time
 * with regard to Q3, but not to k, i.e. execution time depends on the
 * value of k, so this should be used only for situation where k is
 * non-secret. This function is potentially more efficient than calling
 * curve9767_point_add() k times. If k == 0 then Q3 is set to Q1.
 */
void curve9767_point_mul2k(curve9767_point *Q3,
	const curve9767_point *Q1, unsigned k);

/*
 * Point multiplication: multiply point Q1 by scalar s, result in Q3.
 * This is constant-time with regard to both Q1 and s. Scalar s may
 * contain zero, in which case the point-at-infinity is obtained. Base
 * point Q1 may be the point at infinity, in which case the
 * point-at-infinity is obtained. Destination point Q3 may be the same
 * structure as Q1.
 */
void curve9767_point_mul(curve9767_point *Q3, const curve9767_point *Q1,
	const curve9767_scalar *s);

/*
 * Generator multiplication: this is a special case of point
 * multiplication, in which the point to multiply is the conventional
 * generator. This is more efficient than curve9767_point_mul().
 */
void curve9767_point_mulgen(curve9767_point *Q3, const curve9767_scalar *s);

/*
 * Combined point multiplications: this sets Q3 to s1*Q1+s2*G, where G
 * is the curve generator. This is more efficient than calling
 * curve9767_point_mul() and curve9767_point_mulgen() separately and
 * adding the results together. Destination point Q3 may be the same
 * structure as Q1.
 *
 * This operation is typically used in verification of ECDSA or Schnorr
 * signatures, then on nominally public data that do not require
 * constant-time discipline. Nevertheless, this function is
 * constant-time with regard to Q1, s1 and s2.
 */
void curve9767_point_mul_mulgen_add(curve9767_point *Q3,
	const curve9767_point *Q1, const curve9767_scalar *s1,
	const curve9767_scalar *s2);

/*
 * Combined point verification: this functions verifies that:
 *   s1*Q1+s2*G = Q2
 * for the provided points Q1 and Q2, scalars s1 and s2, and the
 * curve generator G. THIS FUNCTION IS NOT CONSTANT-TIME; it
 * should be used only in situations where the source values (scalars
 * and points) are non-secret. This is typically the case of ECDSA or
 * Schnorr signature verification.
 *
 * This function uses reduction of a lattice basis in dimension two in
 * order to remove half of the point doublings; it is on average
 * substantially faster than curve9767_point_mul_mulgen_add().
 *
 * Returned value 1 on success (the equation holds), 0 on error (the
 * equation does not hold).
 */
int curve9767_point_verify_mul_mulgen_add_vartime(
	const curve9767_point *Q1, const curve9767_scalar *s1,
	const curve9767_scalar *s2, const curve9767_point *Q2);

/* ===================================================================== */
/*
 * High-level operations.
 */

/*
 * Hash-to-curve. The provided SHAKE context must have been initialized,
 * fed with the message to hash (including any relevant domain
 * separation string), and flipped (i.e. made ready for generating
 * output). Normally, SHAKE256 is used, but this function also works
 * with SHAKE128 and other SHAKE variants.
 *
 * Exactly 96 bytes are extracted from the SHAKE context.
 *
 * The point Q is set to the hash output. It is theoretically possible
 * that the point-at-infinity is obtained. However, probability is
 * very low (about 2^(-251.82)) and there is no known input that would
 * yield such an output.
 */
void curve9767_hash_to_curve(curve9767_point *Q, shake_context *sc);

/*
 * Generate a key pair from a seed. A private key consists of:
 *  - a secret scalar s
 *  - an additional secret t (used for signing) of size 32 bytes
 * The public key is the curve point Q = s*G.
 *
 * The generation process is deterministic: the concatenation of the
 * domain separation string "curve9767-keygen:" and the provided seed
 * is used as input to SHAKE256; then 96 bytes are output. The first
 * 64 bytes are reduced modulo the curve order: this yields the secret
 * scalar s. The last 32 bytes are the additional secret t.
 *
 * This function fills s, t and Q. If any of these pointers is NULL, then
 * the corresponding element is skipped (in particular, if Q == NULL, then
 * the point Q is not computed, which makes the process much faster).
 *
 * The source seed MUST have at least 128 bits of entropy. It is
 * RECOMMENDED to have a seed with 256 bits of entropy so as to make
 * key collisions utterly improbable even in contexts where very large
 * numbers of key pairs are generated. Note that these requirements are
 * on entropy (in a nutshell, "how much the key contents are unknown to
 * outsiders"), not on the seed length. Entropy and seed length are only
 * loosely correlated (an n-bit seed cannot contain more than n bits of
 * entropy, but it could contain much less). This API does not generate
 * seeds. On Unix-like systems (e.g. Linux or macOS), a cryptographically
 * strong seed value can be obtained by reading the special /dev/urandom
 * file, or by using the getrandom() (or getentropy()) system call.
 *
 * Edge case: if the secret scalar obtained above is exactly 0, then it
 * is replaced with 1. This has negligible probability of happening
 * (about 2^(-251.82)) and there is no known seed value that leads to
 * such an outcome. The automatic replacement of 0 by 1 guarantees that
 * the public key Q will not be the point-at-infinity.
 */
void curve9767_keygen(curve9767_scalar *s, uint8_t t[32], curve9767_point *Q,
	const void *seed, size_t seed_len);

/*
 * ECDH: two functions are used for an ECDH key exchange:
 *
 *  - curve9767_ecdh_keygen(): a simple wrapper around curve9767_keygen(),
 *    it creates the secret scalar s and the encoded public key from the
 *    provided seed. The encoded public key is to be sent to the peer.
 *
 *  - curve9767_ecdh_recv(): using the secret scalar s, and the encoded
 *    point from the peer, a shared secret is obtained. If the peer sent
 *    the (encoded) point Q2, then the following is performed:
 *     - peer point is decoded
 *     - s*Q2 is computed
 *     - the X coordinate of s*Q2 is encoded as 32 bytes
 *     - SHAKE256 is applied on the concatenation on the domain separation
 *       string "curve9767-ecdh:" and the encoded X coordinate; the output
 *       is the shared secret.
 *
 * Generated shared secret length is arbitrary, but SHOULD of course be
 * at least 128 bits (16 bytes) for proper security.
 */

/*
 * Generate an ECDH key pair from a seed. This computes the same scalars
 * and public keys as curve9767_keygen(). The "additional secret" for
 * signing is not generated. The public key (point Q) is encoded into
 * exactly 32 bytes.
 *
 * If encoded_Q is NULL, then the public point Q is not computed and
 * not returned.
 */
void curve9767_ecdh_keygen(curve9767_scalar *s, uint8_t encoded_Q[32],
	const void *seed, size_t seed_len);

/*
 * Compute the shared secret from the secret scalar s, and the point Q2
 * received from the peer (encoded as 32 bytes). On success, 1 is
 * returned. If the received encoded point Q2 is not valid, then this
 * function returns 0.
 *
 * When the input point is decoded successfully, the point s*Q is
 * computed, and its encoded X coordinate (32 bytes) as pre-master secret
 * (pm). If the input point is NOT decoded successfully, then the
 * pre-master is instead computed as the 32-byte output of SHAKE256
 * for, as input, the concatenation of:
 *  - the ASCII string "curve9767-ecdh-failed:"
 *  - the encoded scalar s (32 bytes)
 *  - the received encoded point (32 bytes, as received)
 * Either way, the shared secret is computed from the concatenation of:
 *  - the ASCII string "curve9767-ecdh:"
 *  - the pre-master secret (32 bytes)
 * This concatenation is used as input to SHAKE256, and the output is
 * the returned "shared secret".
 *
 * The use of a specific pre-master secret on shared input is meant to
 * hide to outsiders whether the received point was correct or not. This
 * can be useful in some protocols where ECDH points are sent already
 * encrypted in some way.
 *
 * This function is constant-time not only for all point computations,
 * but also for the outcome: outsiders should not be able to observe
 * whether the computation succeeded or not.
 */
int curve9767_ecdh_recv(void *shared_secret, size_t shared_secret_len,
	const curve9767_scalar *s, const uint8_t encoded_Q2[32]);

/*
 * Signatures:
 *
 * Signature generation and verification operates over a hashed message
 * (denoted hv, of length hv_len bytes). This hashing process is not
 * implemented by this API; it is supposed to be done byte the caller
 * (note: the sha3.h provides access to a SHA3 implementation).
 *
 * The used hash function is identified by a string which is used as
 * part of domain separation strings in hashing; i.e. the caller just
 * passes a constant literal string (see the CURVE9767_OID_* macros).
 * The identifiers for hash functions are derived from ASN.1 OID, thus
 * leveraging existing and future identifiers.
 *
 * Conceptually, hv[] could be the signed message itself. This would
 * mimic the "pure" mode of EdDSA (from RFC 8032). Such a mode avoids
 * a dependency on the collision resistance of the hash function, i.e.
 * it would be more robust against flaky hash functions such as MD5
 * or SHA-1. However, such a mode implies that the public key must
 * already be known at the time the message data starts being received,
 * which complicates things for memory-constrained streamed processing.
 * For instance, when validating an X.509 certificate chain received
 * from a TLS 1.2 server, each certificate is obtained in its entirety
 * before getting the public key to verify the signature on the
 * certificate (that public key is in the CA certificate, which comes
 * afterwards). With "pre-hashed" signatures, the certificate can be
 * hashed and decoded as it arrives, and the client does not need to
 * buffer it; but "pure" signatures break this workflow.
 *
 * For these reasons, pre-hashed mode is recommended, and no OID is
 * defined for "pure" signing.
 *
 * The default identifier string for a hash function is the decimal
 * string representation of its OID. For instance, SHA-256 is
 * "2.16.840.1.101.3.4.2.1".
 *
 * Signatures use the following process:
 *
 *  - Private key is a secret scalar s, and additional secret t (as
 *    produced by curve9767_keygen() from a seed). Public key is
 *    Q = s*G.
 *
 *  - To sign a (hashed) message hv with the secret key s.
 *
 *    1. Generate a random, uniform, non-zero scalar k. This can be
 *       done deterministically by first concatenating the following
 *       values:
 *        - the domain separation string "curve9767-sign-k:"
 *        - the additional secret t
 *        - the hash function identifier
 *        - the string ":" (one byte of value 0x3A)
 *        - the hashed message hv
 *       Then, use the concatenation as input to SHAKE256, and
 *       produce 512 bits (64 bytes). Finally, interpret these bytes
 *       as a big integer (unsigned little-endian convention) and
 *       reduce it modulo the curve order (this is what the
 *       curve9767_scalar_decode_reduce() computes).
 *
 *       If this process yields k == 0, replace it with 1. This happens
 *       with negligible probability (2^(-251.82)) and cannot be forced
 *       to happen with maliciously crafted inputs.
 *
 *       It is not required to follow this deterministic process;
 *       however, using it exactly guarantees that k is unbiased, and
 *       allows the function to be tested against known-answer tests.
 *       Conversely, a randomized process may be more robust against
 *       falut attacks (but then requires a source of randomness).
 *
 *    2. Compute the point C = k*G, and encode it as value c (32 bytes).
 *
 *    3. Concatenate the following values:
 *        - the domain separation string "curve9767-sign-e:"
 *        - the value c (encoding of point C over 32 bytes)
 *        - the encoding of the public key Q (32 bytes)
 *        - the hash function identifier
 *        - the string ":" (one byte of value 0x3A)
 *        - the (hashed) message hv
 *       Use the concatenation as input to SHAKE256, and produce 512 bits
 *       of output (64 bytes). This output is then reduced modulo the
 *       curve order to obtain the value e (again, this is what the
 *       curve9767_scalar_decode_reduce() function computes).
 *
 *       It is theoretically possible that e == 0. This is fine. This
 *       happens with negligible probability and nobody knows how to
 *       make such a case happen in practice.
 *
 *    4. Compute d = k + e*s (modulo the curve order).
 *
 *    5. Signature is the concatenation of c (32 bytes) and d (encoded
 *       over 32 bytes in unsigned little-endian convention).
 *
 *  - To verify a signature, given the signature (c,d), the hashed message
 *    hv, and the public key Q:
 *
 *    1. Decode d as an integer (unsigned little-endian convention).
 *       This integer must be lower than the curve order (otherwise, the
 *       signature is invalid).
 *
 *    2. Recompute the challenge value e as in step 3 of the signature
 *       generation process.
 *
 *    3. Compute C' = d*G - e*Q.
 *
 *    4. The signature is valid if and only if the encoding of C' is
 *       equal to c (the first half of the signature).
 *
 * Notes:
 *
 *  - The signer must know the secret scalar s and the corresponding
 *    public key Q; if deterministic generation of k is used, the
 *    additional secret t must also be known. All three values can be
 *    generated from the seed (with curve9767_keygen()), so only the
 *    seed needs to be stored. However, recomputing Q = s*G is expensive
 *    (on very small systems), and it may be worthwhile to store Q
 *    alongside the seed. Alternatively, s, Q (and optionally t) can
 *    be stored as secret key.
 *
 *  - Verification usually does not need to be constant-time. But this
 *    implementation is fully constant-time, including for signature
 *    verification.
 */

/* Hash function identifier: SHA-224 */
#define CURVE9767_OID_SHA224        "2.16.840.1.101.3.4.2.4"

/* Hash function identifier: SHA-256 */
#define CURVE9767_OID_SHA256        "2.16.840.1.101.3.4.2.1"

/* Hash function identifier: SHA-384 */
#define CURVE9767_OID_SHA384        "2.16.840.1.101.3.4.2.2"

/* Hash function identifier: SHA-512 */
#define CURVE9767_OID_SHA512        "2.16.840.1.101.3.4.2.3"

/* Hash function identifier: SHA-512-224 */
#define CURVE9767_OID_SHA512_224    "2.16.840.1.101.3.4.2.5"

/* Hash function identifier: SHA-512-256 */
#define CURVE9767_OID_SHA512_256    "2.16.840.1.101.3.4.2.6"

/* Hash function identifier: SHA3-224 */
#define CURVE9767_OID_SHA3_224      "2.16.840.1.101.3.4.2.7"

/* Hash function identifier: SHA3-256 */
#define CURVE9767_OID_SHA3_256      "2.16.840.1.101.3.4.2.8"

/* Hash function identifier: SHA3-384 */
#define CURVE9767_OID_SHA3_384      "2.16.840.1.101.3.4.2.9"

/* Hash function identifier: SHA3-512 */
#define CURVE9767_OID_SHA3_512      "2.16.840.1.101.3.4.2.10"

/*
 * Signature generation. Secret scalar s, additional secret t, and
 * public key Q = s*G are provided, as well as the hashed message
 * hv (of size hv_len bytes). Signature is written in sig[] and
 * has length exactly 64 bytes.
 *
 * This function implements the deterministic generation of k.
 */
void curve9767_sign_generate(void *sig,
	const curve9767_scalar *s, const uint8_t t[32],
	const curve9767_point *Q,
	const char *hash_oid, const void *hv, size_t hv_len);

/*
 * Signature verification. Signature value, public key Q, and
 * hashed message hv (of size hv_len bytes) are provided. The
 * signature is exactly 64 bytes in length. Returned value is 1
 * if the signature is correct, 0 otherwise.
 */
int curve9767_sign_verify(const void *sig,
	const curve9767_point *Q,
	const char *hash_oid, const void *hv, size_t hv_len);

/*
 * Signature verification, optimized function. Signature value, public
 * key Q, and hashed message hv (of size hv_len bytes) are provided. The
 * signature is exactly 64 bytes in length. Returned value is 1 if the
 * signature is correct, 0 otherwise. This function is faster than
 * curve9767_sign_verify().
 *
 * THIS FUNCTION IS NOT CONSTANT-TIME. In usual contexts, this is not a
 * problem, assuming that signature values, signed messages and public
 * keys are not secret information. If unsure, use
 * curve9767_sign_verify().
 */
int curve9767_sign_verify_vartime(const void *sig,
	const curve9767_point *Q,
	const char *hash_oid, const void *hv, size_t hv_len);

#endif
