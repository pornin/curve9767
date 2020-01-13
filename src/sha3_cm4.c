/*
 * SHAKE implementation.
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2017-2019  Falcon Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @author   Thomas Pornin <thomas.pornin@nccgroup.com>
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "sha3.h"

/*
 * Process the provided state.
 */
__attribute__((naked))
static void
process_block(uint64_t *A __attribute__((unused)))
{
	__asm__ (
	"push	{ r1, r2, r3, r4, r5, r6, r7, r8, r10, r11, r12, lr }\n\t"
	"sub	sp, sp, #232\n\t"
	"\n\t"
	"@ Invert some words (alternate internal representation, which\n\t"
	"@ saves some operations).\n\t"
	"\n\t"

#define INVERT_WORDS \
	"@ Invert A[1] and A[2].\n\t" \
	"adds	r1, r0, #8\n\t" \
	"ldm	r1, { r2, r3, r4, r5 }\n\t" \
	"mvns	r2, r2\n\t" \
	"mvns	r3, r3\n\t" \
	"mvns	r4, r4\n\t" \
	"mvns	r5, r5\n\t" \
	"stm	r1!, { r2, r3, r4, r5 }\n\t" \
	"@ Invert A[8]\n\t" \
	"adds	r1, r0, #64\n\t" \
	"ldm	r1, { r2, r3 }\n\t" \
	"mvns	r2, r2\n\t" \
	"mvns	r3, r3\n\t" \
	"stm	r1!, { r2, r3 }\n\t" \
	"@ Invert A[12]\n\t" \
	"adds	r1, r0, #96\n\t" \
	"ldm	r1, { r2, r3 }\n\t" \
	"mvns	r2, r2\n\t" \
	"mvns	r3, r3\n\t" \
	"stm	r1!, { r2, r3 }\n\t" \
	"@ Invert A[17]\n\t" \
	"adds	r1, r0, #136\n\t" \
	"ldm	r1, { r2, r3 }\n\t" \
	"mvns	r2, r2\n\t" \
	"mvns	r3, r3\n\t" \
	"stm	r1!, { r2, r3 }\n\t" \
	"@ Invert A[20]\n\t" \
	"adds	r1, r0, #160\n\t" \
	"ldm	r1, { r2, r3 }\n\t" \
	"mvns	r2, r2\n\t" \
	"mvns	r3, r3\n\t" \
	"stm	r1!, { r2, r3 }\n\t" \
	"\n\t"

	INVERT_WORDS

	"@ Do 24 rounds. Each loop iteration performs one rounds. We\n\t"
	"@ keep eight times the current round counter in [sp] (i.e.\n\t"
	"@ a multiple of 8, from 0 to 184).\n\t"
	"\n\t"
	"eors	r1, r1\n\t"
	"str	r1, [sp, #0]\n\t"
".process_block_loop:\n\t"
	"\n\t"
	"@ xor(A[5*i+0]) -> r1:r2\n\t"
	"@ xor(A[5*i+1]) -> r3:r4\n\t"
	"@ xor(A[5*i+2]) -> r5:r6\n\t"
	"@ xor(A[5*i+3]) -> r7:r8\n\t"
	"@ xor(A[5*i+4]) -> r10:r11\n\t"
	"ldm	r0!, { r1, r2, r3, r4, r5, r6, r7, r8 }\n\t"
	"adds	r0, #8\n\t"
	"ldm	r0!, { r10, r11, r12 }\n\t"
	"eors	r1, r10\n\t"
	"eors	r2, r11\n\t"
	"eors	r3, r12\n\t"
	"ldm	r0!, { r10, r11, r12 }\n\t"
	"eors	r4, r10\n\t"
	"eors	r5, r11\n\t"
	"eors	r6, r12\n\t"
	"ldm	r0!, { r10, r11 }\n\t"
	"eors	r7, r10\n\t"
	"eors	r8, r11\n\t"
	"adds	r0, #8\n\t"
	"ldm	r0!, { r10, r11, r12 }\n\t"
	"eors	r1, r10\n\t"
	"eors	r2, r11\n\t"
	"eors	r3, r12\n\t"
	"ldm	r0!, { r10, r11, r12 }\n\t"
	"eors	r4, r10\n\t"
	"eors	r5, r11\n\t"
	"eors	r6, r12\n\t"
	"ldm	r0!, { r10, r11 }\n\t"
	"eors	r7, r10\n\t"
	"eors	r8, r11\n\t"
	"adds	r0, #8\n\t"
	"ldm	r0!, { r10, r11, r12 }\n\t"
	"eors	r1, r10\n\t"
	"eors	r2, r11\n\t"
	"eors	r3, r12\n\t"
	"ldm	r0!, { r10, r11, r12 }\n\t"
	"eors	r4, r10\n\t"
	"eors	r5, r11\n\t"
	"eors	r6, r12\n\t"
	"ldm	r0!, { r10, r11 }\n\t"
	"eors	r7, r10\n\t"
	"eors	r8, r11\n\t"
	"adds	r0, #8\n\t"
	"ldm	r0!, { r10, r11, r12 }\n\t"
	"eors	r1, r10\n\t"
	"eors	r2, r11\n\t"
	"eors	r3, r12\n\t"
	"ldm	r0!, { r10, r11, r12 }\n\t"
	"eors	r4, r10\n\t"
	"eors	r5, r11\n\t"
	"eors	r6, r12\n\t"
	"ldm	r0!, { r10, r11 }\n\t"
	"eors	r7, r10\n\t"
	"eors	r8, r11\n\t"
	"ldm	r0!, { r10, r11 }\n\t"
	"subs	r0, #200\n\t"
	"ldr	r12, [r0, #32]\n\t"
	"eors	r10, r12\n\t"
	"ldr	r12, [r0, #36]\n\t"
	"eors	r11, r12\n\t"
	"ldr	r12, [r0, #72]\n\t"
	"eors	r10, r12\n\t"
	"ldr	r12, [r0, #76]\n\t"
	"eors	r11, r12\n\t"
	"ldr	r12, [r0, #112]\n\t"
	"eors	r10, r12\n\t"
	"ldr	r12, [r0, #116]\n\t"
	"eors	r11, r12\n\t"
	"ldr	r12, [r0, #152]\n\t"
	"eors	r10, r12\n\t"
	"ldr	r12, [r0, #156]\n\t"
	"eors	r11, r12\n\t"
	"\n\t"
	"@ t0 = xor(A[5*i+4]) ^ rotl1(xor(A[5*i+1])) -> r10:r11\n\t"
	"@ t1 = xor(A[5*i+0]) ^ rotl1(xor(A[5*i+2])) -> r1:r2\n\t"
	"@ t2 = xor(A[5*i+1]) ^ rotl1(xor(A[5*i+3])) -> r3:r4\n\t"
	"@ t3 = xor(A[5*i+2]) ^ rotl1(xor(A[5*i+4])) -> r5:r6\n\t"
	"@ t4 = xor(A[5*i+3]) ^ rotl1(xor(A[5*i+0])) -> r7:r8\n\t"
	"str	r11, [sp, #4]\n\t"
	"mov	r12, r10\n\t"
	"eors	r10, r10, r3, lsl #1\n\t"
	"eors	r10, r10, r4, lsr #31\n\t"
	"eors	r11, r11, r4, lsl #1\n\t"
	"eors	r11, r11, r3, lsr #31\n\t"
	"eors	r3, r3, r7, lsl #1\n\t"
	"eors	r3, r3, r8, lsr #31\n\t"
	"eors	r4, r4, r8, lsl #1\n\t"
	"eors	r4, r4, r7, lsr #31\n\t"
	"eors	r7, r7, r1, lsl #1\n\t"
	"eors	r7, r7, r2, lsr #31\n\t"
	"eors	r8, r8, r2, lsl #1\n\t"
	"eors	r8, r8, r1, lsr #31\n\t"
	"eors	r1, r1, r5, lsl #1\n\t"
	"eors	r1, r1, r6, lsr #31\n\t"
	"eors	r2, r2, r6, lsl #1\n\t"
	"eors	r2, r2, r5, lsr #31\n\t"
	"eors	r5, r5, r12, lsl #1\n\t"
	"eors	r6, r6, r12, lsr #31\n\t"
	"ldr	r12, [sp, #4]\n\t"
	"eors	r5, r5, r12, lsr #31\n\t"
	"eors	r6, r6, r12, lsl #1\n\t"
	"\n\t"
	"@ Save t2, t3 and t4 on the stack.\n\t"
	"addw	r12, sp, #4\n\t"
	"stm	r12, { r3, r4, r5, r6, r7, r8 }\n\t"
	"\n\t"
	"@ We XOR one of the t0..t4 values into each A[] word, and\n\t"
	"@ rotate the result by some amount (each word has its own\n\t"
	"@ amount). The results are written back into a stack buffer\n\t"
	"@ that starts at sp+32\n\t"
	"addw	r12, sp, #32\n\t"
	"\n\t"
	"@ XOR t0 into A[5*i+0] and t1 into A[5*i+1]; each A[i] is also\n\t"
	"@ rotated left by some amount.\n\t"
	"\n\t"
	"@ A[0] and A[1]\n\t"
	"ldm	r0!, { r5, r6, r7, r8 }\n\t"
	"eors	r5, r10\n\t"
	"eors	r6, r11\n\t"
	"eors	r3, r7, r1\n\t"
	"eors	r4, r8, r2\n\t"
	"lsl	r7, r3, #1\n\t"
	"orr	r7, r7, r4, lsr #31\n\t"
	"lsl	r8, r4, #1\n\t"
	"orr	r8, r8, r3, lsr #31\n\t"
	"stm	r12!, { r5, r6, r7, r8 }\n\t"
	"\n\t"
	"@ A[5] and A[6]\n\t"
	"adds	r0, #24\n\t"
	"ldm	r0!, { r5, r6, r7, r8 }\n\t"
	"eors	r3, r5, r10\n\t"
	"eors	r4, r6, r11\n\t"
	"lsl	r5, r4, #4\n\t"
	"orr	r5, r5, r3, lsr #28\n\t"
	"lsl	r6, r3, #4\n\t"
	"orr	r6, r6, r4, lsr #28\n\t"
	"eors	r3, r7, r1\n\t"
	"eors	r4, r8, r2\n\t"
	"lsl	r7, r4, #12\n\t"
	"orr	r7, r7, r3, lsr #20\n\t"
	"lsl	r8, r3, #12\n\t"
	"orr	r8, r8, r4, lsr #20\n\t"
	"stm	r12!, { r5, r6, r7, r8 }\n\t"
	"\n\t"
	"@ A[10] and A[11]\n\t"
	"adds	r0, #24\n\t"
	"ldm	r0!, { r5, r6, r7, r8 }\n\t"
	"eors	r3, r5, r10\n\t"
	"eors	r4, r6, r11\n\t"
	"lsl	r5, r3, #3\n\t"
	"orr	r5, r5, r4, lsr #29\n\t"
	"lsl	r6, r4, #3\n\t"
	"orr	r6, r6, r3, lsr #29\n\t"
	"eors	r3, r7, r1\n\t"
	"eors	r4, r8, r2\n\t"
	"lsl	r7, r3, #10\n\t"
	"orr	r7, r7, r4, lsr #22\n\t"
	"lsl	r8, r4, #10\n\t"
	"orr	r8, r8, r3, lsr #22\n\t"
	"stm	r12!, { r5, r6, r7, r8 }\n\t"
	"\n\t"
	"@ A[15] and A[16]\n\t"
	"adds	r0, #24\n\t"
	"ldm	r0!, { r5, r6, r7, r8 }\n\t"
	"eors	r3, r5, r10\n\t"
	"eors	r4, r6, r11\n\t"
	"lsl	r5, r4, #9\n\t"
	"orr	r5, r5, r3, lsr #23\n\t"
	"lsl	r6, r3, #9\n\t"
	"orr	r6, r6, r4, lsr #23\n\t"
	"eors	r3, r7, r1\n\t"
	"eors	r4, r8, r2\n\t"
	"lsl	r7, r4, #13\n\t"
	"orr	r7, r7, r3, lsr #19\n\t"
	"lsl	r8, r3, #13\n\t"
	"orr	r8, r8, r4, lsr #19\n\t"
	"stm	r12!, { r5, r6, r7, r8 }\n\t"
	"\n\t"
	"@ A[20] and A[21]\n\t"
	"adds	r0, #24\n\t"
	"ldm	r0!, { r5, r6, r7, r8 }\n\t"
	"eors	r3, r5, r10\n\t"
	"eors	r4, r6, r11\n\t"
	"lsl	r5, r3, #18\n\t"
	"orr	r5, r5, r4, lsr #14\n\t"
	"lsl	r6, r4, #18\n\t"
	"orr	r6, r6, r3, lsr #14\n\t"
	"eors	r3, r7, r1\n\t"
	"eors	r4, r8, r2\n\t"
	"lsl	r7, r3, #2\n\t"
	"orr	r7, r7, r4, lsr #30\n\t"
	"lsl	r8, r4, #2\n\t"
	"orr	r8, r8, r3, lsr #30\n\t"
	"stm	r12!, { r5, r6, r7, r8 }\n\t"
	"\n\t"
	"@ XOR t2 into A[5*i+2] and t3 into A[5*i+3]; each A[i] is also\n\t"
	"@ rotated left by some amount. We reload t2 into r1:r2 and t3\n\t"
	"@ into r3:r4.\n\t"
	"addw	r5, sp, #4\n\t"
	"ldm	r5!, { r1, r2, r3, r4 }\n\t"
	"\n\t"
	"@ A[2] and A[3]\n\t"
	"subs	r0, #160\n\t"
	"ldm	r0!, { r5, r6, r7, r8 }\n\t"
	"eors	r10, r5, r1\n\t"
	"eors	r11, r6, r2\n\t"
	"lsl	r5, r11, #30\n\t"
	"orr	r5, r5, r10, lsr #2\n\t"
	"lsl	r6, r10, #30\n\t"
	"orr	r6, r6, r11, lsr #2\n\t"
	"eors	r10, r7, r3\n\t"
	"eors	r11, r8, r4\n\t"
	"lsl	r7, r10, #28\n\t"
	"orr	r7, r7, r11, lsr #4\n\t"
	"lsl	r8, r11, #28\n\t"
	"orr	r8, r8, r10, lsr #4\n\t"
	"stm	r12!, { r5, r6, r7, r8 }\n\t"
	"\n\t"
	"@ A[7] and A[8]\n\t"
	"adds	r0, #24\n\t"
	"ldm	r0!, { r5, r6, r7, r8 }\n\t"
	"eors	r10, r5, r1\n\t"
	"eors	r11, r6, r2\n\t"
	"lsl	r5, r10, #6\n\t"
	"orr	r5, r5, r11, lsr #26\n\t"
	"lsl	r6, r11, #6\n\t"
	"orr	r6, r6, r10, lsr #26\n\t"
	"eors	r10, r7, r3\n\t"
	"eors	r11, r8, r4\n\t"
	"lsl	r7, r11, #23\n\t"
	"orr	r7, r7, r10, lsr #9\n\t"
	"lsl	r8, r10, #23\n\t"
	"orr	r8, r8, r11, lsr #9\n\t"
	"stm	r12!, { r5, r6, r7, r8 }\n\t"
	"\n\t"
	"@ A[12] and A[13]\n\t"
	"adds	r0, #24\n\t"
	"ldm	r0!, { r5, r6, r7, r8 }\n\t"
	"eors	r10, r5, r1\n\t"
	"eors	r11, r6, r2\n\t"
	"lsl	r5, r11, #11\n\t"
	"orr	r5, r5, r10, lsr #21\n\t"
	"lsl	r6, r10, #11\n\t"
	"orr	r6, r6, r11, lsr #21\n\t"
	"eors	r10, r7, r3\n\t"
	"eors	r11, r8, r4\n\t"
	"lsl	r7, r10, #25\n\t"
	"orr	r7, r7, r11, lsr #7\n\t"
	"lsl	r8, r11, #25\n\t"
	"orr	r8, r8, r10, lsr #7\n\t"
	"stm	r12!, { r5, r6, r7, r8 }\n\t"
	"\n\t"
	"@ A[17] and A[18]\n\t"
	"adds	r0, #24\n\t"
	"ldm	r0!, { r5, r6, r7, r8 }\n\t"
	"eors	r10, r5, r1\n\t"
	"eors	r11, r6, r2\n\t"
	"lsl	r5, r10, #15\n\t"
	"orr	r5, r5, r11, lsr #17\n\t"
	"lsl	r6, r11, #15\n\t"
	"orr	r6, r6, r10, lsr #17\n\t"
	"eors	r10, r7, r3\n\t"
	"eors	r11, r8, r4\n\t"
	"lsl	r7, r10, #21\n\t"
	"orr	r7, r7, r11, lsr #11\n\t"
	"lsl	r8, r11, #21\n\t"
	"orr	r8, r8, r10, lsr #11\n\t"
	"stm	r12!, { r5, r6, r7, r8 }\n\t"
	"\n\t"
	"@ A[22] and A[23]\n\t"
	"adds	r0, #24\n\t"
	"ldm	r0!, { r5, r6, r7, r8 }\n\t"
	"eors	r10, r5, r1\n\t"
	"eors	r11, r6, r2\n\t"
	"lsl	r5, r11, #29\n\t"
	"orr	r5, r5, r10, lsr #3\n\t"
	"lsl	r6, r10, #29\n\t"
	"orr	r6, r6, r11, lsr #3\n\t"
	"eors	r10, r7, r3\n\t"
	"eors	r11, r8, r4\n\t"
	"lsl	r7, r11, #24\n\t"
	"orr	r7, r7, r10, lsr #8\n\t"
	"lsl	r8, r10, #24\n\t"
	"orr	r8, r8, r11, lsr #8\n\t"
	"stm	r12!, { r5, r6, r7, r8 }\n\t"
	"\n\t"
	"@ XOR t4 into A[5*i+4]; each A[i] is also rotated left by some\n\t"
	"@ amount. We reload t4 into r1:r2.\n\t"
	"ldr	r1, [sp, #20]\n\t"
	"ldr	r2, [sp, #24]\n\t"
	"\n\t"
	"@ A[4]\n\t"
	"subs	r0, #160\n\t"
	"ldm	r0!, { r5, r6 }\n\t"
	"eors	r3, r5, r1\n\t"
	"eors	r4, r6, r2\n\t"
	"lsl	r5, r3, #27\n\t"
	"orr	r5, r5, r4, lsr #5\n\t"
	"lsl	r6, r4, #27\n\t"
	"orr	r6, r6, r3, lsr #5\n\t"
	"stm	r12!, { r5, r6 }\n\t"
	"\n\t"
	"@ A[9]\n\t"
	"adds	r0, #32\n\t"
	"ldm	r0!, { r5, r6 }\n\t"
	"eors	r3, r5, r1\n\t"
	"eors	r4, r6, r2\n\t"
	"lsl	r5, r3, #20\n\t"
	"orr	r5, r5, r4, lsr #12\n\t"
	"lsl	r6, r4, #20\n\t"
	"orr	r6, r6, r3, lsr #12\n\t"
	"stm	r12!, { r5, r6 }\n\t"
	"\n\t"
	"@ A[14]\n\t"
	"adds	r0, #32\n\t"
	"ldm	r0!, { r5, r6 }\n\t"
	"eors	r3, r5, r1\n\t"
	"eors	r4, r6, r2\n\t"
	"lsl	r5, r4, #7\n\t"
	"orr	r5, r5, r3, lsr #25\n\t"
	"lsl	r6, r3, #7\n\t"
	"orr	r6, r6, r4, lsr #25\n\t"
	"stm	r12!, { r5, r6 }\n\t"
	"\n\t"
	"@ A[19]\n\t"
	"adds	r0, #32\n\t"
	"ldm	r0!, { r5, r6 }\n\t"
	"eors	r3, r5, r1\n\t"
	"eors	r4, r6, r2\n\t"
	"lsl	r5, r3, #8\n\t"
	"orr	r5, r5, r4, lsr #24\n\t"
	"lsl	r6, r4, #8\n\t"
	"orr	r6, r6, r3, lsr #24\n\t"
	"stm	r12!, { r5, r6 }\n\t"
	"\n\t"
	"@ A[24]\n\t"
	"adds	r0, #32\n\t"
	"ldm	r0!, { r5, r6 }\n\t"
	"eors	r3, r5, r1\n\t"
	"eors	r4, r6, r2\n\t"
	"lsl	r5, r3, #14\n\t"
	"orr	r5, r5, r4, lsr #18\n\t"
	"lsl	r6, r4, #14\n\t"
	"orr	r6, r6, r3, lsr #18\n\t"
	"stm	r12!, { r5, r6 }\n\t"
	"\n\t"
	"subs	r0, #200\n\t"
	"\n\t"
	"@ At that point, the stack buffer at sp+32 contains the words\n\t"
	"@ at the following indexes (0 to 24) and offsets (from sp)\n\t"
	"@   A[ 0]    0      32\n\t"
	"@   A[ 1]    1      40\n\t"
	"@   A[ 2]   10     112\n\t"
	"@   A[ 3]   11     120\n\t"
	"@   A[ 4]   20     192\n\t"
	"@   A[ 5]    2      48\n\t"
	"@   A[ 6]    3      56\n\t"
	"@   A[ 7]   12     128\n\t"
	"@   A[ 8]   13     136\n\t"
	"@   A[ 9]   21     200\n\t"
	"@   A[10]    4      64\n\t"
	"@   A[11]    5      72\n\t"
	"@   A[12]   14     144\n\t"
	"@   A[13]   15     152\n\t"
	"@   A[14]   22     208\n\t"
	"@   A[15]    6      80\n\t"
	"@   A[16]    7      88\n\t"
	"@   A[17]   16     160\n\t"
	"@   A[18]   17     168\n\t"
	"@   A[19]   23     216\n\t"
	"@   A[20]    8      96\n\t"
	"@   A[21]    9     104\n\t"
	"@   A[22]   18     176\n\t"
	"@   A[23]   19     184\n\t"
	"@   A[24]   24     224\n\t"

#define KHI_LOAD(s0, s1, s2, s3, s4) \
	"ldr	r1, [sp, #(32 + 8 * " #s0 ")]\n\t" \
	"ldr	r2, [sp, #(36 + 8 * " #s0 ")]\n\t" \
	"ldr	r3, [sp, #(32 + 8 * " #s1 ")]\n\t" \
	"ldr	r4, [sp, #(36 + 8 * " #s1 ")]\n\t" \
	"ldr	r5, [sp, #(32 + 8 * " #s2 ")]\n\t" \
	"ldr	r6, [sp, #(36 + 8 * " #s2 ")]\n\t" \
	"ldr	r7, [sp, #(32 + 8 * " #s3 ")]\n\t" \
	"ldr	r8, [sp, #(36 + 8 * " #s3 ")]\n\t" \
	"ldr	r10, [sp, #(32 + 8 * " #s4 ")]\n\t" \
	"ldr	r11, [sp, #(36 + 8 * " #s4 ")]\n\t"

#define KHI_STEP(op, x0, x1, x2, x3, x4, x5, d) \
	#op "	r12, " #x0 ", " #x2 "\n\t" \
	"eors	r12, " #x4 "\n\t" \
	"str	r12, [r0, #(8 * " #d ")]\n\t" \
	#op "	r12, " #x1 ", " #x3 "\n\t" \
	"eors	r12, " #x5 "\n\t" \
	"str	r12, [r0, #(4 + 8 * " #d ")]\n\t"

	"@ A[0], A[6], A[12], A[18] and A[24]\n\t"
	KHI_LOAD(0, 3, 14, 17, 24)
	KHI_STEP(orrs, r3, r4, r5, r6, r1, r2, 0)
	KHI_STEP(orns, r7, r8, r5, r6, r3, r4, 1)
	KHI_STEP(ands, r7, r8, r10, r11, r5, r6, 2)
	KHI_STEP(orrs, r1, r2, r10, r11, r7, r8, 3)
	KHI_STEP(ands, r1, r2, r3, r4, r10, r11, 4)
	"\n\t"

	"@ A[3], A[9], A[10], A[16] and A[22]\n\t"
	KHI_LOAD(11, 21, 4, 7, 18)
	KHI_STEP(orrs, r3, r4, r5, r6, r1, r2, 5)
	KHI_STEP(ands, r7, r8, r5, r6, r3, r4, 6)
	KHI_STEP(orns, r7, r8, r10, r11, r5, r6, 7)
	KHI_STEP(orrs, r1, r2, r10, r11, r7, r8, 8)
	KHI_STEP(ands, r1, r2, r3, r4, r10, r11, 9)
	"\n\t"

	"@ A[1], A[7], A[13], A[19] and A[20]\n\t"
	KHI_LOAD(1, 12, 15, 23, 8)
	KHI_STEP(orrs, r3, r4, r5, r6, r1, r2, 10)
	KHI_STEP(ands, r7, r8, r5, r6, r3, r4, 11)
	KHI_STEP(bics, r10, r11, r7, r8, r5, r6, 12)
	"mvns	r7, r7\n\t"
	"mvns	r8, r8\n\t"
	KHI_STEP(orrs, r1, r2, r10, r11, r7, r8, 13)
	KHI_STEP(ands, r1, r2, r3, r4, r10, r11, 14)
	"\n\t"

	"@ A[4], A[5], A[11], A[17] and A[23]\n\t"
	KHI_LOAD(20, 2, 5, 16, 19)
	KHI_STEP(ands, r3, r4, r5, r6, r1, r2, 15)
	KHI_STEP(orrs, r7, r8, r5, r6, r3, r4, 16)
	KHI_STEP(orns, r10, r11, r7, r8, r5, r6, 17)
	"mvns	r7, r7\n\t"
	"mvns	r8, r8\n\t"
	KHI_STEP(ands, r1, r2, r10, r11, r7, r8, 18)
	KHI_STEP(orrs, r1, r2, r3, r4, r10, r11, 19)
	"\n\t"

	"@ A[2], A[8], A[14], A[15] and A[21]\n\t"
	KHI_LOAD(10, 13, 22, 6, 9)
	KHI_STEP(bics, r5, r6, r3, r4, r1, r2, 20)
	KHI_STEP(ands, r1, r2, r3, r4, r10, r11, 24)
	"mvns	r3, r3\n\t"
	"mvns	r4, r4\n\t"
	KHI_STEP(orrs, r7, r8, r5, r6, r3, r4, 21)
	KHI_STEP(ands, r7, r8, r10, r11, r5, r6, 22)
	KHI_STEP(orrs, r1, r2, r10, r11, r7, r8, 23)
	"\n\t"

	"@ Get round counter XOR round constant into A[0]\n\t"
	"ldr	r1, [sp, #0]\n\t"
	"adr	r2, .process_block_RC\n\t"
	"adds	r2, r1\n\t"
	"ldm	r2, { r3, r4 }\n\t"
	"ldm	r0, { r5, r6 }\n\t"
	"eors	r5, r3\n\t"
	"eors	r6, r4\n\t"
	"stm	r0, { r5, r6 }\n\t"
	"\n\t"
	"@ Increment round counter, loop until all 24 rounds are done.\n\t"
	"\n\t"
	"adds	r1, #8\n\t"
	"str	r1, [sp, #0]\n\t"
	"cmp	r1, #192\n\t"
	"blo	.process_block_loop\n\t"

	INVERT_WORDS

	"add	sp, sp, #232\n\t"
	"pop	{ r1, r2, r3, r4, r5, r6, r7, r8, r10, r11, r12, pc }\n\t"
	"\n\t"
	".align	2\n\t"
".process_block_RC:\n\t"
	".word	0x00000001\n\t"
	".word	0x00000000\n\t"
	".word	0x00008082\n\t"
	".word	0x00000000\n\t"
	".word	0x0000808A\n\t"
	".word	0x80000000\n\t"
	".word	0x80008000\n\t"
	".word	0x80000000\n\t"
	".word	0x0000808B\n\t"
	".word	0x00000000\n\t"
	".word	0x80000001\n\t"
	".word	0x00000000\n\t"
	".word	0x80008081\n\t"
	".word	0x80000000\n\t"
	".word	0x00008009\n\t"
	".word	0x80000000\n\t"
	".word	0x0000008A\n\t"
	".word	0x00000000\n\t"
	".word	0x00000088\n\t"
	".word	0x00000000\n\t"
	".word	0x80008009\n\t"
	".word	0x00000000\n\t"
	".word	0x8000000A\n\t"
	".word	0x00000000\n\t"
	".word	0x8000808B\n\t"
	".word	0x00000000\n\t"
	".word	0x0000008B\n\t"
	".word	0x80000000\n\t"
	".word	0x00008089\n\t"
	".word	0x80000000\n\t"
	".word	0x00008003\n\t"
	".word	0x80000000\n\t"
	".word	0x00008002\n\t"
	".word	0x80000000\n\t"
	".word	0x00000080\n\t"
	".word	0x80000000\n\t"
	".word	0x0000800A\n\t"
	".word	0x00000000\n\t"
	".word	0x8000000A\n\t"
	".word	0x80000000\n\t"
	".word	0x80008081\n\t"
	".word	0x80000000\n\t"
	".word	0x00008080\n\t"
	".word	0x80000000\n\t"
	".word	0x80000001\n\t"
	".word	0x00000000\n\t"
	".word	0x80008008\n\t"
	".word	0x80000000\n\t"

#undef INVERT_WORDS
#undef KHI_LOAD
#undef KHI_STEP

	);
}

