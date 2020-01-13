#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/flash.h>

#include "inner.h"

static void
send_mult_chars(char c, size_t len)
{
	size_t u;

	for (u = 0; u < len; u ++) {
		usart_send_blocking(USART2, c);
	}
}

static void
send_string(const char *s, size_t len, size_t olen, char pad, int pad_left)
{
	size_t u;

	if (len < olen && pad_left) {
		send_mult_chars(pad, olen - len);
	}
	for (u = 0; u < len; u ++) {
		usart_send_blocking(USART2, s[u]);
	}
	if (len < olen && !pad_left) {
		send_mult_chars(pad, olen - len);
	}
}

static size_t
to_digits(char *buf, uint64_t x, unsigned base, int upper)
{
	size_t u, v;

	u = 0;
	while (x != 0) {
		unsigned d;

		d = (x % base);
		x /= base;
		if (d >= 10) {
			if (upper) {
				d += 'A' - 10;
			} else {
				d += 'a' - 10;
			}
		} else {
			d += '0';
		}
		buf[u ++] = d;
	}
	if (u == 0) {
		buf[u ++] = '0';
	}
	for (v = 0; (v + v) < u; v ++) {
		char t;

		t = buf[v];
		buf[v] = buf[u - 1 - v];
		buf[u - 1 - v] = t;
	}
	return u;
}

static void
send_uint64(uint64_t x, unsigned base, int upper,
	size_t olen, int pad0, int pad_left)
{
	char buf[65];
	size_t u;

	u = to_digits(buf, x, base, upper);
	send_string(buf, u, olen, pad0 ? '0' : ' ', pad_left);
}

static void
send_int64(int64_t x, unsigned base, int upper,
	size_t olen, int pad0, int pad_left)
{
	char buf[65];
	size_t u;

	if (x < 0) {
		u = to_digits(buf, -(uint64_t)x, base, upper);
		if (pad_left) {
			if (pad0) {
				usart_send_blocking(USART2, '-');
				if (olen > (u + 1)) {
					send_mult_chars('0', olen - (u + 1));
				}
			} else {
				if (olen > (u + 1)) {
					send_mult_chars(' ', olen - (u + 1));
				}
				usart_send_blocking(USART2, '-');
			}
			send_string(buf, u, 0, 0, 0);
		} else {
			usart_send_blocking(USART2, '-');
			if (olen > 0) {
				olen --;
			}
			send_string(buf, u, olen, ' ', 0);
		}
	} else {
		u = to_digits(buf, x, base, upper);
		send_string(buf, u, olen, pad0 ? '0' : ' ', pad_left);
	}
}

void
prf(const char *fmt, ...)
{
	va_list ap;
	const char *c;

	va_start(ap, fmt);
	c = fmt;
	for (;;) {
		int d;
		int pad0, wide, pad_left;
		unsigned olen;

		d = *c ++;
		if (d == 0) {
			break;
		}
		if (d != '%') {
			usart_send_blocking(USART2, d);
			continue;
		}
		d = *c ++;
		pad0 = 0;
		olen = 0;
		wide = 0;
		pad_left = 1;
		if (d == '-') {
			pad_left = 0;
			d = *c ++;
		}
		if (d >= '0' && d <= '9') {
			if (d == '0') {
				pad0 = 1;
			}
			while (d >= '0' && d <= '9') {
				olen = 10 * olen + (d - '0');
				d = *c ++;
			}
		}
		if (d == 'w') {
			wide = 1;
			d = *c ++;
		}
		if (pad0 && !pad_left) {
			pad0 = 0;
		}
		switch (d) {
		case 'd': {
			int64_t x;

			if (wide) {
				x = va_arg(ap, int64_t);
			} else {
				x = va_arg(ap, int32_t);
			}
			send_int64(x, 10, 0, olen, pad0, pad_left);
			break;
		}
		case 'u': {
			uint64_t x;

			if (wide) {
				x = va_arg(ap, uint64_t);
			} else {
				x = va_arg(ap, uint32_t);
			}
			send_uint64(x, 10, 0, olen, pad0, pad_left);
			break;
		}
		case 'x': {
			uint64_t x;

			if (wide) {
				x = va_arg(ap, uint64_t);
			} else {
				x = va_arg(ap, uint32_t);
			}
			send_uint64(x, 16, 0, olen, pad0, pad_left);
			break;
		}
		case 'X': {
			uint64_t x;

			if (wide) {
				x = va_arg(ap, uint64_t);
			} else {
				x = va_arg(ap, uint32_t);
			}
			send_uint64(x, 16, 1, olen, pad0, pad_left);
			break;
		}
		case 's': {
			const char *s;

			s = va_arg(ap, const char *);
			send_string(s, strlen(s), olen, ' ', pad_left);
			break;
		}
		case 0:
			break;
		default:
			usart_send_blocking(USART2, '%');
			break;
		}
		if (d == 0) {
			break;
		}
	}
	va_end(ap);
}

