#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include <x86intrin.h>

#include "inner.h"

static void
warmup(void)
{
	/*
	 * Run some curve operations for a total time of at least 0.1 ms
	 * in order to ensure that the CPU frequency scaled up to nominal.
	 */
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
		if (tt >= 0.1) {
			break;
		}
		num <<= 1;
	}
}

/*
 * This dummy function performs the decoding and encoding to/from AVX2
 * registers, but with a quasi-trivial body (simple copy from input to
 * output). This mimics the overhead of a field inversion.
 */
void curve9767_inner_gf_dummy_1(uint16_t *c, const uint16_t *a);

/*
 * This dummy function performs the decoding and encoding to/from AVX2
 * registers, but with a quasi-trivial body (word-wise addition, no
 * reduction whatsoever). This mimics the overhead of a field
 * multiplication.
 */
void curve9767_inner_gf_dummy_2(uint16_t *c,
	const uint16_t *a, const uint16_t *b);

static void
speed_dummy_1(void)
{
	static const uint8_t bx[] = {
		0xE0, 0xE9, 0x54, 0x89, 0x0D, 0x2C, 0xE9, 0x4E,
		0x5E, 0x05, 0xB4, 0x81, 0x80, 0x02, 0x6F, 0xFB,
		0x2B, 0x49, 0x2C, 0x1D, 0x5D, 0x3C, 0x23, 0x26,
		0x6C, 0x4F, 0xC9, 0x6B, 0xE4, 0xBC, 0x9D, 0x13
	};

	field_element x, z;
	int i;
	int64_t best;

	curve9767_inner_gf_decode(x.v, bx);

	/*
	 * Some blank invocations to fill caches and train branch prediction.
	 */
	for (i = 0; i < 100; i ++) {
		curve9767_inner_gf_dummy_1(z.v, x.v);
	}

	best = INT64_MAX;
	for (i = 0; i < 100; i ++) {
		int64_t begin, end;

		_mm_lfence();
		begin = __rdtsc();
		curve9767_inner_gf_dummy_1(z.v, x.v);
		_mm_lfence();
		end = __rdtsc();
		end -= begin;
		if (end > 0 && end < best) {
			best = end;
		}
	}
	printf("gf_dummy_1             %10ld\n", (long)best);
}

static void
speed_dummy_2(void)
{
	static const uint8_t bx[] = {
		0xE0, 0xE9, 0x54, 0x89, 0x0D, 0x2C, 0xE9, 0x4E,
		0x5E, 0x05, 0xB4, 0x81, 0x80, 0x02, 0x6F, 0xFB,
		0x2B, 0x49, 0x2C, 0x1D, 0x5D, 0x3C, 0x23, 0x26,
		0x6C, 0x4F, 0xC9, 0x6B, 0xE4, 0xBC, 0x9D, 0x13
	};
	static const uint8_t by[] = {
		0x18, 0x94, 0x5A, 0x59, 0x7C, 0x2B, 0xF1, 0x98,
		0x02, 0xAC, 0xD0, 0x4C, 0xD5, 0x30, 0xF4, 0x24,
		0x9D, 0x11, 0xA5, 0x01, 0xBC, 0x4C, 0x18, 0x27,
		0x7E, 0xB8, 0x3B, 0x8F, 0x26, 0x60, 0xF4, 0x0D
	};

	field_element x, y, z;
	int i;
	int64_t best;

	curve9767_inner_gf_decode(x.v, bx);
	curve9767_inner_gf_decode(y.v, by);

	/*
	 * Some blank invocations to fill caches and train branch prediction.
	 */
	for (i = 0; i < 100; i ++) {
		curve9767_inner_gf_dummy_2(z.v, y.v, x.v);
	}

	best = INT64_MAX;
	for (i = 0; i < 100; i ++) {
		int64_t begin, end;

		_mm_lfence();
		begin = __rdtsc();
		curve9767_inner_gf_dummy_2(z.v, y.v, x.v);
		_mm_lfence();
		end = __rdtsc();
		end -= begin;
		if (end > 0 && end < best) {
			best = end;
		}
	}
	printf("gf_dummy_2             %10ld\n", (long)best);
}

