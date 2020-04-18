#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "inner.h"

static void
speed_inv(void)
{
	static const uint8_t bx[] = {
		0xE0, 0xE9, 0x54, 0x89, 0x0D, 0x2C, 0xE9, 0x4E,
		0x5E, 0x05, 0xB4, 0x81, 0x80, 0x02, 0x6F, 0xFB,
		0x2B, 0x49, 0x2C, 0x1D, 0x5D, 0x3C, 0x23, 0x26,
		0x6C, 0x4F, 0xC9, 0x6B, 0xE4, 0xBC, 0x9D, 0x13
	};

	field_element x, y;
	long j, num;

	curve9767_inner_gf_decode(x.v, bx);

	num = 1;
	for (;;) {
		clock_t begin, end;
		double tt;

		begin = clock();
		for (j = 0; j < num; j ++) {
			curve9767_inner_gf_inv(y.v, x.v);
		}
		end = clock();
		tt = (double)(end - begin) / CLOCKS_PER_SEC;
		if (tt >= 2.0) {
			printf("%13.2f inv/s\n", (double)num / tt);
			break;
		}
		if (tt >= 0.2) {
			num = (long)((double)num * (2.1 / tt));
		} else {
			num <<= 1;
		}
	}
}

static void
speed_sqrt(void)
{
	static const uint8_t bx[] = {
		0xE0, 0xE9, 0x54, 0x89, 0x0D, 0x2C, 0xE9, 0x4E,
		0x5E, 0x05, 0xB4, 0x81, 0x80, 0x02, 0x6F, 0xFB,
		0x2B, 0x49, 0x2C, 0x1D, 0x5D, 0x3C, 0x23, 0x26,
		0x6C, 0x4F, 0xC9, 0x6B, 0xE4, 0xBC, 0x9D, 0x13
	};

	field_element x, y;
	long j, num;

	curve9767_inner_gf_decode(x.v, bx);

	num = 1;
	for (;;) {
		clock_t begin, end;
		double tt;

		begin = clock();
		for (j = 0; j < num; j ++) {
			curve9767_inner_gf_sqrt(y.v, x.v);
		}
		end = clock();
		tt = (double)(end - begin) / CLOCKS_PER_SEC;
		if (tt >= 2.0) {
			printf("%13.2f sqrt/s\n", (double)num / tt);
			break;
		}
		if (tt >= 0.2) {
			num = (long)((double)num * (2.1 / tt));
		} else {
			num <<= 1;
		}
	}
}

static void
speed_test_qr(void)
{
	static const uint8_t bx[] = {
		0xE0, 0xE9, 0x54, 0x89, 0x0D, 0x2C, 0xE9, 0x4E,
		0x5E, 0x05, 0xB4, 0x81, 0x80, 0x02, 0x6F, 0xFB,
		0x2B, 0x49, 0x2C, 0x1D, 0x5D, 0x3C, 0x23, 0x26,
		0x6C, 0x4F, 0xC9, 0x6B, 0xE4, 0xBC, 0x9D, 0x13
	};

	field_element x;
	long j, num;

	curve9767_inner_gf_decode(x.v, bx);

	num = 1;
	for (;;) {
		clock_t begin, end;
		double tt;

		begin = clock();
		for (j = 0; j < num; j ++) {
			curve9767_inner_gf_sqrt(NULL, x.v);
		}
		end = clock();
		tt = (double)(end - begin) / CLOCKS_PER_SEC;
		if (tt >= 2.0) {
			printf("%13.2f test_qr/s\n", (double)num / tt);
			break;
		}
		if (tt >= 0.2) {
			num = (long)((double)num * (2.1 / tt));
		} else {
			num <<= 1;
		}
	}
}

