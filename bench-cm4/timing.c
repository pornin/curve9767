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

/*
 * These scalar values were generated randomly with Sage. They are
 * encoded in the usual unsigned little-endian format.
 */
static const uint8_t RAND_SCALAR[100][32] = {
	{ 0x6D, 0x90, 0x70, 0x03, 0xA3, 0x2D, 0x1E, 0x3B,
	  0x0B, 0x80, 0xB0, 0x92, 0xA8, 0x74, 0x9E, 0xD4,
	  0xEC, 0xC7, 0xBC, 0x1D, 0x1D, 0xDC, 0x0F, 0x14,
	  0x55, 0x95, 0x15, 0x99, 0x1B, 0xF9, 0x57, 0x0A },
	{ 0x60, 0xE3, 0xC8, 0xED, 0x4F, 0xFD, 0x65, 0x11,
	  0xE2, 0xE9, 0xB5, 0xAC, 0xD5, 0x33, 0xB3, 0x08,
	  0xA0, 0x2D, 0x28, 0x1E, 0x45, 0x83, 0x6C, 0xE2,
	  0x52, 0xC0, 0x3C, 0x4F, 0x8F, 0xDA, 0x0F, 0x02 },
	{ 0x6C, 0x3B, 0x92, 0x0C, 0x30, 0xD8, 0x6E, 0xC8,
	  0x8D, 0x51, 0xE8, 0x67, 0xDE, 0x55, 0xB5, 0x6D,
	  0x72, 0xE0, 0xE9, 0x05, 0xD2, 0x88, 0x61, 0x1C,
	  0xDD, 0x8C, 0x1E, 0x40, 0xB2, 0xE1, 0xBE, 0x0D },
	{ 0x0D, 0x2E, 0x49, 0x92, 0x74, 0x77, 0x2A, 0xDD,
	  0xEA, 0x22, 0x9A, 0x20, 0xEB, 0x90, 0x57, 0xBD,
	  0xE4, 0x79, 0xBD, 0xFD, 0xFE, 0xC7, 0xF5, 0x6B,
	  0x19, 0x20, 0x4C, 0xE4, 0x57, 0xF5, 0x12, 0x0E },
	{ 0x6A, 0x3A, 0x5A, 0xFC, 0xA4, 0x01, 0x21, 0xDE,
	  0x60, 0xE4, 0xDD, 0x7E, 0x7B, 0x9A, 0xB0, 0x93,
	  0xF2, 0x13, 0x91, 0xAB, 0x63, 0x4C, 0x47, 0x4C,
	  0x3A, 0x67, 0x0D, 0xE8, 0xED, 0xAC, 0x30, 0x03 },
	{ 0x8E, 0x48, 0x29, 0xFF, 0xD5, 0xCE, 0xA8, 0x87,
	  0xAB, 0x47, 0xAF, 0x92, 0xFC, 0xE5, 0x55, 0x6E,
	  0x66, 0xE1, 0xB0, 0x89, 0xF1, 0x44, 0x48, 0xD6,
	  0xC5, 0xBE, 0xA1, 0xD5, 0x85, 0x2F, 0xB4, 0x0B },
	{ 0x7F, 0xA7, 0xD4, 0x59, 0x5E, 0xAF, 0x38, 0x58,
	  0x9D, 0xA9, 0x21, 0x66, 0x6E, 0xAC, 0x09, 0xEE,
	  0x28, 0xB7, 0x77, 0x08, 0xB7, 0xEB, 0x49, 0x0F,
	  0xC3, 0x79, 0x0E, 0xC1, 0xAB, 0x80, 0xA2, 0x02 },
	{ 0x3C, 0xB9, 0x1C, 0x24, 0x18, 0x73, 0x13, 0x27,
	  0xD1, 0xCD, 0xD6, 0x63, 0x68, 0xB7, 0x45, 0x88,
	  0x42, 0xFC, 0x7A, 0xCF, 0x6B, 0x3F, 0xD4, 0x59,
	  0xAD, 0x8B, 0x91, 0xB8, 0xA7, 0x86, 0xFD, 0x08 },
	{ 0x4C, 0x34, 0xE7, 0x33, 0x44, 0x53, 0x01, 0x1E,
	  0xC6, 0xFA, 0x0F, 0xAD, 0x48, 0x6E, 0x73, 0xD5,
	  0x47, 0x45, 0xE5, 0x5A, 0x0E, 0x59, 0x6F, 0x07,
	  0x35, 0x17, 0x23, 0x9A, 0x68, 0x08, 0xEA, 0x06 },
	{ 0x5A, 0x8F, 0xCA, 0xD6, 0x21, 0xD6, 0x27, 0x72,
	  0x7D, 0xCF, 0xE8, 0xCD, 0x6D, 0x63, 0xD2, 0x1A,
	  0x50, 0x29, 0x08, 0x19, 0xF5, 0xB9, 0x3A, 0x5A,
	  0xEE, 0x77, 0x4D, 0x33, 0xB2, 0x43, 0x09, 0x09 },
	{ 0x79, 0xF8, 0x6C, 0x16, 0xCE, 0xF8, 0xA1, 0x68,
	  0x50, 0xBF, 0x34, 0x84, 0xAD, 0xC8, 0x3C, 0xFF,
	  0x26, 0xE7, 0x11, 0xDA, 0x3F, 0x28, 0x44, 0x88,
	  0x24, 0xB9, 0x28, 0x0F, 0xC7, 0x4D, 0x1C, 0x02 },
	{ 0xB8, 0xF1, 0x51, 0x86, 0xD3, 0x64, 0xA8, 0x3B,
	  0x30, 0x54, 0xAE, 0x1D, 0x06, 0xDA, 0x2B, 0xDD,
	  0x79, 0x82, 0xE8, 0x16, 0x04, 0x52, 0xC6, 0xF7,
	  0x22, 0xD2, 0xCC, 0xEF, 0x56, 0x41, 0x40, 0x01 },
	{ 0xD1, 0x01, 0xB7, 0xA3, 0x7A, 0xEC, 0xA6, 0x87,
	  0x10, 0x18, 0x9E, 0x18, 0x74, 0x1A, 0x19, 0x81,
	  0x21, 0x3C, 0x12, 0x21, 0xF6, 0x0E, 0x31, 0xF0,
	  0xC1, 0x53, 0x94, 0x36, 0xAA, 0x4D, 0x4E, 0x01 },
	{ 0x7B, 0x94, 0xCE, 0x45, 0x7D, 0x29, 0x83, 0xDC,
	  0xE9, 0xCA, 0x9F, 0x69, 0x01, 0x0D, 0x1C, 0x9B,
	  0xFB, 0x64, 0xA2, 0xF7, 0xEF, 0x6D, 0x01, 0x72,
	  0xC6, 0x23, 0x64, 0xB0, 0xE9, 0x3B, 0xAF, 0x04 },
	{ 0x3E, 0x07, 0xAD, 0xA9, 0x2A, 0x58, 0xA7, 0x41,
	  0x7B, 0xCF, 0x73, 0x80, 0x1C, 0xF8, 0xBA, 0xE5,
	  0x7D, 0xCF, 0x0F, 0x8F, 0xE6, 0xF0, 0x74, 0xCF,
	  0xC6, 0x48, 0xBF, 0x03, 0xE8, 0x95, 0xDC, 0x01 },
	{ 0xD0, 0xA1, 0x01, 0x42, 0x18, 0x4E, 0x42, 0x35,
	  0x18, 0x86, 0xAE, 0x64, 0x73, 0x7C, 0xFF, 0x30,
	  0x9F, 0x8E, 0x83, 0xF3, 0x11, 0x50, 0xEE, 0xD8,
	  0xB8, 0x34, 0xC5, 0x59, 0xE1, 0xC9, 0xC4, 0x08 },
	{ 0x35, 0xB2, 0x85, 0x51, 0xDB, 0x67, 0x38, 0xE6,
	  0xA6, 0xA2, 0xD0, 0x99, 0x48, 0x97, 0x51, 0x73,
	  0x7E, 0x02, 0x54, 0xE1, 0x09, 0xDC, 0xD6, 0x2E,
	  0x89, 0x68, 0xA8, 0xCC, 0xC9, 0xC0, 0x29, 0x00 },
	{ 0x33, 0x6C, 0xCA, 0xA5, 0x05, 0xAD, 0x40, 0x82,
	  0xF9, 0x10, 0x0A, 0x26, 0x21, 0x17, 0x42, 0x72,
	  0xAE, 0x4E, 0xB7, 0x8E, 0x18, 0xE1, 0x1C, 0xCC,
	  0x52, 0x6D, 0x1E, 0xE0, 0x73, 0x1C, 0x1F, 0x04 },
	{ 0xF7, 0x37, 0x41, 0xCF, 0xBC, 0x8A, 0x0D, 0x7F,
	  0x7B, 0xD9, 0x98, 0x64, 0xD8, 0x26, 0x14, 0x06,
	  0xC2, 0x92, 0x61, 0xAD, 0x25, 0x81, 0xBB, 0x65,
	  0x3D, 0x97, 0x48, 0x9F, 0xFC, 0x90, 0xC4, 0x01 },
	{ 0xCD, 0xE2, 0x65, 0x29, 0xC8, 0x71, 0x4A, 0x6C,
	  0x0E, 0x97, 0xFB, 0x28, 0xB2, 0xE8, 0xA0, 0x21,
	  0x3E, 0x1E, 0x80, 0xF5, 0x63, 0x11, 0xD5, 0x25,
	  0xEE, 0xE3, 0xA6, 0x52, 0xD6, 0x35, 0x32, 0x01 },
	{ 0xB8, 0xA3, 0x2F, 0x0B, 0x93, 0x01, 0x17, 0x10,
	  0x71, 0x6F, 0x92, 0x9A, 0xB0, 0x15, 0xCF, 0x96,
	  0xA5, 0x2B, 0xFD, 0x89, 0x67, 0x3F, 0xDE, 0x91,
	  0x63, 0xE7, 0x9D, 0x9E, 0xF9, 0x03, 0xAA, 0x00 },
	{ 0x92, 0xAB, 0x59, 0x67, 0x5A, 0x66, 0x8B, 0xD0,
	  0xD7, 0xE6, 0x28, 0x0D, 0x30, 0x54, 0x7E, 0xD4,
	  0x84, 0x09, 0xC1, 0x36, 0x69, 0x96, 0x35, 0xC0,
	  0xB9, 0x3C, 0x28, 0x27, 0xC2, 0x3A, 0x64, 0x09 },
	{ 0x3E, 0xC2, 0xE3, 0x67, 0x01, 0x17, 0xE2, 0x4D,
	  0xB4, 0x47, 0x2D, 0x15, 0x1E, 0x57, 0x64, 0xCF,
	  0x65, 0x37, 0x6E, 0x3A, 0x22, 0x6C, 0x3D, 0x1C,
	  0x98, 0x4C, 0x96, 0x57, 0x7C, 0x39, 0x71, 0x0A },
	{ 0x9D, 0x15, 0x23, 0x25, 0xCC, 0x79, 0xC8, 0x53,
	  0x00, 0xA2, 0xD6, 0x9F, 0xAE, 0x51, 0xB4, 0x18,
	  0x9C, 0x71, 0x4E, 0x7A, 0x8C, 0x24, 0xAA, 0x80,
	  0x51, 0xBD, 0x50, 0x53, 0x7B, 0x33, 0x55, 0x0C },
	{ 0xB5, 0x9C, 0xD1, 0x13, 0xE6, 0x86, 0xC8, 0x79,
	  0x73, 0xEE, 0x83, 0x5D, 0xFE, 0x75, 0x87, 0xF4,
	  0x85, 0xBB, 0x83, 0x36, 0x32, 0x22, 0x1E, 0xCF,
	  0xEE, 0x9A, 0xD7, 0xD2, 0x5E, 0x5C, 0xF2, 0x06 },
	{ 0xDC, 0xD3, 0x52, 0xED, 0x38, 0x1F, 0x47, 0xB1,
	  0xDF, 0xEA, 0xC4, 0xCD, 0xA2, 0xBD, 0xCB, 0x32,
	  0xE2, 0xE9, 0xA9, 0x6B, 0x63, 0xCA, 0x0C, 0x9E,
	  0xA0, 0x96, 0xC3, 0x68, 0x9E, 0x6F, 0xF9, 0x03 },
	{ 0x07, 0x67, 0x5D, 0x4E, 0x90, 0x99, 0x0F, 0x00,
	  0xC8, 0x65, 0xB1, 0x80, 0x24, 0x7D, 0xDA, 0xC0,
	  0xB2, 0x96, 0xD2, 0x4A, 0x13, 0xAE, 0xCA, 0xF3,
	  0x20, 0xC9, 0xBF, 0x5C, 0x29, 0x16, 0xF5, 0x07 },
	{ 0xFC, 0x1B, 0x9F, 0xEF, 0x91, 0x3E, 0x90, 0xB6,
	  0x62, 0xE6, 0x9A, 0x48, 0xAB, 0x10, 0x2A, 0xD1,
	  0x29, 0xA4, 0xB4, 0xF3, 0xB9, 0x8E, 0x33, 0x45,
	  0xCD, 0x53, 0x62, 0xE4, 0xD4, 0x84, 0xE1, 0x02 },
	{ 0xEF, 0xB9, 0xFD, 0xF6, 0xC0, 0x5D, 0xD0, 0x53,
	  0x24, 0x76, 0xC5, 0x52, 0x83, 0x5B, 0xC7, 0xFF,
	  0xCD, 0x57, 0xC7, 0x4E, 0x69, 0x60, 0x0D, 0x7E,
	  0x2C, 0x03, 0xD1, 0x4A, 0xF0, 0x9E, 0xC5, 0x09 },
	{ 0x14, 0x0C, 0x87, 0xF2, 0x56, 0xBF, 0x62, 0x44,
	  0xF1, 0xD5, 0x12, 0xCB, 0xEA, 0xEE, 0xCC, 0xC1,
	  0xB1, 0x54, 0x7B, 0xCF, 0xD9, 0x57, 0xF3, 0x88,
	  0xF1, 0xFF, 0xAC, 0xB5, 0x78, 0x80, 0x83, 0x08 },
	{ 0x4E, 0xC7, 0x6C, 0xE6, 0x5F, 0xD8, 0x97, 0x0A,
	  0x12, 0x3F, 0x1E, 0xB0, 0xEA, 0x34, 0x5E, 0x83,
	  0xDF, 0x16, 0xDD, 0x9D, 0xD3, 0xD1, 0xF3, 0x63,
	  0x5F, 0x67, 0x83, 0xD8, 0xC0, 0xAA, 0xF0, 0x09 },
	{ 0x64, 0x31, 0x86, 0xFD, 0x3D, 0xDC, 0x49, 0xA6,
	  0x00, 0x99, 0xB0, 0x97, 0x7A, 0xCE, 0xAA, 0xC7,
	  0x6F, 0x05, 0x21, 0x5A, 0xC3, 0x14, 0x80, 0x34,
	  0xC6, 0xD8, 0x68, 0xEB, 0x13, 0xDA, 0x4B, 0x09 },
	{ 0xF0, 0x60, 0xC1, 0x99, 0xA9, 0xAB, 0x74, 0x47,
	  0x53, 0x84, 0xEF, 0xD6, 0x31, 0xCD, 0x94, 0x46,
	  0xEC, 0x06, 0x85, 0x33, 0xA1, 0x19, 0xB1, 0x54,
	  0x92, 0x69, 0x3D, 0x85, 0x98, 0xD7, 0xDC, 0x01 },
	{ 0x93, 0xF8, 0x56, 0x00, 0xA2, 0xB8, 0x22, 0xB2,
	  0x5B, 0xDB, 0x5E, 0x49, 0x49, 0x8E, 0x57, 0xB2,
	  0xAD, 0xA6, 0x15, 0x4A, 0xFB, 0xC7, 0xF7, 0xEC,
	  0x9A, 0xC1, 0xE2, 0x59, 0xCB, 0x7F, 0x30, 0x05 },
	{ 0x2D, 0xBC, 0xE4, 0xE7, 0x1E, 0xB8, 0xE9, 0xC4,
	  0xD4, 0xBE, 0x29, 0xDC, 0xBD, 0x43, 0xF0, 0xBD,
	  0xD5, 0x6D, 0x14, 0x8A, 0x69, 0x5E, 0x47, 0x23,
	  0x32, 0xD4, 0x56, 0x8B, 0xBE, 0xAC, 0x8A, 0x01 },
	{ 0x09, 0x34, 0x7E, 0x5A, 0x6B, 0x55, 0x85, 0x7C,
	  0x37, 0x6C, 0x14, 0x3E, 0xA7, 0xC9, 0xD0, 0xAA,
	  0x49, 0x77, 0x37, 0x98, 0xC2, 0xC0, 0x4E, 0xB6,
	  0xFC, 0x02, 0xB8, 0x4C, 0x8A, 0x83, 0x63, 0x08 },
	{ 0x54, 0x57, 0x0A, 0x4D, 0x60, 0xAA, 0xA6, 0x57,
	  0x83, 0x76, 0x3E, 0xAA, 0xB1, 0xA7, 0xCD, 0xA4,
	  0xD7, 0x52, 0x62, 0xEC, 0xF9, 0x36, 0x71, 0xEA,
	  0x31, 0x13, 0x4D, 0x50, 0xE1, 0xB6, 0x90, 0x01 },
	{ 0x2C, 0xEC, 0xCB, 0x70, 0x8A, 0xD1, 0xF6, 0x77,
	  0x4D, 0x10, 0xA2, 0x8A, 0x10, 0x4C, 0xCE, 0xF4,
	  0x35, 0xB2, 0xB9, 0x44, 0xBA, 0x70, 0xA2, 0x93,
	  0x61, 0xBC, 0x64, 0x1D, 0xEF, 0x2A, 0x2F, 0x0A },
	{ 0x3F, 0x7D, 0xEA, 0x8E, 0xF9, 0x83, 0xA3, 0x8F,
	  0xDE, 0x9C, 0x47, 0x7B, 0xA8, 0x4D, 0xD6, 0xCC,
	  0xAE, 0x2F, 0xF0, 0x66, 0xFF, 0x21, 0x8B, 0x5E,
	  0xE3, 0x14, 0x66, 0x48, 0xD4, 0xE7, 0xCF, 0x0C },
	{ 0xBB, 0x36, 0xC9, 0xE5, 0x41, 0xF9, 0x8E, 0xB0,
	  0x87, 0xFC, 0x47, 0x33, 0x3C, 0xF8, 0x99, 0x56,
	  0x74, 0x32, 0xE4, 0xCE, 0xF8, 0xEC, 0x2F, 0x17,
	  0x4E, 0xB5, 0x3D, 0x41, 0x1C, 0xE6, 0xDD, 0x05 },
	{ 0x3B, 0xE5, 0xFC, 0xD1, 0xB3, 0x86, 0x0D, 0x71,
	  0x2A, 0x48, 0x16, 0xA1, 0x93, 0xC6, 0x84, 0xE7,
	  0x5E, 0x95, 0xE1, 0x7B, 0x91, 0x7C, 0x89, 0xDC,
	  0xCA, 0x9F, 0x1C, 0x87, 0x31, 0x76, 0x3C, 0x01 },
	{ 0x28, 0xF3, 0x6A, 0x07, 0xC4, 0xAA, 0x5F, 0x1C,
	  0x4A, 0xAC, 0x94, 0xB4, 0x8A, 0xFA, 0x57, 0x73,
	  0xA8, 0xAC, 0xD3, 0x80, 0xDD, 0x42, 0x35, 0xAE,
	  0x7F, 0x6A, 0x7D, 0x7C, 0xCB, 0xB9, 0xF0, 0x09 },
	{ 0xB2, 0x18, 0x24, 0xF4, 0xCC, 0xBA, 0x06, 0x07,
	  0x0F, 0xEB, 0x32, 0xC8, 0xCF, 0xC9, 0xB0, 0x36,
	  0x94, 0xFF, 0xCF, 0xEC, 0xF1, 0x05, 0xA3, 0xA3,
	  0x4B, 0xAB, 0xFF, 0xBB, 0xD6, 0xD6, 0x5C, 0x05 },
	{ 0xE8, 0x80, 0xE2, 0xA2, 0xBF, 0x12, 0x7B, 0xC9,
	  0x4B, 0x3F, 0x11, 0x7C, 0xED, 0xBD, 0xAA, 0xBD,
	  0xAD, 0x9D, 0x3D, 0xB9, 0x65, 0xD9, 0xDB, 0xCA,
	  0x2B, 0x17, 0x64, 0x6D, 0x1D, 0x80, 0x2F, 0x0D },
	{ 0xEC, 0x0A, 0x9E, 0xFD, 0x47, 0xB0, 0xEB, 0xF9,
	  0x71, 0x08, 0x25, 0x96, 0xBF, 0x2D, 0x82, 0xB9,
	  0xF9, 0x94, 0x80, 0x9A, 0xE2, 0x06, 0xBE, 0x20,
	  0x37, 0x84, 0x46, 0xFD, 0xF2, 0x70, 0xBE, 0x09 },
	{ 0xB7, 0x35, 0x8F, 0xFE, 0x8E, 0x6A, 0x9B, 0x0C,
	  0x1C, 0x13, 0xE2, 0x1B, 0x60, 0x67, 0xC2, 0x4F,
	  0x81, 0xBA, 0x4C, 0x6B, 0x05, 0x74, 0x76, 0xA4,
	  0x86, 0xA2, 0x2B, 0x21, 0x01, 0xED, 0x39, 0x0C },
	{ 0x9E, 0x1F, 0x94, 0x75, 0x70, 0xA8, 0xC8, 0xE1,
	  0x6B, 0x60, 0x4E, 0x74, 0xA6, 0x72, 0xDF, 0x91,
	  0xBD, 0x0D, 0x00, 0xA0, 0xE6, 0xD8, 0x9A, 0x40,
	  0x35, 0xF8, 0x71, 0x89, 0xEA, 0x8D, 0x03, 0x0C },
	{ 0x2C, 0x1E, 0xA3, 0x27, 0x79, 0x30, 0xA6, 0xD0,
	  0x4E, 0xCB, 0x31, 0xFD, 0x2D, 0xF5, 0xD2, 0x66,
	  0xB6, 0x8D, 0xED, 0xE4, 0xD7, 0x55, 0xF9, 0x3F,
	  0x1D, 0xC9, 0xAE, 0xC7, 0xB0, 0xAD, 0x14, 0x01 },
	{ 0x8E, 0x9F, 0x27, 0xD0, 0x70, 0xA2, 0xCD, 0xE3,
	  0xE9, 0xEA, 0xC9, 0xEA, 0x06, 0x40, 0xBF, 0x73,
	  0xCA, 0xDC, 0xDC, 0xD4, 0x7C, 0xB3, 0xF8, 0x95,
	  0xF8, 0x5F, 0xB3, 0x67, 0x20, 0x99, 0xB6, 0x0C },
	{ 0x54, 0x23, 0x47, 0x08, 0xBF, 0xB8, 0x3E, 0x62,
	  0x8C, 0xE3, 0x9C, 0xFA, 0x6F, 0x17, 0x8C, 0x5E,
	  0xC9, 0xF0, 0x06, 0xDA, 0xFD, 0xE9, 0x2A, 0x8B,
	  0xC1, 0xCC, 0x05, 0xB0, 0xDB, 0xF8, 0x96, 0x0D },
	{ 0x8B, 0xF5, 0x37, 0x71, 0x51, 0x79, 0x1C, 0x36,
	  0x5E, 0x20, 0x69, 0x55, 0x17, 0xA6, 0xC5, 0xF4,
	  0x2F, 0x98, 0x0C, 0xC8, 0x83, 0x7F, 0x9B, 0xC2,
	  0x24, 0x82, 0x3E, 0xDB, 0x54, 0x1A, 0x1C, 0x0A },
	{ 0x2C, 0xFA, 0x9D, 0x4C, 0x90, 0x2D, 0xF2, 0x8D,
	  0x36, 0xE9, 0xEB, 0xC0, 0xDC, 0x04, 0x16, 0x77,
	  0xD8, 0x7A, 0x7F, 0x0F, 0x36, 0x62, 0xFC, 0x55,
	  0x16, 0xF0, 0x8D, 0x51, 0x95, 0xC4, 0x85, 0x06 },
	{ 0x32, 0x28, 0xFF, 0x9A, 0x2D, 0x61, 0x55, 0x5A,
	  0x95, 0x5E, 0xD6, 0xBE, 0x89, 0x09, 0xAA, 0xB2,
	  0xB7, 0xD0, 0x38, 0x8A, 0xF2, 0xF3, 0x6D, 0xA9,
	  0xF6, 0xE6, 0x24, 0x1A, 0x5F, 0xDE, 0x69, 0x0B },
	{ 0x83, 0xD4, 0xEF, 0x36, 0xBD, 0x3B, 0x0C, 0x57,
	  0x66, 0xFB, 0xD0, 0x51, 0x70, 0x4D, 0x5B, 0xAA,
	  0xF4, 0x0B, 0x43, 0xF9, 0x4A, 0x56, 0xEB, 0xFB,
	  0xC2, 0x07, 0x86, 0x3E, 0x99, 0x29, 0x93, 0x00 },
	{ 0x66, 0xD9, 0xBF, 0x66, 0x9C, 0xED, 0xDB, 0xBF,
	  0xB5, 0x05, 0xE7, 0x17, 0xD4, 0x9E, 0x31, 0xCD,
	  0x17, 0x2E, 0xF0, 0xBC, 0x48, 0x1F, 0x63, 0x7E,
	  0xBE, 0x7A, 0xDB, 0x3F, 0x58, 0x6E, 0x89, 0x0A },
	{ 0x95, 0x63, 0x7F, 0x2C, 0x91, 0x5E, 0x3B, 0x61,
	  0x72, 0xB3, 0x26, 0x8D, 0x11, 0x52, 0x86, 0xA1,
	  0x46, 0x4B, 0xC6, 0xD1, 0x94, 0x7C, 0x6C, 0x91,
	  0x6C, 0x62, 0x09, 0x1C, 0xA9, 0x56, 0xDE, 0x0C },
	{ 0x26, 0xB6, 0x12, 0xDB, 0x8A, 0x96, 0x96, 0xD0,
	  0x9F, 0xF2, 0x66, 0xB7, 0xEA, 0xEA, 0xE8, 0x93,
	  0x99, 0xFE, 0x14, 0x1F, 0x17, 0xC1, 0x4B, 0xEA,
	  0x17, 0x28, 0x12, 0x2D, 0x50, 0xFD, 0xE2, 0x0C },
	{ 0x85, 0x69, 0xF8, 0x18, 0x15, 0x9E, 0x9E, 0x11,
	  0x2D, 0x77, 0x12, 0x6C, 0xCA, 0x7F, 0xB0, 0x24,
	  0x21, 0xF8, 0xD1, 0xF5, 0x5D, 0xC8, 0xE5, 0xBB,
	  0x06, 0xB4, 0x97, 0x18, 0x2D, 0xB4, 0xD4, 0x07 },
	{ 0x2C, 0xF7, 0x98, 0x55, 0xBD, 0x31, 0x0A, 0xF5,
	  0x51, 0x08, 0x81, 0x9E, 0x94, 0x61, 0x97, 0x52,
	  0x4A, 0xBB, 0xE6, 0xCD, 0x65, 0xD4, 0x23, 0xEF,
	  0x45, 0xED, 0xFB, 0xB4, 0x50, 0x4A, 0x52, 0x06 },
	{ 0x6D, 0xE8, 0x7C, 0x9F, 0x72, 0xB9, 0x96, 0xBE,
	  0x0D, 0xC6, 0x25, 0x91, 0xB4, 0x42, 0x0D, 0x0E,
	  0xB7, 0xAD, 0x63, 0xD5, 0xFD, 0xE7, 0x00, 0x9E,
	  0xDD, 0x6F, 0x6C, 0x71, 0x33, 0xCD, 0xAF, 0x02 },
	{ 0xFB, 0x25, 0xF9, 0x33, 0x11, 0x74, 0x03, 0x80,
	  0xB3, 0xC6, 0x1A, 0xA5, 0xDE, 0x2D, 0x0E, 0x9F,
	  0x87, 0x96, 0xAA, 0x68, 0x0B, 0x88, 0x05, 0x69,
	  0x6F, 0x58, 0xAD, 0x44, 0xF4, 0xFF, 0x1C, 0x0B },
	{ 0x95, 0xF2, 0x78, 0x8D, 0xF6, 0x42, 0x54, 0x8C,
	  0x04, 0x63, 0x41, 0xCA, 0xC0, 0xA2, 0xC3, 0x5C,
	  0xF4, 0xB9, 0x0F, 0x82, 0x84, 0x89, 0xC8, 0x1F,
	  0xB0, 0xCE, 0xDB, 0xB4, 0xB8, 0xC7, 0xAA, 0x01 },
	{ 0x09, 0xB7, 0x2F, 0x5B, 0xC9, 0x89, 0x25, 0xAF,
	  0x89, 0x3E, 0x19, 0xEC, 0x0A, 0xF1, 0x11, 0x7F,
	  0x6E, 0xF0, 0xA3, 0x12, 0x3B, 0xE7, 0x05, 0x4B,
	  0x1F, 0x9B, 0xFA, 0x09, 0x75, 0x2E, 0xE8, 0x06 },
	{ 0x4A, 0x86, 0x82, 0x36, 0xE8, 0x50, 0x5F, 0x50,
	  0xD8, 0x91, 0x91, 0xD7, 0x73, 0x74, 0xB3, 0x0C,
	  0x0E, 0xAB, 0xE5, 0xC0, 0xA3, 0xA2, 0x9B, 0x50,
	  0x73, 0xF0, 0xB1, 0xE5, 0x51, 0x34, 0xC1, 0x0B },
	{ 0x26, 0x7E, 0xD7, 0x8D, 0xC7, 0x42, 0x5D, 0xF0,
	  0x9E, 0x7F, 0x6B, 0x13, 0xB4, 0x0B, 0xE1, 0x0C,
	  0x91, 0x2E, 0x72, 0xE8, 0x14, 0xB4, 0x8B, 0xDC,
	  0xFA, 0x0E, 0x9A, 0x56, 0x23, 0x10, 0x1D, 0x01 },
	{ 0x61, 0x37, 0x14, 0xF0, 0xFA, 0xDB, 0x05, 0x25,
	  0x82, 0xF3, 0xA4, 0x15, 0x6B, 0x71, 0x08, 0x80,
	  0x8A, 0xB2, 0xEE, 0x7A, 0x57, 0x7E, 0xBD, 0xE0,
	  0x58, 0x72, 0x8F, 0x5B, 0x10, 0xD2, 0x10, 0x02 },
	{ 0x43, 0xB4, 0xC5, 0xB1, 0x61, 0xF5, 0x48, 0x1F,
	  0xD2, 0x5F, 0x33, 0xD2, 0xC6, 0xB3, 0x71, 0x82,
	  0xBB, 0x6A, 0xBE, 0xAA, 0xC7, 0xE9, 0x7F, 0xCF,
	  0x6A, 0xC5, 0x51, 0x78, 0x45, 0x0D, 0x19, 0x02 },
	{ 0xC2, 0x71, 0x23, 0xA0, 0x21, 0x03, 0x53, 0x65,
	  0xB4, 0x42, 0x81, 0x0B, 0x05, 0xD5, 0x7B, 0x38,
	  0xFA, 0x6F, 0x48, 0x68, 0x4D, 0x16, 0xCF, 0x8F,
	  0xA6, 0x6E, 0xE9, 0xAA, 0x95, 0xD2, 0xFF, 0x03 },
	{ 0x99, 0xA4, 0x30, 0x35, 0xBB, 0x41, 0xE0, 0x7D,
	  0x87, 0x98, 0xD4, 0xC7, 0xCD, 0x9C, 0x0B, 0x65,
	  0x3C, 0x1C, 0xCE, 0x02, 0x02, 0x68, 0xE3, 0x45,
	  0xDE, 0x91, 0xFC, 0x97, 0x84, 0xEC, 0xE7, 0x08 },
	{ 0x28, 0x78, 0xE3, 0x18, 0x0D, 0xC5, 0xE6, 0xDC,
	  0x87, 0x5C, 0xFB, 0x63, 0x1A, 0xAC, 0x03, 0xF9,
	  0xD4, 0x04, 0x27, 0x9C, 0xEB, 0x70, 0x9B, 0x89,
	  0x87, 0xDC, 0x8B, 0x02, 0x2A, 0x61, 0xDC, 0x0D },
	{ 0x66, 0x22, 0xF7, 0xC3, 0x4A, 0xB7, 0xBF, 0x50,
	  0x2B, 0xB9, 0xFC, 0x1D, 0x6C, 0xEC, 0xA9, 0x9D,
	  0xEF, 0xEF, 0xAA, 0x24, 0x0A, 0x7F, 0x67, 0xCB,
	  0xAA, 0x91, 0xC3, 0x23, 0x82, 0x6A, 0x54, 0x0A },
	{ 0x69, 0xF3, 0x31, 0x74, 0xA9, 0xCA, 0xA7, 0x62,
	  0xBF, 0xA1, 0x5C, 0xEF, 0x4D, 0x72, 0x14, 0xDB,
	  0xAE, 0xDC, 0xCB, 0x7A, 0x95, 0x96, 0x4D, 0xC5,
	  0xEC, 0xAC, 0x12, 0x6C, 0x31, 0xD7, 0x74, 0x0D },
	{ 0x32, 0xCA, 0x6A, 0xB2, 0x18, 0xE9, 0xFB, 0xB6,
	  0x55, 0xF8, 0x84, 0x92, 0x2E, 0x00, 0xCC, 0x89,
	  0x35, 0x52, 0x33, 0x4F, 0x92, 0xC8, 0x21, 0xD7,
	  0xFD, 0x15, 0xA6, 0xF4, 0x8E, 0x59, 0xDD, 0x04 },
	{ 0x70, 0xFB, 0xF7, 0x3E, 0x8F, 0xF1, 0xF2, 0xAF,
	  0x80, 0x1C, 0x14, 0xD1, 0x4D, 0xD9, 0x27, 0xF3,
	  0x57, 0x99, 0xE1, 0x7A, 0xC0, 0xC3, 0x61, 0xD0,
	  0x51, 0xE0, 0xF5, 0x7D, 0x06, 0x94, 0x38, 0x00 },
	{ 0xB0, 0x5B, 0x28, 0x3B, 0x17, 0x6F, 0xF1, 0xA9,
	  0x56, 0x94, 0x56, 0x2D, 0x41, 0xD7, 0x3D, 0x7D,
	  0xBA, 0x5E, 0x9E, 0x40, 0x78, 0xA8, 0xAF, 0xA2,
	  0x56, 0x5D, 0x36, 0x38, 0xD9, 0xA9, 0x76, 0x0A },
	{ 0xBC, 0x92, 0x51, 0xE0, 0x7B, 0xF9, 0x37, 0xD4,
	  0x45, 0x23, 0xDE, 0x5E, 0x0B, 0xA0, 0x71, 0x65,
	  0x10, 0x46, 0x4D, 0xA6, 0xDC, 0x2A, 0xF3, 0x7C,
	  0x13, 0x10, 0xBB, 0xE0, 0x81, 0x55, 0xED, 0x02 },
	{ 0x57, 0x4E, 0x43, 0x7E, 0x9E, 0x3A, 0xE9, 0xB0,
	  0x81, 0x7C, 0x83, 0x08, 0x6D, 0x38, 0x4A, 0xDB,
	  0x59, 0x5E, 0xA1, 0x84, 0xB2, 0x50, 0x90, 0xB4,
	  0xFE, 0xEE, 0x4B, 0x64, 0xE3, 0x7B, 0xA1, 0x06 },
	{ 0x8C, 0x16, 0x5D, 0xC0, 0x87, 0xB2, 0x98, 0x4D,
	  0xD3, 0x72, 0x14, 0x06, 0xDE, 0x18, 0xA0, 0x31,
	  0xF4, 0x73, 0xD3, 0x22, 0x84, 0xEC, 0x5B, 0xE5,
	  0x9C, 0x9C, 0x5A, 0x5B, 0xAF, 0x8B, 0xF9, 0x02 },
	{ 0xB8, 0xDD, 0xF2, 0x40, 0x76, 0xD4, 0xD3, 0xAE,
	  0x76, 0x56, 0xD4, 0xA8, 0x00, 0xE2, 0x2A, 0x33,
	  0x14, 0x57, 0xD8, 0xEA, 0x6A, 0x94, 0x18, 0x07,
	  0xAB, 0xDD, 0x59, 0xF4, 0x88, 0xDA, 0x22, 0x01 },
	{ 0x0F, 0x63, 0xEA, 0x99, 0xC9, 0x0A, 0x24, 0x57,
	  0x94, 0x5B, 0x03, 0x85, 0xCB, 0xFF, 0xC5, 0xA8,
	  0xC7, 0xBF, 0xFF, 0x47, 0x1B, 0x82, 0xA9, 0xE6,
	  0x1E, 0xB4, 0x23, 0xC5, 0x5E, 0x2E, 0x39, 0x0A },
	{ 0xBF, 0xE9, 0x99, 0x2D, 0x90, 0xB3, 0x9E, 0x63,
	  0x46, 0xBF, 0x96, 0xAC, 0x05, 0x88, 0x5B, 0x87,
	  0x78, 0xE7, 0xC2, 0x5E, 0x79, 0x67, 0xE2, 0xAF,
	  0xD6, 0x84, 0x3C, 0x58, 0x44, 0x2E, 0x71, 0x0B },
	{ 0xF9, 0x38, 0x77, 0xA4, 0xFB, 0xA4, 0x0F, 0xD0,
	  0xD6, 0x3B, 0x16, 0x8A, 0xB4, 0x1A, 0x17, 0x3B,
	  0x75, 0x75, 0xF9, 0x24, 0x5B, 0xE4, 0x47, 0xA3,
	  0xFA, 0xAF, 0x3C, 0x13, 0x63, 0xE4, 0x73, 0x06 },
	{ 0xBC, 0x56, 0xFD, 0x0D, 0x6F, 0xC2, 0x6A, 0x7A,
	  0x9A, 0xCE, 0xD1, 0x5A, 0xA3, 0xA3, 0x63, 0x91,
	  0xF9, 0x90, 0x40, 0x48, 0xF5, 0x99, 0x78, 0xA6,
	  0x56, 0xB3, 0xD6, 0xB1, 0x36, 0xA5, 0x1A, 0x0A },
	{ 0x8F, 0x2F, 0xA3, 0x64, 0x9C, 0x6F, 0x04, 0x16,
	  0xB5, 0xAB, 0xB6, 0x13, 0x0B, 0xD2, 0xDB, 0xC9,
	  0xA6, 0x9A, 0x64, 0x56, 0xFE, 0x03, 0x1C, 0x18,
	  0xBE, 0x49, 0x08, 0x5D, 0x42, 0x87, 0xDE, 0x0A },
	{ 0xEA, 0xB8, 0xDB, 0xB3, 0xF4, 0x88, 0x6F, 0x76,
	  0x4B, 0x27, 0x84, 0xBF, 0x4E, 0x62, 0x40, 0x8C,
	  0x6C, 0x97, 0xCB, 0xCE, 0x3F, 0xA0, 0xA0, 0x57,
	  0x15, 0xFA, 0x00, 0xFC, 0xC9, 0x6B, 0xAC, 0x06 },
	{ 0x3A, 0xDD, 0x2A, 0xAD, 0x94, 0xE5, 0x21, 0x32,
	  0xEC, 0xB8, 0x0D, 0x36, 0x05, 0x75, 0x73, 0x19,
	  0x8D, 0x39, 0xBF, 0x75, 0x5D, 0xAB, 0x01, 0x1D,
	  0xB4, 0x77, 0x18, 0x1F, 0x1A, 0xB7, 0x09, 0x0A },
	{ 0x44, 0x15, 0x98, 0xF1, 0xD0, 0x68, 0x7A, 0x60,
	  0xE4, 0xE5, 0xE1, 0xE6, 0x52, 0xCA, 0xF3, 0xB2,
	  0x86, 0x82, 0x70, 0xEA, 0xB8, 0x28, 0xEB, 0x61,
	  0xA2, 0x2F, 0x76, 0xF5, 0x30, 0xFB, 0x0C, 0x0B },
	{ 0x12, 0x71, 0xCF, 0x4C, 0x9D, 0x6A, 0x6A, 0x84,
	  0xFC, 0xEE, 0x10, 0x55, 0xF1, 0xC8, 0xB3, 0x22,
	  0x47, 0xC8, 0x88, 0x65, 0xE7, 0x11, 0xFB, 0x68,
	  0x9B, 0xE5, 0x48, 0x07, 0x40, 0x57, 0x38, 0x03 },
	{ 0xA6, 0x6C, 0xC6, 0x52, 0xD8, 0xF2, 0x51, 0xD8,
	  0x35, 0x6D, 0x93, 0xFF, 0xC5, 0x6F, 0xC8, 0x5E,
	  0xD7, 0x34, 0x3C, 0x0E, 0x01, 0x54, 0xBB, 0xAD,
	  0x55, 0x88, 0x29, 0x18, 0x5B, 0x80, 0x16, 0x03 },
	{ 0x5C, 0x3C, 0x5A, 0x29, 0x96, 0x32, 0x13, 0x79,
	  0xD5, 0x5B, 0x2F, 0x13, 0x1E, 0x6F, 0x01, 0xCC,
	  0x05, 0xE2, 0x86, 0x70, 0x51, 0xF4, 0xC0, 0xB5,
	  0xDE, 0x4D, 0xC5, 0x48, 0x76, 0x2E, 0x0F, 0x07 },
	{ 0x3C, 0x89, 0xF4, 0xEA, 0x02, 0x13, 0x01, 0x8D,
	  0x8D, 0xD8, 0xD4, 0x54, 0x71, 0x9E, 0x6F, 0x58,
	  0xEC, 0x2F, 0x36, 0x0F, 0x2A, 0xF7, 0x0F, 0x51,
	  0xCA, 0xD0, 0x59, 0xE9, 0x96, 0x03, 0x1C, 0x0D },
	{ 0x2B, 0x62, 0x3A, 0x7F, 0xA6, 0x01, 0x7F, 0x8C,
	  0xD5, 0x81, 0x78, 0xE6, 0xA6, 0x3A, 0x97, 0x3F,
	  0x16, 0xF7, 0x97, 0x30, 0x1D, 0x5D, 0xC8, 0xBF,
	  0x76, 0xBE, 0xC6, 0x94, 0x98, 0xB5, 0x2F, 0x0C },
	{ 0x9D, 0x45, 0x1B, 0x8E, 0xEA, 0x69, 0x76, 0xAB,
	  0x7E, 0x82, 0xBB, 0xEA, 0x82, 0x7C, 0x4C, 0x09,
	  0x11, 0x70, 0x79, 0xFF, 0x10, 0x75, 0x83, 0x0F,
	  0xBB, 0x0B, 0xC3, 0x5F, 0xAD, 0xCE, 0x8F, 0x07 },
	{ 0xB7, 0xDC, 0xF4, 0x23, 0xF5, 0xB5, 0xDF, 0x87,
	  0xC2, 0x56, 0xAB, 0xE6, 0x24, 0x2F, 0xD4, 0xFD,
	  0x88, 0x55, 0xEC, 0x16, 0xDC, 0x9C, 0xBC, 0x0A,
	  0x7B, 0x0D, 0xF7, 0x1B, 0xD7, 0xC8, 0xEC, 0x03 },
	{ 0x55, 0x27, 0xBD, 0x0D, 0x5C, 0xBA, 0x1A, 0x03,
	  0x23, 0xBB, 0xCD, 0xCB, 0x25, 0xDC, 0xFC, 0xD3,
	  0x08, 0x73, 0x14, 0x15, 0xAF, 0xAD, 0x47, 0x9C,
	  0x3E, 0x9F, 0x36, 0x5B, 0x53, 0x34, 0xDA, 0x01 },
	{ 0xAF, 0x15, 0x98, 0xC7, 0xD9, 0xD2, 0x03, 0xAB,
	  0xFD, 0xC3, 0xE6, 0xE5, 0x6B, 0x7D, 0x54, 0xE0,
	  0xB3, 0x49, 0xD4, 0xE1, 0x9D, 0xB6, 0xC8, 0xF8,
	  0xDC, 0xFE, 0xD1, 0xFA, 0xB8, 0xCB, 0x79, 0x01 },
	{ 0x9F, 0x2F, 0xF7, 0x52, 0xF1, 0x2B, 0x9D, 0x5E,
	  0x81, 0x81, 0x59, 0x18, 0x81, 0x38, 0x4D, 0x72,
	  0xC7, 0xFF, 0xF0, 0x6C, 0xB8, 0x01, 0x51, 0x0D,
	  0x79, 0xF5, 0xEE, 0x5F, 0xFC, 0xA9, 0xFE, 0x0D },
	{ 0x76, 0xBA, 0xCB, 0x85, 0x7B, 0x45, 0x65, 0x2B,
	  0x31, 0xB0, 0x86, 0x81, 0x1D, 0x3E, 0x8E, 0x6B,
	  0x98, 0x57, 0x4F, 0x32, 0x40, 0x8D, 0x0A, 0x13,
	  0xA2, 0xE7, 0x9A, 0xC6, 0xD2, 0x12, 0x62, 0x0A },
	{ 0x9D, 0x8F, 0xFC, 0xE1, 0xDE, 0x43, 0x1A, 0x70,
	  0xF5, 0x33, 0xC4, 0xE0, 0xC0, 0xF8, 0xF1, 0x27,
	  0x63, 0x1C, 0x1D, 0x2D, 0x7E, 0xFD, 0x68, 0x18,
	  0x18, 0x63, 0xA4, 0x26, 0xF2, 0x9E, 0x4E, 0x0A },
	{ 0xD1, 0xCD, 0x5E, 0x3E, 0x15, 0x78, 0x22, 0x3B,
	  0x12, 0x5F, 0xDC, 0x3C, 0xE0, 0x4A, 0x73, 0xC9,
	  0x5A, 0xF4, 0x18, 0x8A, 0x3A, 0x35, 0xE7, 0x4A,
	  0xA9, 0x67, 0x1C, 0x58, 0x7C, 0xA4, 0xEA, 0x07 },
};

