#include <stddef.h>
#include <stdint.h>

#include "sysinc/samd20e18.h"

int main(void);

/* =================================================================== */

/*
 * Disable IRQ.
 */
static void
cli(void)
{
	__DMB();
	__disable_irq();
}

/*
 * Enabled IRQ.
 */
static void
sei(void)
{
	__enable_irq();
	__DMB();
}

/*
 * Segment boundary symbols (defined in the linker script).
 */
extern uint32_t _sfixed;
extern uint32_t _efixed;
extern uint32_t _etext;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _sstack;
extern uint32_t _estack;

static void dummy_handler(void);
static void reset_handler(void);
/*
static void tc0_handler(void);
*/

/*
 * Exception table. Some are zeros (crashes if the IRQ happens); others
 * just point to a do-nothing function.
 */
__attribute__ ((section(".vectors")))
const void *const exception_table[] = {
	&_estack,                   /* initial stack pointer */

	/* Cortex-M handlers */
	(void *)&reset_handler,     /* reset */
	(void *)&dummy_handler,     /* NMI */
	(void *)&dummy_handler,     /* hard fault */
	0,                          /* reserved (M12) */
	0,                          /* reserved (M11) */
	0,                          /* reserved (M10) */
	0,                          /* reserved (M9) */
	0,                          /* reserved (M8) */
	0,                          /* reserved (M7) */
	0,                          /* reserved (M6) */
	(void *)&dummy_handler,     /* SVC */
	0,                          /* reserved (M4) */
	0,                          /* reserved (M3) */
	(void *)&dummy_handler,     /* pend SV */
	(void *)&dummy_handler,     /* system tick */

	/* Peripheral handlers */
	(void *)&dummy_handler,     /*  0 power manager */
	(void *)&dummy_handler,     /*  1 system control */
	(void *)&dummy_handler,     /*  2 watchdog timer */
	(void *)&dummy_handler,     /*  3 real-time counter */
	(void *)&dummy_handler,     /*  4 external interrupt controller */
	(void *)&dummy_handler,     /*  5 NVRAM controller */
	(void *)&dummy_handler,     /*  6 event system interface */
	(void *)&dummy_handler,     /*  7 SERCOM0 */
	(void *)&dummy_handler,     /*  8 SERCOM1 */
	(void *)&dummy_handler,     /*  9 SERCOM2 */
	(void *)&dummy_handler,     /* 10 SERCOM3 */
	0,                          /* 11 reserved */
	0,                          /* 12 reserved */
	(void *)&dummy_handler,     /* 13 basic timer counter 0 */
	(void *)&dummy_handler,     /* 14 basic timer counter 1 */
	(void *)&dummy_handler,     /* 15 basic timer counter 2 */
	(void *)&dummy_handler,     /* 16 basic timer counter 3 */
	(void *)&dummy_handler,     /* 17 basic timer counter 4 */
	(void *)&dummy_handler,     /* 18 basic timer counter 5 */
	0,                          /* 19 reserved */
	0,                          /* 20 reserved */
	(void *)&dummy_handler,     /* 21 analog to digital converter */
	(void *)&dummy_handler,     /* 22 analog comparator */
	(void *)&dummy_handler,     /* 23 digital to analog converter */
	(void *)&dummy_handler,     /* 24 touch controller */
};

static void
clock_init(void)
{
	uint32_t x;

	// Set the NVM controller to use zero wait state when reading
	NVMCTRL.CTRLB = (NVMCTRL.CTRLB & ~NVMCTRL_CTRLB_RWS_Msk)
		| NVMCTRL_CTRLB_RWS(0);

	// OSC8M: leave calibration bits, set prescaler to 1, enable.
	// This should clock the main CPU at 8 MHz.
	x = SYSCTRL.OSC8M;
	x &= 0xFFFF0000u;
	x |= SYSCTRL_OSC8M_PRESC(0) | SYSCTRL_OSC8M_ENABLE;
	SYSCTRL.OSC8M = x;

	// Wait for GCLK sync
	while (GCLK.STATUS & GCLK_STATUS_SYNCBUSY);
	// Set GCLK Generator 0 prescaler to 1
	GCLK.GENDIV = GCLK_GENDIV_ID_GCLK0 | GCLK_GENDIV_DIV(0);
	// Wait for GCLK sync
	while (GCLK.STATUS & GCLK_STATUS_SYNCBUSY);
	// Configure GCLK Generator 0 (Main clock) to use the OSC8M output.
	GCLK.GENCTRL = GCLK_GENCTRL_ID_GCLK0
		| GCLK_GENCTRL_SRC_OSC8M
		| GCLK_GENCTRL_IDC
		| GCLK_GENCTRL_GENEN;
}