/* see sha3.h */
void
shake_init(shake_context *sc, unsigned size)
{
	sc->rate = 200 - (size_t)(size >> 2);
	sc->dptr = 0;
	memset(sc->A, 0, sizeof sc->A);
}

/* see sha3.h */
void
shake_inject(shake_context *sc, const void *in, size_t len)
{
	size_t dptr, rate;
	const uint8_t *buf;

	dptr = sc->dptr;
	rate = sc->rate;
	buf = in;
	while (len > 0) {
		size_t clen, u;

		clen = rate - dptr;
		if (clen > len) {
			clen = len;
		}
		for (u = 0; u < clen; u ++) {
			size_t v;

			v = u + dptr;
			sc->A[v >> 3] ^= (uint64_t)buf[u] << ((v & 7) << 3);
		}
		dptr += clen;
		buf += clen;
		len -= clen;
		if (dptr == rate) {
			process_block(sc->A);
			dptr = 0;
		}
	}
	sc->dptr = dptr;
}

/* see sha3.h */
void
shake_flip(shake_context *sc)
{
	/*
	 * We apply padding and pre-XOR the value into the state. We
	 * set dptr to the end of the buffer, so that first call to
	 * shake_extract() will process the block.
	 */
	unsigned v;

	v = sc->dptr;
	sc->A[v >> 3] ^= (uint64_t)0x1F << ((v & 7) << 3);
	v = sc->rate - 1;
	sc->A[v >> 3] ^= (uint64_t)0x80 << ((v & 7) << 3);
	sc->dptr = sc->rate;
}

