#include "inner.h"

#define DOM_SIGN_K   "curve9767-sign-k:"
#define DOM_SIGN_E   "curve9767-sign-e:"

static void
make_k(curve9767_scalar *k, const uint8_t t[32],
	const char *hash_oid, const void *hv, size_t hv_len)
{
	shake_context sc;
	uint8_t tmp[64];

	shake_init(&sc, 256);
	shake_inject(&sc, DOM_SIGN_K, strlen(DOM_SIGN_K));
	shake_inject(&sc, t, 32);
	shake_inject(&sc, hash_oid, strlen(hash_oid));
	shake_inject(&sc, ":", 1);
	shake_inject(&sc, hv, hv_len);
	shake_flip(&sc);
	shake_extract(&sc, tmp, 64);
	curve9767_scalar_decode_reduce(k, tmp, 64);
	curve9767_scalar_condcopy(k, &curve9767_scalar_one,
		curve9767_scalar_is_zero(k));
}

static void
make_e(curve9767_scalar *e, const uint8_t c[32], const curve9767_point *Q,
	const char *hash_oid, const void *hv, size_t hv_len)
{
	shake_context sc;
	uint8_t tmp[64];

	shake_init(&sc, 256);
	shake_inject(&sc, DOM_SIGN_E, strlen(DOM_SIGN_E));
	shake_inject(&sc, c, 32);
	curve9767_point_encode(tmp, Q);
	shake_inject(&sc, tmp, 32);
	shake_inject(&sc, hash_oid, strlen(hash_oid));
	shake_inject(&sc, ":", 1);
	shake_inject(&sc, hv, hv_len);
	shake_flip(&sc);
	shake_extract(&sc, tmp, 64);
	curve9767_scalar_decode_reduce(e, tmp, 64);
}

/* see curve9767.h */
void
curve9767_sign_generate(void *sig,
	const curve9767_scalar *s, const uint8_t t[32],
	const curve9767_point *Q,
	const char *hash_oid, const void *hv, size_t hv_len)
{
	curve9767_scalar k, e;
	curve9767_point C;
	uint8_t tmp[64];

	make_k(&k, t, hash_oid, hv, hv_len);
	curve9767_point_mulgen(&C, &k);
	curve9767_point_encode(tmp, &C);
	make_e(&e, tmp, Q, hash_oid, hv, hv_len);
	curve9767_scalar_mul(&e, &e, s);
	curve9767_scalar_add(&e, &e, &k);
	curve9767_scalar_encode(tmp + 32, &e);
	memcpy(sig, tmp, 64);
}

/* see curve9767.h */
int
curve9767_sign_verify(const void *sig,
	const curve9767_point *Q,
	const char *hash_oid, const void *hv, size_t hv_len)
{
	curve9767_scalar d, e;
	curve9767_point C;
	uint32_t r, w;
	const uint8_t *buf;
	uint8_t tmp[32];
	int i;

	buf = sig;
	r = curve9767_scalar_decode_strict(&d, buf + 32, 32);
	make_e(&e, buf, Q, hash_oid, hv, hv_len);
	curve9767_scalar_neg(&e, &e);
	curve9767_point_mul_mulgen_add(&C, Q, &e, &d);
	curve9767_point_encode(tmp, &C);
	w = 0;
	for (i = 0; i < 32; i ++) {
		w |= tmp[i] ^ buf[i];
	}
	return r & ((w - 1) >> 31);
}