void
do_benchmark_reduce_basis(void)
{
	int i;
	uint32_t total;

	prf("%-25s", "reduce_basis");
	total = 0;
	for (i = 0; i < 100; i ++) {
		uint32_t t1, t2;
		curve9767_scalar b;
		uint8_t c0[16], c1[16];

		if (i == 1) {
			prf(" #1x: %10u", total);
		} else if (i == 10) {
			prf(" #10x: %10u", total);
		}
		curve9767_scalar_decode_strict(&b, RAND_SCALAR[i], 32);
		t1 = get_system_ticks();
		curve9767_inner_reduce_basis_vartime(c0, c1, &b);
		t2 = get_system_ticks();
		total += t2 - t1;
	}
	prf(" #100x: %10u", total);
	prf("\n");
}

void
do_benchmark_signature_vartime(void)
{
	int i;
	uint32_t total;

	prf("%-25s", "verify_vartime");
	total = 0;
	for (i = 0; i < 100; i ++) {
		uint32_t t1, t2;
		curve9767_scalar s;
		uint8_t t[32];
		curve9767_point Q;
		uint8_t tmp[32];
		uint8_t sig[64];
		int r;

		if (i == 1) {
			prf(" #1x: %10u", total);
		} else if (i == 10) {
			prf(" #10x: %10u", total);
		}

		/*
		 * Generate a key pair with a given seed.
		 */
		memset(tmp, 0, sizeof tmp);
		tmp[0] = (uint8_t)i;
		curve9767_keygen(&s, t, &Q, tmp, sizeof tmp);

		/*
		 * Compute a signature. We use the seed as a
		 * pretend hashed message (it does not matter for tests,
		 * since this get hashed again internally).
		 */
		curve9767_sign_generate(sig, &s, t, &Q,
			CURVE9767_OID_SHA3_256, tmp, sizeof tmp);

		/*
		 * Verify the signature.
		 */
		t1 = get_system_ticks();
		r = curve9767_sign_verify_vartime(sig, &Q,
			CURVE9767_OID_SHA3_256, tmp, sizeof tmp);
		t2 = get_system_ticks();
		total += t2 - t1;
		if (r != 1) {
			prf("ERR verify vartime! %d\n", i);
		}
	}
	prf(" #100x: %10u", total);
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

	do_benchmark_reduce_basis();
	do_benchmark_signature_vartime();

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