static void
speed_mul(void)
{
	static const uint8_t bx[] = {
		0xE0, 0xE9, 0x54, 0x89, 0x0D, 0x2C, 0xE9, 0x4E,
		0x5E, 0x05, 0xB4, 0x81, 0x80, 0x02, 0x6F, 0xFB,
		0x2B, 0x49, 0x2C, 0x1D, 0x5D, 0x3C, 0x23, 0x26,
		0x6C, 0x4F, 0xC9, 0x6B, 0xE4, 0xBC, 0x9D, 0x13
	};
	static const uint8_t by[] = {
		0x18, 0x94, 0x5A, 0x59, 0x7C, 0x2B, 0xF1, 0x98,
		0x02, 0xAC, 0xD0, 0x4C, 0xD5, 0x30, 0xF4, 0x24,
		0x9D, 0x11, 0xA5, 0x01, 0xBC, 0x4C, 0x18, 0x27,
		0x7E, 0xB8, 0x3B, 0x8F, 0x26, 0x60, 0xF4, 0x0D
	};

	field_element x, y, z;
	int i;
	int64_t best;

	curve9767_inner_gf_decode(x.v, bx);
	curve9767_inner_gf_decode(y.v, by);

	/*
	 * Some blank invocations to fill caches and train branch prediction.
	 */
	for (i = 0; i < 100; i ++) {
		curve9767_inner_gf_mul(z.v, y.v, x.v);
	}

	best = INT64_MAX;
	for (i = 0; i < 100; i ++) {
		int64_t begin, end;

		_mm_lfence();
		begin = __rdtsc();
		curve9767_inner_gf_mul(z.v, y.v, x.v);
		_mm_lfence();
		end = __rdtsc();
		end -= begin;
		if (end > 0 && end < best) {
			best = end;
		}
	}
	printf("gf_mul                 %10ld\n", (long)best);
}

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
	int i;
	int64_t best;

	curve9767_inner_gf_decode(x.v, bx);

	/*
	 * Some blank invocations to fill caches and train branch prediction.
	 */
	for (i = 0; i < 100; i ++) {
		curve9767_inner_gf_inv(y.v, x.v);
	}

	best = INT64_MAX;
	for (i = 0; i < 100; i ++) {
		int64_t begin, end;

		_mm_lfence();
		begin = __rdtsc();
		curve9767_inner_gf_inv(y.v, x.v);
		_mm_lfence();
		end = __rdtsc();
		end -= begin;
		if (end > 0 && end < best) {
			best = end;
		}
	}
	printf("gf_inv                 %10ld\n", (long)best);
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
	int i;
	int64_t best;

	curve9767_inner_gf_decode(x.v, bx);

	/*
	 * Some blank invocations to fill caches and train branch prediction.
	 */
	for (i = 0; i < 100; i ++) {
		curve9767_inner_gf_sqrt(y.v, x.v);
	}

	best = INT64_MAX;
	for (i = 0; i < 100; i ++) {
		int64_t begin, end;

		_mm_lfence();
		begin = __rdtsc();
		curve9767_inner_gf_sqrt(y.v, x.v);
		_mm_lfence();
		end = __rdtsc();
		end -= begin;
		if (end > 0 && end < best) {
			best = end;
		}
	}
	printf("gf_sqrt                %10ld\n", (long)best);
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
	int i;
	int64_t best;

	curve9767_inner_gf_decode(x.v, bx);

	/*
	 * Some blank invocations to fill caches and train branch prediction.
	 */
	for (i = 0; i < 100; i ++) {
		curve9767_inner_gf_sqrt(NULL, x.v);
	}

	best = INT64_MAX;
	for (i = 0; i < 100; i ++) {
		int64_t begin, end;

		_mm_lfence();
		begin = __rdtsc();
		curve9767_inner_gf_sqrt(NULL, x.v);
		_mm_lfence();
		end = __rdtsc();
		end -= begin;
		if (end > 0 && end < best) {
			best = end;
		}
	}
	printf("gf_test_qr             %10ld\n", (long)best);
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
	int i;
	int64_t best;

	curve9767_inner_gf_decode(x.v, bx);

	/*
	 * Some blank invocations to fill caches and train branch prediction.
	 */
	for (i = 0; i < 100; i ++) {
		curve9767_inner_gf_cubert(y.v, x.v);
	}

	best = INT64_MAX;
	for (i = 0; i < 100; i ++) {
		int64_t begin, end;

		_mm_lfence();
		begin = __rdtsc();
		curve9767_inner_gf_cubert(y.v, x.v);
		_mm_lfence();
		end = __rdtsc();
		end -= begin;
		if (end > 0 && end < best) {
			best = end;
		}
	}
	printf("gf_cubert              %10ld\n", (long)best);
}