static void
gpio_init(void)
{
	// Set LED0 to output
	PORTA.DIRSET = PORT_PA14;
	// Set peripheral multiplexing for SERCOM3
	PORTA.PMUX[12] = PORT_PMUX_PMUXO_C | PORT_PMUX_PMUXE_C;
	// Configure PA24 and PA25
	PORTA.PINCFG[24] = PORT_PINCFG_INEN | PORT_PINCFG_PMUXEN;
	PORTA.PINCFG[25] = PORT_PINCFG_INEN | PORT_PINCFG_PMUXEN;
}

static void
usart_init(void)
{
	// Set the SERCOM3 bit in the APBC mask
	PM.APBCMASK |= PM_APBCMASK_SERCOM3;
	// Wait for GCLK sync
	while (GCLK.STATUS & GCLK_STATUS_SYNCBUSY);
	// Configure SERCOM3 core and slow clocks on generator 0 (8 MHz)
	GCLK.CLKCTRL = GCLK_CLKCTRL_CLKEN
		| GCLK_CLKCTRL_ID_SERCOM3_CORE | GCLK_CLKCTRL_GEN_GCLK0;
	GCLK.CLKCTRL = GCLK_CLKCTRL_CLKEN
		| GCLK_CLKCTRL_ID_SERCOMX_SLOW | GCLK_CLKCTRL_GEN_GCLK0;
	// Set uart baud register for 115200bps
	SERCOM3.USART.BAUD = 50436;
	// Wait for SERCOM synchronization
	while (SERCOM3.USART.STATUS & SERCOM_USART_STATUS_SYNCBUSY);
	// Enable transmit and receive
	SERCOM3.USART.CTRLB = SERCOM_USART_CTRLB_RXEN | SERCOM_USART_CTRLB_TXEN;
	// Wait for SERCOM synchronization
	while (SERCOM3.USART.STATUS & SERCOM_USART_STATUS_SYNCBUSY);
	// Set data order, RX/TX mux, standby mode, and enable SERCOM3
	SERCOM3.USART.CTRLA = SERCOM_USART_CTRLA_DORD
		| SERCOM_USART_CTRLA_RXPO_PAD3 | SERCOM_USART_CTRLA_TXPO_PAD2
		| SERCOM_USART_CTRLA_RUNSTDBY
		| SERCOM_USART_CTRLA_MODE_USART_INT_CLK
		| SERCOM_USART_CTRLA_ENABLE;
}

static void
tc_init(void)
{
	// Disable TC0 and TC1
	TC1.COUNT32.CTRLA = 0;
	while (TC1.COUNT32.STATUS & TC_STATUS_SYNCBUSY);
	TC0.COUNT32.CTRLA = 0;
	while (TC0.COUNT32.STATUS & TC_STATUS_SYNCBUSY);
	// Reset TC0 and TC1
	TC1.COUNT32.CTRLA = TC_CTRLA_SWRST;
	while (TC1.COUNT32.STATUS & TC_STATUS_SYNCBUSY);
	TC0.COUNT32.CTRLA = TC_CTRLA_SWRST;
	while (TC0.COUNT32.STATUS & TC_STATUS_SYNCBUSY);
	// Enable APBC clock for TC0 and TC1
	PM.APBCMASK |= PM_APBCMASK_TC0 | PM_APBCMASK_TC1;
	// Wait for GCLK sync
	while (GCLK.STATUS & GCLK_STATUS_SYNCBUSY);
	// Map GCLK Generator 0 as the TC0/TC1 input clock and enable it
	GCLK.CLKCTRL = GCLK_CLKCTRL_GEN_GCLK0
		| GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID_TC0_TC1;
	// Wait for GCLK sync
	while (GCLK.STATUS & GCLK_STATUS_SYNCBUSY);
	// Configure TC0 to synchronize with the GCLK, DIV1 prescaler,
	// match frequency mode and 32bit mode
	TC0.COUNT32.CTRLA = TC_CTRLA_PRESCSYNC_GCLK | TC_CTRLA_PRESCALER_DIV1
		| TC_CTRLA_WAVEGEN_NFRQ | TC_CTRLA_MODE_COUNT32;
	// Wait for TC0 to synchronize
	while (TC0.COUNT32.STATUS & TC_STATUS_SYNCBUSY);
	// Enable the counter
	TC0.COUNT32.CTRLA |= TC_CTRLA_ENABLE;
	// Wait for TC0 to synchronize
	while (TC0.COUNT32.STATUS & TC_STATUS_SYNCBUSY);
	// Start/restart TC0
	TC0.COUNT32.CTRLBSET = TC_CTRLBSET_CMD_RETRIGGER;
	// Wait for TC0 to synchronize
	while (TC0.COUNT32.STATUS & TC_STATUS_SYNCBUSY);

	/*
	// Enable APBC clock for TC0
	PM.APBCMASK |= PM_APBCMASK_TC0;
	// Wait for GCLK sync
	while (GCLK.STATUS & GCLK_STATUS_SYNCBUSY);
	// Map GCLK Generator 0 as the TC0 input clock and enable it
	GCLK.CLKCTRL = GCLK_CLKCTRL_GEN_GCLK0
		| GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID_TC0_TC1;
	// Wait for TC0 to synchronize
	while (TC0.COUNT16.STATUS & TC_STATUS_SYNCBUSY);
	// Configure TC0 to synchronize with the GCLK, DIV1 prescaler,
	// match frequency mode, 16bit mode, and enable it
	TC0.COUNT16.CTRLA = TC_CTRLA_PRESCSYNC_GCLK | TC_CTRLA_PRESCALER_DIV1
		| TC_CTRLA_WAVEGEN_MFRQ | TC_CTRLA_MODE_COUNT16;
	// Wait for TC0 to synchronize
	while (TC0.COUNT16.STATUS & TC_STATUS_SYNCBUSY);
	// Clear CTRLB
	TC0.COUNT16.CTRLBCLR = 0xFF;
	// Wait for TC0 to synchronize
	while (TC0.COUNT16.STATUS & TC_STATUS_SYNCBUSY);
	// Count up
	TC0.COUNT16.CTRLBSET = TC_CTRLBSET_DIR;
	// Wait for TC0 to synchronize
	while (TC0.COUNT16.STATUS & TC_STATUS_SYNCBUSY);
	// Enable TC overflow interrupts
	TC0.COUNT16.INTENSET = TC_INTENSET_OVF;
	// Enable TC0 IRQ vector
	NVIC_EnableIRQ(TC0_IRQn);
	*/
}