/* 24 MHz */
const struct rcc_clock_scale benchmarkclock = {
	.pllm = 8, //VCOin = HSE / PLLM = 1 MHz
	.plln = 192, //VCOout = VCOin * PLLN = 192 MHz
	.pllp = 8, //PLLCLK = VCOout / PLLP = 24 MHz (low to have 0WS)
	.pllq = 4, //PLL48CLK = VCOout / PLLQ = 48 MHz (required for USB, RNG)
	.pllr = 0,
	.pll_source = RCC_CFGR_PLLSRC_HSE_CLK,
	.hpre = RCC_CFGR_HPRE_DIV_NONE,
	.ppre1 = RCC_CFGR_PPRE_DIV_2,
	.ppre2 = RCC_CFGR_PPRE_DIV_NONE,
	.voltage_scale = PWR_SCALE1,
	.flash_config = FLASH_ACR_DCEN | FLASH_ACR_ICEN | FLASH_ACR_LATENCY_0WS,
	.ahb_frequency  = 24000000,
	.apb1_frequency = 12000000,
	.apb2_frequency = 24000000,
};

static void
enable_cyccnt(void)
{
	volatile uint32_t *DWT_CONTROL = (volatile uint32_t *)0xE0001000;
	volatile uint32_t *DWT_CYCCNT = (volatile uint32_t *)0xE0001004;
	volatile uint32_t *DEMCR = (volatile uint32_t *)0xE000EDFC;
	volatile uint32_t *LAR  = (volatile uint32_t *)0xE0001FB0;

	*DEMCR = *DEMCR | 0x01000000;
	*LAR = 0xC5ACCE55;
	*DWT_CYCCNT = 0;
	*DWT_CONTROL = *DWT_CONTROL | 1;
}

static inline uint32_t
get_system_ticks(void)
{
	return *(volatile uint32_t *)0xE0001004;
}

/*
 * Calibration functions defined in ops_cm4.s.
 */
void curve9767_inner_dummy(void);
void curve9767_inner_dummy_wrapper(void);

typedef void (*funbench)(void *, void *, void *,
	void *, void *, void *, void *);

uint32_t
do_benchmark(funbench fun, void *a0, void *a1, void *a2, void *a3,
	void *a4, void *a5, void *a6, int num)
{
	uint32_t t1, t2;
	int i;

	t1 = get_system_ticks();
	for (i = 0; i < num; i ++) {
		fun(a0, a1, a2, a3, a4, a5, a6);
	}
	t2 = get_system_ticks();
	return t2 - t1;
}

void
do_benchmark_fast(const char *name, funbench fun,
	void *a0, void *a1, void *a2, void *a3, void *a4, void *a5, void *a6)
{
	prf("%-25s", name);
	prf(" #1x: %10u", do_benchmark(fun, a0, a1, a2, a3, a4, a5, a6, 1));
	prf(" #10x: %10u", do_benchmark(fun, a0, a1, a2, a3, a4, a5, a6, 10));
	prf(" #100x: %10u", do_benchmark(fun, a0, a1, a2, a3, a4, a5, a6, 100));
	prf("\n");
}

void
do_benchmark_slow(const char *name, funbench fun,
	void *a0, void *a1, void *a2, void *a3, void *a4, void *a5, void *a6)
{
	prf("%-25s", name);
	prf(" #1x: %10u", do_benchmark(fun, a0, a1, a2, a3, a4, a5, a6, 1));
	prf(" #10x: %10u", do_benchmark(fun, a0, a1, a2, a3, a4, a5, a6, 10));
	prf("\n");
}

void
do_hash_to_curve(curve9767_point *Q, const void *data, size_t len)
{
	shake_context sc;

	shake_init(&sc, 256);
	shake_inject(&sc, data, len);
	shake_flip(&sc);
	curve9767_hash_to_curve(Q, &sc);
}

void
do_shake256(void *out, size_t out_len, const void *in, size_t in_len)
{
	shake_context sc;

	shake_init(&sc, 256);
	shake_inject(&sc, in, in_len);
	shake_flip(&sc);
	shake_extract(&sc, out, out_len);
}