static int
cmp_int64(const void *v1, const void *v2)
{
	int64_t x1, x2;

	x1 = *(const int64_t *)v1;
	x2 = *(const int64_t *)v2;
	if (x1 < x2) {
		return -1;
	} else if (x1 == x2) {
		return 0;
	} else {
		return 1;
	}
}

static void
speed_reduce_basis(void)
{
	curve9767_scalar b[200];
	uint8_t c0[16], c1[16];
	int i, j;
	shake_context sc;
	double best;

	for (i = 0; i < 200; i ++) {
		uint8_t tmp[64];

		tmp[0] = (uint8_t)i;
		shake_init(&sc, 256);
		shake_inject(&sc, tmp, 1);
		shake_flip(&sc);
		shake_extract(&sc, tmp, sizeof tmp);
		curve9767_scalar_decode_reduce(&b[i], tmp, sizeof tmp);
	}

	best = -1.0;
	for (j = 0; j < 10; j ++) {
		int64_t tt[200], med, sum;
		int count;
		double avg;

		/* Some warm-up to exercise caches and branch prediction. */
		for (i = 0; i < 200; i ++) {
			curve9767_inner_reduce_basis_vartime(c0, c1, &b[i]);
		}

		/* Make 200 measures. */
		for (i = 0; i < 200; i ++) {
			int64_t begin, end;

			_mm_lfence();
			begin = __rdtsc();
			curve9767_inner_reduce_basis_vartime(c0, c1, &b[i]);
			_mm_lfence();
			end = __rdtsc();
			tt[i] = end - begin;
		}

		/*
		 * Find median time.
		 */
		qsort(tt, 200, sizeof(int64_t), cmp_int64);
		med = tt[100];

		/*
		 * Make average over all times which are within
		 * 0.05*med .. 20*med.
		 */
		count = 0;
		sum = 0;
		for (i = 0; i < 200; i ++) {
			if (tt[i] < 0 || 20 * tt[i] < med || 20 * med < tt[i]) {
				continue;
			}
			count ++;
			sum += tt[i];
		}
		avg = (double)sum / (double)count;
		if (best < 0.0 || best > avg) {
			best = avg;
		}
	}
	printf("reduce_basis (avg)     %13.2f\n", best);
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
	int i;
	int64_t best;

	curve9767_point_decode(&Q1, bq);
	curve9767_point_add(&Q2, &Q1, &Q1);

	/*
	 * Some blank invocations to fill caches and train branch prediction.
	 */
	for (i = 0; i < 100; i ++) {
		curve9767_point_add(&Q2, &Q1, &Q2);
	}

	best = INT64_MAX;
	for (i = 0; i < 100; i ++) {
		int64_t begin, end;

		_mm_lfence();
		begin = __rdtsc();
		curve9767_point_add(&Q2, &Q1, &Q2);
		_mm_lfence();
		end = __rdtsc();
		end -= begin;
		if (end > 0 && end < best) {
			best = end;
		}
	}
	printf("point_add              %10ld\n", (long)best);
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
	int i;
	int64_t best;

	curve9767_point_decode(&Q1, bq);

	/*
	 * Some blank invocations to fill caches and train branch prediction.
	 */
	for (i = 0; i < 100; i ++) {
		curve9767_point_mul2k(&Q1, &Q1, k);
	}

	best = INT64_MAX;
	for (i = 0; i < 100; i ++) {
		int64_t begin, end;

		_mm_lfence();
		begin = __rdtsc();
		curve9767_point_mul2k(&Q1, &Q1, k);
		_mm_lfence();
		end = __rdtsc();
		end -= begin;
		if (end > 0 && end < best) {
			best = end;
		}
	}
	printf("point_mul2k(%d)         %10ld\n", k, (long)best);
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
	int i;
	int64_t best;

	/*
	 * Some blank invocations to fill caches and train branch prediction.
	 */
	for (i = 0; i < 100; i ++) {
		curve9767_point_decode(&Q1, bq);
	}

	best = INT64_MAX;
	for (i = 0; i < 100; i ++) {
		int64_t begin, end;

		_mm_lfence();
		begin = __rdtsc();
		curve9767_point_decode(&Q1, bq);
		_mm_lfence();
		end = __rdtsc();
		end -= begin;
		if (end > 0 && end < best) {
			best = end;
		}
	}
	printf("point_decode           %10ld\n", (long)best);
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
	int i;
	int64_t best;

	curve9767_point_decode(&Q1, bq);

	/*
	 * Some blank invocations to fill caches and train branch prediction.
	 */
	for (i = 0; i < 100; i ++) {
		curve9767_point_encode(tmp, &Q1);
	}

	best = INT64_MAX;
	for (i = 0; i < 100; i ++) {
		int64_t begin, end;

		_mm_lfence();
		begin = __rdtsc();
		curve9767_point_encode(tmp, &Q1);
		_mm_lfence();
		end = __rdtsc();
		end -= begin;
		if (end > 0 && end < best) {
			best = end;
		}
	}
	printf("point_encode           %10ld\n", (long)best);
}