static void
speed_cubert(void)
{
	static const uint8_t bx[] = {
		0xE0, 0xE9, 0x54, 0x89, 0x0D, 0x2C, 0xE9, 0x4E,
		0x5E, 0x05, 0xB4, 0x81, 0x80, 0x02, 0x6F, 0xFB,
		0x2B, 0x49, 0x2C, 0x1D, 0x5D, 0x3C, 0x23, 0x26,
		0x6C, 0x4F, 0xC9, 0x6B, 0xE4, 0xBC, 0x9D, 0x13
	};

	field_element x, y;
	long j, num;

	curve9767_inner_gf_decode(x.v, bx);

	num = 1;
	for (;;) {
		clock_t begin, end;
		double tt;

		begin = clock();
		for (j = 0; j < num; j ++) {
			curve9767_inner_gf_cubert(y.v, x.v);
		}
		end = clock();
		tt = (double)(end - begin) / CLOCKS_PER_SEC;
		if (tt >= 2.0) {
			printf("%13.2f cubert/s\n", (double)num / tt);
			break;
		}
		if (tt >= 0.2) {
			num = (long)((double)num * (2.1 / tt));
		} else {
			num <<= 1;
		}
	}
}

static void
speed_reduce_basis(void)
{
	curve9767_scalar b[200];
	uint8_t c0[16], c1[16];
	long i, j, num;
	shake_context sc;

	for (i = 0; i < 200; i ++) {
		uint8_t tmp[64];

		tmp[0] = (uint8_t)i;
		shake_init(&sc, 256);
		shake_inject(&sc, tmp, 1);
		shake_flip(&sc);
		shake_extract(&sc, tmp, sizeof tmp);
		curve9767_scalar_decode_reduce(&b[i], tmp, sizeof tmp);
	}

	num = 1;
	for (;;) {
		clock_t begin, end;
		double tt;

		begin = clock();
		for (j = 0; j < num; j ++) {
			for (i = 0; i < 200; i ++) {
				curve9767_inner_reduce_basis_vartime(
					c0, c1, &b[i]);
			}
		}
		end = clock();
		tt = (double)(end - begin) / CLOCKS_PER_SEC;
		if (tt >= 2.0) {
			printf("%13.2f reduce_basis/s\n",
				(double)(num * 200) / tt);
			break;
		}
		if (tt >= 0.2) {
			num = (long)((double)num * (2.1 / tt));
		} else {
			num <<= 1;
		}
	}
}

static void
speed_point_add(void)
{
	static const uint8_t bq[] = {
		0xE0, 0xE9, 0x54, 0x89, 0x0D, 0x2C, 0xE9, 0x4E,
		0x5E, 0x05, 0xB4, 0x81, 0x80, 0x02, 0x6F, 0xFB,
		0x2B, 0x49, 0x2C, 0x1D, 0x5D, 0x3C, 0x23, 0x26,
		0x6C, 0x4F, 0xC9, 0x6B, 0xE4, 0xBC, 0x9D, 0x13
	};

	curve9767_point Q1, Q2;
	long j, num;

	curve9767_point_decode(&Q1, bq);
	curve9767_point_add(&Q2, &Q1, &Q1);

	num = 1;
	for (;;) {
		clock_t begin, end;
		double tt;

		begin = clock();
		for (j = 0; j < num; j ++) {
			curve9767_point_add(&Q2, &Q1, &Q2);
		}
		end = clock();
		tt = (double)(end - begin) / CLOCKS_PER_SEC;
		if (tt >= 2.0) {
			printf("%13.2f point_add/s\n", (double)num / tt);
			break;
		}
		if (tt >= 0.2) {
			num = (long)((double)num * (2.1 / tt));
		} else {
			num <<= 1;
		}
	}
}