int
main(void)
{
	field_element a, b, c;
	curve9767_point Q2;
	curve9767_scalar k;
	uint8_t bb[32];

	/* Setup clock. */
	rcc_clock_setup_pll(&benchmarkclock);

	/* Configure I/O. */
	rcc_periph_clock_enable(RCC_GPIOD);
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_USART2);

	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT,
		GPIO_PUPD_NONE, GPIO12 | GPIO13 | GPIO14 | GPIO15);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);
	gpio_set_af(GPIOA, GPIO_AF7, GPIO2);

	usart_set_baudrate(USART2, 115200);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_mode(USART2, USART_MODE_TX);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART2);

	/*
	 * Enable cycle counter.
	 */
	enable_cyccnt();

	prf("-----------------------------------------\n");
	prf("FLASH_ACR: %08X\n", *(volatile uint32_t *)0x40023C00);

	/*
	 * This can be used to enable/disable caches and/or prefetch.
	 *   0x0100   enable prefetch flag
	 *   0x0200   enable instruction cache flag
	 *   0x0400   enable data cache flag
	 */
	/*
	*(volatile uint32_t *)0x40023C00 &= (uint32_t)0x0000;
	// *(volatile uint32_t *)0x40023C00 |= (uint32_t)0x0700;
	prf("FLASH_ACR (2): %08X\n", *(volatile uint32_t *)0x40023C00);
	*/

	/*
	 * Q2 = k*G
	 */

	static const uint8_t bk[] = {
		0x38, 0x9E, 0x39, 0x77, 0xCE, 0x5A, 0x72, 0x23,
		0x0F, 0x42, 0x86, 0x6D, 0x12, 0xD8, 0x20, 0x7A,
		0x98, 0x2F, 0x3A, 0x9E, 0x69, 0x23, 0x8A, 0x40,
		0x75, 0x91, 0x73, 0x1D, 0x37, 0xF3, 0x7E, 0x0A
	};

	static const uint8_t bQ2[] = {
		0xE0, 0xE9, 0x54, 0x89, 0x0D, 0x2C, 0xE9, 0x4E,
		0x5E, 0x05, 0xB4, 0x81, 0x80, 0x02, 0x6F, 0xFB,
		0x2B, 0x49, 0x2C, 0x1D, 0x5D, 0x3C, 0x23, 0x26,
		0x6C, 0x4F, 0xC9, 0x6B, 0xE4, 0xBC, 0x9D, 0x13
	};

	static const uint8_t seed48[] = {
		0x3C, 0x35, 0xFC, 0x15, 0x4F, 0x95, 0x11, 0xC2,
		0x32, 0x42, 0xE1, 0xDB, 0xD5, 0xD8, 0x6F, 0x3B,
		0x0D, 0x1E, 0xAE, 0x67, 0xD7, 0x77, 0x30, 0x9D,
		0x22, 0x22, 0xBC, 0x73, 0x18, 0xBB, 0xC5, 0xFF,
		0xD1, 0x31, 0xB0, 0x5C, 0x25, 0xF0, 0xB7, 0x48,
		0xCF, 0xD5, 0xC9, 0x38, 0xC0, 0x2E, 0xA1, 0x83
	};

	memcpy(&a, &curve9767_inner_gf_one, sizeof a);
	memcpy(&b, &curve9767_inner_gf_one, sizeof b);
	memcpy(&c, &curve9767_inner_gf_one, sizeof c);

	prf("--------------------------------------------------------\n");
	prf("self-test:\n");
	curve9767_scalar_decode_strict(&k, bk, sizeof bk);
	memcpy(&Q2, &curve9767_generator, sizeof Q2);
	curve9767_point_mul(&Q2, &Q2, &k);
	curve9767_point_encode(bb, &Q2);
	prf("mul OK: %u\n", memcmp(bb, bQ2, sizeof bb) == 0);
	memset(&Q2, 0, sizeof Q2);
	memset(bb, 0, sizeof bb);
	curve9767_point_mulgen(&Q2, &k);
	curve9767_point_encode(bb, &Q2);
	prf("mulgen OK: %u\n", memcmp(bb, bQ2, sizeof bb) == 0);
	prf("\n");

	do_benchmark_fast("SHAKE256 48 -> 32",
		(funbench)&do_shake256, bb, (void *)sizeof bb,
		(void *)seed48, (void *)sizeof seed48, 0, 0, 0);

	do_benchmark_fast("dummy",
		(funbench)&curve9767_inner_dummy, 0, 0, 0, 0, 0, 0, 0);
	do_benchmark_fast("dummy_wrapper",
		(funbench)&curve9767_inner_dummy_wrapper, 0, 0, 0, 0, 0, 0, 0);
	do_benchmark_fast("gf_mul",
		(funbench)&curve9767_inner_gf_mul, &a, &b, &c, 0, 0, 0, 0);
	do_benchmark_fast("gf_sqr",
		(funbench)&curve9767_inner_gf_sqr, &a, &b, 0, 0, 0, 0, 0);
	do_benchmark_fast("gf_inv",
		(funbench)&curve9767_inner_gf_inv, &a, &b, 0, 0, 0, 0, 0);
	do_benchmark_fast("gf_sqrt",
		(funbench)&curve9767_inner_gf_sqrt, &a, &b, 0, 0, 0, 0, 0);
	do_benchmark_fast("gf_test_QR",
		(funbench)&curve9767_inner_gf_sqrt, 0, &b, 0, 0, 0, 0, 0);
	do_benchmark_fast("gf_cubert",
		(funbench)&curve9767_inner_gf_cubert, &a, &b, 0, 0, 0, 0, 0);

	memcpy(&Q2, &curve9767_generator, sizeof Q2);
	do_benchmark_fast("point_add",
		(funbench)&curve9767_point_add, &Q2, &Q2, &Q2, 0, 0, 0, 0);
	do_benchmark_fast("point_mul2",
		(funbench)&curve9767_point_mul2k,
		&Q2, &Q2, (void *)1, 0, 0, 0, 0);
	do_benchmark_fast("point_mul4",
		(funbench)&curve9767_point_mul2k,
		&Q2, &Q2, (void *)2, 0, 0, 0, 0);
	do_benchmark_fast("point_mul8",
		(funbench)&curve9767_point_mul2k,
		&Q2, &Q2, (void *)3, 0, 0, 0, 0);
	do_benchmark_fast("point_mul16",
		(funbench)&curve9767_point_mul2k,
		&Q2, &Q2, (void *)4, 0, 0, 0, 0);

	do_benchmark_fast("point_decode",
		(funbench)&curve9767_point_decode,
		&Q2, (void *)bQ2, 0, 0, 0, 0, 0);
	do_benchmark_fast("point_encode",
		(funbench)&curve9767_point_encode, bb, &Q2, 0, 0, 0, 0, 0);

	do_benchmark_slow("point_mul",
		(funbench)&curve9767_point_mul, &Q2, &Q2, &k, 0, 0, 0, 0);
	do_benchmark_slow("point_mulgen",
		(funbench)&curve9767_point_mulgen, &Q2, &k, 0, 0, 0, 0, 0);
	do_benchmark_slow("point_mul_mulgen_add",
		(funbench)&curve9767_point_mul_mulgen_add,
		&Q2, &Q2, &k, &k, 0, 0, 0);

	do_benchmark_fast("map_to_base",
		(funbench)&curve9767_inner_gf_map_to_base,
		&a, (void *)seed48, 0, 0, 0, 0, 0);
	do_benchmark_fast("Icart_map",
		(funbench)&curve9767_inner_Icart_map, &Q2, &a, 0, 0, 0, 0, 0);
	do_benchmark_fast("window_lookup",
		(funbench)&curve9767_inner_window_lookup,
		&Q2, (void *)&curve9767_inner_window_G64, 0, 0, 0, 0, 0);

	do_benchmark_fast("hash_to_curve",
		(funbench)&do_hash_to_curve,
		&Q2, (void *)seed48, (void *)sizeof seed48, 0, 0, 0, 0);

	do_benchmark_slow("ecdh_keygen",
		(funbench)&curve9767_ecdh_keygen,
		&k, bb, (void *)seed48, (void *)sizeof seed48, 0, 0, 0);
	do_benchmark_slow("ecdh_recv",
		(funbench)&curve9767_ecdh_recv,
		bb, (void *)sizeof bb, &k, bb, 0, 0, 0);

	do_benchmark_slow("sign_generate",
		(funbench)&curve9767_sign_generate,
		bb, &k, bb, &Q2, CURVE9767_OID_SHA384,
		(void *)seed48, (void *)sizeof seed48);
	do_benchmark_slow("sign_verify",
		(funbench)&curve9767_sign_verify,
		bb, &Q2, CURVE9767_OID_SHA384,
		(void *)seed48, (void *)sizeof seed48, 0, 0);

	/* Blink the LED (PD12) on the board. */
	while (1) {
		int i;

		gpio_toggle(GPIOD, GPIO12);

		/* Upon button press, blink more slowly. */
		if (gpio_get(GPIOA, GPIO0)) {
			for (i = 0; i < 3000000; i++) {	/* Wait a bit. */
				__asm__("nop");
			}
		}

		for (i = 0; i < 3000000; i++) {		/* Wait a bit. */
			__asm__("nop");
		}
	}

	return 0;
}