static void
speed_map_to_field(void)
{
	uint8_t tmp[48];
	field_element c;
	shake_context sc;
	int i;
	int64_t best;

	shake_init(&sc, 256);
	memset(tmp, 0, sizeof tmp);
	shake_inject(&sc, tmp, sizeof tmp);
	shake_flip(&sc);
	shake_extract(&sc, tmp, sizeof tmp);

	/*
	 * Some blank invocations to fill caches and train branch prediction.
	 */
	for (i = 0; i < 100; i ++) {
		curve9767_inner_gf_map_to_base(c.v, tmp);
	}

	best = INT64_MAX;
	for (i = 0; i < 100; i ++) {
		int64_t begin, end;

		_mm_lfence();
		begin = __rdtsc();
		curve9767_inner_gf_map_to_base(c.v, tmp);
		_mm_lfence();
		end = __rdtsc();
		end -= begin;
		if (end > 0 && end < best) {
			best = end;
		}
	}
	printf("map_to_field           %10ld\n", (long)best);
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
	int i;
	int64_t best;

	curve9767_point_decode(&Q, bq);
	curve9767_scalar_decode_strict(&s, bs, sizeof bs);

	/*
	 * Some blank invocations to fill caches and train branch prediction.
	 */
	for (i = 0; i < 100; i ++) {
		curve9767_point_mul(&Q, &Q, &s);
	}

	best = INT64_MAX;
	for (i = 0; i < 100; i ++) {
		int64_t begin, end;

		_mm_lfence();
		begin = __rdtsc();
		curve9767_point_mul(&Q, &Q, &s);
		_mm_lfence();
		end = __rdtsc();
		end -= begin;
		if (end > 0 && end < best) {
			best = end;
		}
	}
	printf("point_mul              %10ld\n", (long)best);
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
	int i;
	int64_t best;

	curve9767_scalar_decode_strict(&s, bs, sizeof bs);

	/*
	 * Some blank invocations to fill caches and train branch prediction.
	 */
	for (i = 0; i < 100; i ++) {
		curve9767_point_mulgen(&Q, &s);
	}

	best = INT64_MAX;
	for (i = 0; i < 100; i ++) {
		int64_t begin, end;

		_mm_lfence();
		begin = __rdtsc();
		curve9767_point_mulgen(&Q, &s);
		_mm_lfence();
		end = __rdtsc();
		end -= begin;
		if (end > 0 && end < best) {
			best = end;
		}
	}
	printf("point_mulgen           %10ld\n", (long)best);
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
	int i;
	int64_t best;

	curve9767_scalar_decode_strict(&s, bs, sizeof bs);
	curve9767_point_mulgen(&Q, &s);

	/*
	 * Some blank invocations to fill caches and train branch prediction.
	 */
	for (i = 0; i < 100; i ++) {
		curve9767_point_mul_mulgen_add(&Q, &Q, &s, &s);
	}

	best = INT64_MAX;
	for (i = 0; i < 100; i ++) {
		int64_t begin, end;

		_mm_lfence();
		begin = __rdtsc();
		curve9767_point_mul_mulgen_add(&Q, &Q, &s, &s);
		_mm_lfence();
		end = __rdtsc();
		end -= begin;
		if (end > 0 && end < best) {
			best = end;
		}
	}
	printf("point_mul_mulgen_add   %10ld\n", (long)best);
}