static void
speed_point_mul2k(int k)
{
	static const uint8_t bq[] = {
		0xE0, 0xE9, 0x54, 0x89, 0x0D, 0x2C, 0xE9, 0x4E,
		0x5E, 0x05, 0xB4, 0x81, 0x80, 0x02, 0x6F, 0xFB,
		0x2B, 0x49, 0x2C, 0x1D, 0x5D, 0x3C, 0x23, 0x26,
		0x6C, 0x4F, 0xC9, 0x6B, 0xE4, 0xBC, 0x9D, 0x13
	};

	curve9767_point Q1;
	long j, num;

	curve9767_point_decode(&Q1, bq);

	num = 1;
	for (;;) {
		clock_t begin, end;
		double tt;

		begin = clock();
		for (j = 0; j < num; j ++) {
			curve9767_point_mul2k(&Q1, &Q1, k);
		}
		end = clock();
		tt = (double)(end - begin) / CLOCKS_PER_SEC;
		if (tt >= 2.0) {
			printf("%13.2f point_mul2k(%d)/s\n",
				(double)num / tt, k);
			break;
		}
		if (tt >= 0.2) {
			num = (long)((double)num * (2.1 / tt));
		} else {
			num <<= 1;
		}
	}
}

static void
speed_point_decode(void)
{
	static const uint8_t bq[] = {
		0xE0, 0xE9, 0x54, 0x89, 0x0D, 0x2C, 0xE9, 0x4E,
		0x5E, 0x05, 0xB4, 0x81, 0x80, 0x02, 0x6F, 0xFB,
		0x2B, 0x49, 0x2C, 0x1D, 0x5D, 0x3C, 0x23, 0x26,
		0x6C, 0x4F, 0xC9, 0x6B, 0xE4, 0xBC, 0x9D, 0x13
	};

	curve9767_point Q1;
	long j, num;

	num = 1;
	for (;;) {
		clock_t begin, end;
		double tt;

		begin = clock();
		for (j = 0; j < num; j ++) {
			curve9767_point_decode(&Q1, bq);
		}
		end = clock();
		tt = (double)(end - begin) / CLOCKS_PER_SEC;
		if (tt >= 2.0) {
			printf("%13.2f point_decode/s\n", (double)num / tt);
			break;
		}
		if (tt >= 0.2) {
			num = (long)((double)num * (2.1 / tt));
		} else {
			num <<= 1;
		}
	}
}

static void
speed_point_encode(void)
{
	static const uint8_t bq[] = {
		0xE0, 0xE9, 0x54, 0x89, 0x0D, 0x2C, 0xE9, 0x4E,
		0x5E, 0x05, 0xB4, 0x81, 0x80, 0x02, 0x6F, 0xFB,
		0x2B, 0x49, 0x2C, 0x1D, 0x5D, 0x3C, 0x23, 0x26,
		0x6C, 0x4F, 0xC9, 0x6B, 0xE4, 0xBC, 0x9D, 0x13
	};

	curve9767_point Q1;
	uint8_t tmp[32];
	long j, num;

	curve9767_point_decode(&Q1, bq);

	num = 1;
	for (;;) {
		clock_t begin, end;
		double tt;

		begin = clock();
		for (j = 0; j < num; j ++) {
			curve9767_point_encode(tmp, &Q1);
		}
		end = clock();
		tt = (double)(end - begin) / CLOCKS_PER_SEC;
		if (tt >= 2.0) {
			printf("%13.2f point_encode/s\n", (double)num / tt);
			break;
		}
		if (tt >= 0.2) {
			num = (long)((double)num * (2.1 / tt));
		} else {
			num <<= 1;
		}
	}
}

static void
speed_map_to_field(void)
{
	uint8_t tmp[48];
	field_element c;
	long j, num;
	shake_context sc;

	shake_init(&sc, 256);
	memset(tmp, 0, sizeof tmp);
	shake_inject(&sc, tmp, sizeof tmp);
	shake_flip(&sc);
	shake_extract(&sc, tmp, sizeof tmp);

	num = 1;
	for (;;) {
		clock_t begin, end;
		double tt;

		begin = clock();
		for (j = 0; j < num; j ++) {
			curve9767_inner_gf_map_to_base(c.v, tmp);
		}
		end = clock();
		tt = (double)(end - begin) / CLOCKS_PER_SEC;
		if (tt >= 2.0) {
			printf("%13.2f map_to_field/s\n", (double)num / tt);
			break;
		}
		if (tt >= 0.2) {
			num = (long)((double)num * (2.1 / tt));
		} else {
			num <<= 1;
		}
	}
}

