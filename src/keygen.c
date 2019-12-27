#include "inner.h"

#define DOM_KEYGEN   "curve9767-keygen:"

/* see curve9767.h */
void
curve9767_keygen(curve9767_scalar *s, uint8_t t[32], curve9767_point *Q,
	const void *seed, size_t seed_len)
{
	shake_context sc;
	uint8_t tmp[64];
	curve9767_scalar s2;

	shake_init(&sc, 256);
	shake_inject(&sc, DOM_KEYGEN, strlen(DOM_KEYGEN));
	shake_inject(&sc, seed, seed_len);
	shake_flip(&sc);

	shake_extract(&sc, tmp, 64);
	if (s == NULL && Q != NULL) {
		s = &s2;
	}
	if (s != NULL) {
		curve9767_scalar_decode_reduce(s, tmp, 64);
		curve9767_scalar_condcopy(s, &curve9767_scalar_one,
			curve9767_scalar_is_zero(s));
	}

	if (t != NULL) {
		shake_extract(&sc, t, 32);
	}

	if (Q != NULL) {
		curve9767_point_mulgen(Q, s);
	}
}