static void
speed_ecdh_keygen(void)
{
	uint8_t seed[32];
	uint8_t encoded_Q[32];
	curve9767_scalar s;
	int i;
	int64_t best;

	memset(seed, 0, sizeof seed);

	/*
	 * Some blank invocations to fill caches and train branch prediction.
	 */
	for (i = 0; i < 100; i ++) {
		curve9767_ecdh_keygen(&s, encoded_Q, seed, sizeof seed);
	}

	best = INT64_MAX;
	for (i = 0; i < 100; i ++) {
		int64_t begin, end;

		_mm_lfence();
		begin = __rdtsc();
		curve9767_ecdh_keygen(&s, encoded_Q, seed, sizeof seed);
		_mm_lfence();
		end = __rdtsc();
		end -= begin;
		if (end > 0 && end < best) {
			best = end;
		}
	}
	printf("ecdh_keygen            %10ld\n", (long)best);
}

static void
speed_ecdh_recv(void)
{
	uint8_t seed[32];
	uint8_t encoded_Q[32];
	uint8_t shared_secret[32];
	curve9767_scalar s;
	int i;
	int64_t best;

	memset(seed, 0, sizeof seed);
	curve9767_ecdh_keygen(&s, encoded_Q, seed, sizeof seed);

	/*
	 * We use our public key as the key received from the peer (since
	 * the whole implementation is constant-time, it does not matter
	 * for benchmarks which point we are using).
	 */

	/*
	 * Some blank invocations to fill caches and train branch prediction.
	 */
	for (i = 0; i < 100; i ++) {
		curve9767_ecdh_recv(shared_secret, sizeof shared_secret,
			&s, encoded_Q);
	}

	best = INT64_MAX;
	for (i = 0; i < 100; i ++) {
		int64_t begin, end;

		_mm_lfence();
		begin = __rdtsc();
		curve9767_ecdh_recv(shared_secret, sizeof shared_secret,
			&s, encoded_Q);
		_mm_lfence();
		end = __rdtsc();
		end -= begin;
		if (end > 0 && end < best) {
			best = end;
		}
	}
	printf("ecdh_recv              %10ld\n", (long)best);
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
	int i;
	int64_t best;

	memset(seed, 0, sizeof seed);
	curve9767_keygen(&s, t, &Q, seed, sizeof seed);
	memset(hv, 0, sizeof hv);

	/*
	 * We use an all-zero pseudo hash value as input message (it will
	 * get re-hashed internally for challenge generation, and the
	 * whole code is constant-time anyway).
	 */

	/*
	 * Some blank invocations to fill caches and train branch prediction.
	 */
	for (i = 0; i < 100; i ++) {
		curve9767_sign_generate(sig, &s, t, &Q,
			CURVE9767_OID_SHA3_256, hv, sizeof hv);
	}

	best = INT64_MAX;
	for (i = 0; i < 100; i ++) {
		int64_t begin, end;

		_mm_lfence();
		begin = __rdtsc();
		curve9767_sign_generate(sig, &s, t, &Q,
			CURVE9767_OID_SHA3_256, hv, sizeof hv);
		_mm_lfence();
		end = __rdtsc();
		end -= begin;
		if (end > 0 && end < best) {
			best = end;
		}
	}
	printf("sign_generate          %10ld\n", (long)best);
}