static void
speed_point_mul(void)
{
	static const uint8_t bq[] = {
		0xE0, 0xE9, 0x54, 0x89, 0x0D, 0x2C, 0xE9, 0x4E,
		0x5E, 0x05, 0xB4, 0x81, 0x80, 0x02, 0x6F, 0xFB,
		0x2B, 0x49, 0x2C, 0x1D, 0x5D, 0x3C, 0x23, 0x26,
		0x6C, 0x4F, 0xC9, 0x6B, 0xE4, 0xBC, 0x9D, 0x13
	};

	static const uint8_t bs[] = {
		0x38, 0x9E, 0x39, 0x77, 0xCE, 0x5A, 0x72, 0x23,
		0x0F, 0x42, 0x86, 0x6D, 0x12, 0xD8, 0x20, 0x7A,
		0x98, 0x2F, 0x3A, 0x9E, 0x69, 0x23, 0x8A, 0x40,
		0x75, 0x91, 0x73, 0x1D, 0x37, 0xF3, 0x7E, 0x0A
	};

	curve9767_point Q;
	curve9767_scalar s;
	long j, num;

	curve9767_point_decode(&Q, bq);
	curve9767_scalar_decode_strict(&s, bs, sizeof bs);

	num = 1;
	for (;;) {
		clock_t begin, end;
		double tt;

		begin = clock();
		for (j = 0; j < num; j ++) {
			curve9767_point_mul(&Q, &Q, &s);
		}
		end = clock();
		tt = (double)(end - begin) / CLOCKS_PER_SEC;
		if (tt >= 2.0) {
			printf("%13.2f point_mul/s\n", (double)num / tt);
			break;
		}
		if (tt >= 0.2) {
			num = (long)((double)num * (2.1 / tt));
		} else {
			num <<= 1;
		}
	}
}

static void
speed_point_mulgen(void)
{
	static const uint8_t bs[] = {
		0x38, 0x9E, 0x39, 0x77, 0xCE, 0x5A, 0x72, 0x23,
		0x0F, 0x42, 0x86, 0x6D, 0x12, 0xD8, 0x20, 0x7A,
		0x98, 0x2F, 0x3A, 0x9E, 0x69, 0x23, 0x8A, 0x40,
		0x75, 0x91, 0x73, 0x1D, 0x37, 0xF3, 0x7E, 0x0A
	};

	curve9767_point Q;
	curve9767_scalar s;
	long j, num;

	curve9767_scalar_decode_strict(&s, bs, sizeof bs);

	num = 1;
	for (;;) {
		clock_t begin, end;
		double tt;

		begin = clock();
		for (j = 0; j < num; j ++) {
			curve9767_point_mulgen(&Q, &s);
		}
		end = clock();
		tt = (double)(end - begin) / CLOCKS_PER_SEC;
		if (tt >= 2.0) {
			printf("%13.2f point_mulgen/s\n", (double)num / tt);
			break;
		}
		if (tt >= 0.2) {
			num = (long)((double)num * (2.1 / tt));
		} else {
			num <<= 1;
		}
	}
}

