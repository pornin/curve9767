#include "inner.h"

#define DOM_ECDH        "curve9767-ecdh:"
#define DOM_ECDH_FAIL   "curve9767-ecdh-failed:"

/* see curve9767.h */
void
curve9767_ecdh_keygen(curve9767_scalar *s, uint8_t encoded_Q[32],
	const void *seed, size_t seed_len)
{
	if (encoded_Q == NULL) {
		curve9767_keygen(s, NULL, NULL, seed, seed_len);
	} else {
		curve9767_point Q;

		curve9767_keygen(s, NULL, &Q, seed, seed_len);
		curve9767_point_encode(encoded_Q, &Q);
	}
}

/* see curve9767.h */
int
curve9767_ecdh_recv(void *shared_secret, size_t shared_secret_len,
	const curve9767_scalar *s, const uint8_t encoded_Q2[32])
{
	uint8_t pm[32], tmp[32];
	curve9767_point Q2;
	uint32_t r;
	shake_context sc;
	int i;

	/*
	 * Decode input point, do the point multiplication, and encode
	 * the result into the pre-master array.
	 */
	r = curve9767_point_decode(&Q2, encoded_Q2);
	curve9767_point_mul(&Q2, &Q2, s);
	curve9767_point_encode_X(pm, &Q2);

	/*
	 * Compute the alternate pre-master secret, to be used in case
	 * of failure (r == 0).
	 */
	curve9767_scalar_encode(tmp, s);
	shake_init(&sc, 256);
	shake_inject(&sc, DOM_ECDH_FAIL, strlen(DOM_ECDH_FAIL));
	shake_inject(&sc, tmp, 32);
	shake_inject(&sc, encoded_Q2, 32);
	shake_flip(&sc);
	shake_extract(&sc, tmp, 32);

	/*
	 * Replace the pre-master with the alternate one, if the point
	 * decoding process failed.
	 */
	for (i = 0; i < 32; i ++) {
		pm[i] ^= (uint8_t)((r - 1) & (pm[i] ^ tmp[i]));
	}

	/*
	 * Compute the shared secret.
	 */
	shake_init(&sc, 256);
	shake_inject(&sc, DOM_ECDH, strlen(DOM_ECDH));
	shake_inject(&sc, pm, 32);
	shake_flip(&sc);
	shake_extract(&sc, shared_secret, shared_secret_len);

	return (int)r;
}