static void
dummy_handler(void)
{
	return;
}

/*
static volatile uint32_t system_ticks;
*/

/*
static void
tc0_handler(void)
{
	system_ticks ++;
	TC0.COUNT8.INTFLAG = TC_INTFLAG_OVF;
}
*/

static void
reset_handler(void)
{
	uint32_t *src, *dst;

	/*
	 * .data is just after .text; this copies it into RAM.
	 */
	src = &_etext;
	dst = &_sdata;
	if (src != dst) {
		while (dst < &_edata) {
			*dst ++ = *src ++;
		}
	}

	/*
	 * Clear .bss.
	 */
	for (dst = &_sbss; dst < &_ebss; dst ++) {
		*dst = 0;
	}

	/*
	 * Set the vector table base address.
	 */
	SCB.VTOR = (((uint32_t)&_sfixed) & SCB_VTOR_TBLOFF_Msk);

	/*
	 * Apparently this is needed to workaround a bug.
	 */
	NVMCTRL.CTRLB = NVMCTRL_CTRLB_MANW;

	/*
	 * System initialization.
	 */
	cli();
	clock_init();
	gpio_init();
	usart_init();
	tc_init();
	sei();

	/*
	 * Start timer. We want an IRQ every 8000 cycles.
	 */
	/*
	system_ticks = 0;
	while (TC0.COUNT16.STATUS & TC_STATUS_SYNCBUSY);
	TC0.COUNT16.CC[0] = 8000;
	while (TC0.COUNT16.STATUS & TC_STATUS_SYNCBUSY);
	TC0.COUNT16.CTRLA |= TC_CTRLA_ENABLE;
	*/

	/*
	 * Call the program main function.
	 */
	main();

	/*
	 * Loop forever after main() has returned.
	 */
	for (;;);
}

/*
 * Get current time (ticks since startup).
 */
uint32_t
get_system_ticks(void)
{
	TC0.COUNT32.READREQ = TC_READREQ_RREQ | TC_COUNT32_COUNT_OFFSET;
	while (TC0.COUNT32.STATUS & TC_STATUS_SYNCBUSY);
	return TC0.COUNT32.COUNT;
	/*
	return system_ticks;
	*/
}

/*
 * Emit a character on the USART.
 */
void
usart_send(uint8_t x)
{
	while (!(SERCOM3.USART.INTFLAG & SERCOM_USART_INTFLAG_DRE));
	// Wait until synchronization is complete
	while (SERCOM3.USART.STATUS & SERCOM_USART_STATUS_SYNCBUSY);
	// Write data to USART module
	SERCOM3.USART.DATA = x;
	// Wait for the byte to be written
	while (!(SERCOM3.USART.INTFLAG & SERCOM_USART_INTFLAG_TXC));
}