/* see sha3.h */
void
shake_extract(shake_context *sc, void *out, size_t len)
{
	size_t dptr, rate;
	uint8_t *buf;

	dptr = sc->dptr;
	rate = sc->rate;
	buf = out;
	while (len > 0) {
		size_t clen;

		if (dptr == rate) {
			process_block(sc->A);
			dptr = 0;
		}
		clen = rate - dptr;
		if (clen > len) {
			clen = len;
		}
		len -= clen;
		while (clen -- > 0) {
			*buf ++ = sc->A[dptr >> 3] >> ((dptr & 7) << 3);
			dptr ++;
		}
	}
	sc->dptr = dptr;
}

/* see sha3.h */
void
sha3_init(sha3_context *sc, unsigned size)
{
	shake_init(sc, size);
}

/* see sha3.h */
void
sha3_update(sha3_context *sc, const void *in, size_t len)
{
	shake_inject(sc, in, len);
}

/* see sha3.h */
void
sha3_close(sha3_context *sc, void *out)
{
	unsigned v;
	uint8_t *buf;
	size_t u, len;

	/*
	 * Apply padding. It differs from the SHAKE padding in that
	 * we append '01', not '1111'.
	 */
	v = sc->dptr;
	sc->A[v >> 3] ^= (uint64_t)0x06 << ((v & 7) << 3);
	v = sc->rate - 1;
	sc->A[v >> 3] ^= (uint64_t)0x80 << ((v & 7) << 3);

	/*
	 * Process the padded block.
	 */
	process_block(sc->A);

	/*
	 * Write output. Output length (in bytes) is obtained from the rate.
	 */
	buf = out;
	len = (200 - sc->rate) >> 1;
	for (u = 0; u < len; u ++) {
		buf[u] = sc->A[u >> 3] >> ((u & 7) << 3);
	}
}