static void
speed_point_mul_mulgen_add(void)
{
	static const uint8_t bs[] = {
		0x38, 0x9E, 0x39, 0x77, 0xCE, 0x5A, 0x72, 0x23,
		0x0F, 0x42, 0x86, 0x6D, 0x12, 0xD8, 0x20, 0x7A,
		0x98, 0x2F, 0x3A, 0x9E, 0x69, 0x23, 0x8A, 0x40,
		0x75, 0x91, 0x73, 0x1D, 0x37, 0xF3, 0x7E, 0x0A
	};

	curve9767_point Q;
	curve9767_scalar s;
	long j, num;

	curve9767_scalar_decode_strict(&s, bs, sizeof bs);

	num = 1;
	for (;;) {
		clock_t begin, end;
		double tt;

		begin = clock();
		for (j = 0; j < num; j ++) {
			curve9767_point_mul_mulgen_add(&Q, &Q, &s, &s);
		}
		end = clock();
		tt = (double)(end - begin) / CLOCKS_PER_SEC;
		if (tt >= 2.0) {
			printf("%13.2f point_mul_mulgen_add/s\n",
				(double)num / tt);
			break;
		}
		if (tt >= 0.2) {
			num = (long)((double)num * (2.1 / tt));
		} else {
			num <<= 1;
		}
	}
}

static void
speed_ecdh_keygen(void)
{
	uint8_t seed[32];
	uint8_t encoded_Q[32];
	curve9767_scalar s;
	long j, num;

	memset(seed, 0, sizeof seed);

	num = 1;
	for (;;) {
		clock_t begin, end;
		double tt;

		begin = clock();
		for (j = 0; j < num; j ++) {
			curve9767_ecdh_keygen(&s, encoded_Q, seed, sizeof seed);
		}
		end = clock();
		tt = (double)(end - begin) / CLOCKS_PER_SEC;
		if (tt >= 2.0) {
			printf("%13.2f ecdh_keygen/s\n",
				(double)num / tt);
			break;
		}
		if (tt >= 0.2) {
			num = (long)((double)num * (2.1 / tt));
		} else {
			num <<= 1;
		}
	}
}

static void
speed_ecdh_recv(void)
{
	uint8_t seed[32];
	uint8_t encoded_Q[32];
	uint8_t shared_secret[32];
	curve9767_scalar s;
	long j, num;

	memset(seed, 0, sizeof seed);
	curve9767_ecdh_keygen(&s, encoded_Q, seed, sizeof seed);

	/*
	 * We use our public key as the key received from the peer (since
	 * the whole implementation is constant-time, it does not matter
	 * for benchmarks which point we are using).
	 */
	num = 1;
	for (;;) {
		clock_t begin, end;
		double tt;

		begin = clock();
		for (j = 0; j < num; j ++) {
			curve9767_ecdh_recv(
				shared_secret, sizeof shared_secret,
				&s, encoded_Q);
		}
		end = clock();
		tt = (double)(end - begin) / CLOCKS_PER_SEC;
		if (tt >= 2.0) {
			printf("%13.2f ecdh_recv/s\n",
				(double)num / tt);
			break;
		}
		if (tt >= 0.2) {
			num = (long)((double)num * (2.1 / tt));
		} else {
			num <<= 1;
		}
	}
}

static void
speed_sign(void)
{
	uint8_t seed[32];
	curve9767_point Q;
	uint8_t t[32];
	uint8_t hv[32];
	uint8_t sig[32];
	curve9767_scalar s;
	long j, num;

	memset(seed, 0, sizeof seed);
	curve9767_keygen(&s, t, &Q, seed, sizeof seed);
	memset(hv, 0, sizeof hv);

	/*
	 * We use an all-zero pseudo hash value as input message (it will
	 * get re-hashed internally for challenge generation, and the
	 * whole code is constant-time anyway).
	 */
	num = 1;
	for (;;) {
		clock_t begin, end;
		double tt;

		begin = clock();
		for (j = 0; j < num; j ++) {
			curve9767_sign_generate(sig, &s, t, &Q,
				CURVE9767_OID_SHA3_256, hv, sizeof hv);
		}
		end = clock();
		tt = (double)(end - begin) / CLOCKS_PER_SEC;
		if (tt >= 2.0) {
			printf("%13.2f sign/s\n",
				(double)num / tt);
			break;
		}
		if (tt >= 0.2) {
			num = (long)((double)num * (2.1 / tt));
		} else {
			num <<= 1;
		}
	}
}