static void
speed_verify(void)
{
	uint8_t seed[32];
	curve9767_point Q;
	curve9767_scalar s;
	uint8_t t[32];
	uint8_t sig[64];
	uint8_t hv[32];
	int i;
	int64_t best;

	memset(seed, 0, sizeof seed);
	curve9767_keygen(&s, t, &Q, seed, sizeof seed);
	memset(hv, 0, sizeof hv);
	curve9767_sign_generate(sig,
		&s, t, &Q, CURVE9767_OID_SHA3_256, hv, sizeof hv);

	/*
	 * Some blank invocations to fill caches and train branch prediction.
	 */
	for (i = 0; i < 100; i ++) {
		curve9767_sign_verify(sig, &Q,
			CURVE9767_OID_SHA3_256, hv, sizeof hv);
	}

	best = INT64_MAX;
	for (i = 0; i < 100; i ++) {
		int64_t begin, end;

		_mm_lfence();
		begin = __rdtsc();
		curve9767_sign_verify(sig, &Q,
			CURVE9767_OID_SHA3_256, hv, sizeof hv);
		_mm_lfence();
		end = __rdtsc();
		end -= begin;
		if (end > 0 && end < best) {
			best = end;
		}
	}
	printf("sign_verify            %10ld\n", (long)best);
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
	int i, j;
	double best;

	memset(seed, 0, sizeof seed);
	curve9767_keygen(&s, t, &Q, seed, sizeof seed);
	memset(hv, 0, sizeof hv);
	for (i = 0; i < 200; i ++) {
		hv[0] = (uint8_t)i;
		curve9767_sign_generate(sig[i],
			&s, t, &Q, CURVE9767_OID_SHA3_256, hv, sizeof hv);
	}

	best = -1.0;
	for (j = 0; j < 10; j ++) {
		int64_t tt[200], med, sum;
		int count;
		double avg;

		/* Some warm-up to exercise caches and branch prediction. */
		for (i = 0; i < 200; i ++) {
			curve9767_sign_verify_vartime(sig[i], &Q,
				CURVE9767_OID_SHA3_256, hv, sizeof hv);
		}

		/* Make 200 measures. */
		for (i = 0; i < 200; i ++) {
			int64_t begin, end;

			_mm_lfence();
			begin = __rdtsc();
			curve9767_sign_verify_vartime(sig[i], &Q,
				CURVE9767_OID_SHA3_256, hv, sizeof hv);
			_mm_lfence();
			end = __rdtsc();
			tt[i] = end - begin;
		}

		/*
		 * Find median time.
		 */
		qsort(tt, 200, sizeof(int64_t), cmp_int64);
		med = tt[100];

		/*
		 * Make average over all times which are within
		 * 0.05*med .. 20*med.
		 */
		count = 0;
		sum = 0;
		for (i = 0; i < 200; i ++) {
			if (tt[i] < 0 || 20 * tt[i] < med || 20 * med < tt[i]) {
				continue;
			}
			count ++;
			sum += tt[i];
		}
		avg = (double)sum / (double)count;
		if (best < 0.0 || best > avg) {
			best = avg;
		}
	}
	printf("sign_verify_vartime (avg) %10.2f\n", best);
}

int
main(void)
{
	warmup();
	speed_dummy_1();
	speed_dummy_2();
	speed_mul();
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
	speed_point_mul2k(5);
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