static void
speed_verify(void)
{
	uint8_t seed[32];
	curve9767_point Q;
	curve9767_scalar s;
	uint8_t t[32];
	uint8_t sig[200][64];
	uint8_t hv[32];
	long i, j, num;

	memset(seed, 0, sizeof seed);
	curve9767_keygen(&s, t, &Q, seed, sizeof seed);
	memset(hv, 0, sizeof hv);
	for (i = 0; i < 200; i ++) {
		hv[0] = (uint8_t)i;
		curve9767_sign_generate(sig[i],
			&s, t, &Q, CURVE9767_OID_SHA3_256, hv, sizeof hv);
	}

	num = 1;
	for (;;) {
		clock_t begin, end;
		double tt;
		int r;

		r = 1;
		begin = clock();
		for (j = 0; j < num; j ++) {
			for (i = 0; i < 200; i ++) {
				hv[0] = (uint8_t)i;
				r &= curve9767_sign_verify(sig[i], &Q,
					CURVE9767_OID_SHA3_256, hv, sizeof hv);
			}
		}
		end = clock();
		if (r != 1) {
			fprintf(stderr, "ERR: not all verified\n");
			exit(EXIT_FAILURE);
		}
		tt = (double)(end - begin) / CLOCKS_PER_SEC;
		if (tt >= 2.0) {
			printf("%13.2f verify/s\n",
				(double)(num * 200) / tt);
			break;
		}
		if (tt >= 0.2) {
			num = (long)((double)num * (2.1 / tt));
		} else {
			num <<= 1;
		}
	}
}

static void
speed_verify_vartime(void)
{
	uint8_t seed[32];
	curve9767_point Q;
	curve9767_scalar s;
	uint8_t t[32];
	uint8_t sig[200][64];
	uint8_t hv[32];
	long i, j, num;

	memset(seed, 0, sizeof seed);
	curve9767_keygen(&s, t, &Q, seed, sizeof seed);
	memset(hv, 0, sizeof hv);
	for (i = 0; i < 200; i ++) {
		hv[0] = (uint8_t)i;
		curve9767_sign_generate(sig[i],
			&s, t, &Q, CURVE9767_OID_SHA3_256, hv, sizeof hv);
	}

	num = 1;
	for (;;) {
		clock_t begin, end;
		double tt;
		int r;

		r = 1;
		begin = clock();
		for (j = 0; j < num; j ++) {
			for (i = 0; i < 200; i ++) {
				hv[0] = (uint8_t)i;
				r &= curve9767_sign_verify_vartime(sig[i], &Q,
					CURVE9767_OID_SHA3_256, hv, sizeof hv);
			}
		}
		end = clock();
		if (r != 1) {
			fprintf(stderr, "ERR: not all verified\n");
			exit(EXIT_FAILURE);
		}
		tt = (double)(end - begin) / CLOCKS_PER_SEC;
		if (tt >= 2.0) {
			printf("%13.2f verify_vartime/s\n",
				(double)(num * 200) / tt);
			break;
		}
		if (tt >= 0.2) {
			num = (long)((double)num * (2.1 / tt));
		} else {
			num <<= 1;
		}
	}
}

int
main(void)
{
	speed_inv();
	speed_sqrt();
	speed_test_qr();
	speed_cubert();
	speed_reduce_basis();
	speed_point_add();
	speed_point_mul2k(1);
	speed_point_mul2k(2);
	speed_point_mul2k(3);
	speed_point_mul2k(4);
	speed_point_decode();
	speed_point_encode();
	speed_map_to_field();
	speed_point_mul();
	speed_point_mulgen();
	speed_point_mul_mulgen_add();
	speed_ecdh_keygen();
	speed_ecdh_recv();
	speed_sign();
	speed_verify();
	speed_verify_vartime();
	return 0;
}
