/*
* Serial driver for UART1 on K20
*
* 2014-01-08 @stg, (cc) https://creativecommons.org/licenses/by/3.0/
*/

#include <mchck.h>
#include "serial.h"

#define BUF_SIZE 64 // buffer size

static volatile bool    tx_en = false;
static volatile uint8_t tx_buf[BUF_SIZE];
static volatile uint8_t rx_buf[BUF_SIZE];
static volatile uint8_t tx_buf_head = 0;
static volatile uint8_t tx_buf_tail = 0;
static volatile uint8_t rx_buf_head = 0;
static volatile uint8_t rx_buf_tail = 0;

void serial1_begin(uint32_t baudrate, enum SERIAL_e format) {
#if EXTERNAL_XTAL==32768
  uint32_t divisor = (F_CPU * 4) / baudrate;
#else
  uint32_t divisor = (F_CPU * 2) / baudrate;
#endif
	
	SIM.scgc4.uart1 = 1; 	// TODO: Use bitband for clocks
  SIM.scgc5.portc = 1;
  rx_buf_head = 0;
  rx_buf_tail = 0;
  tx_buf_head = 0;
  tx_buf_tail = 0;
  tx_en = 0;
  
  // TODO: Allow alternate pin locations?
  pin_mode(PIN_PTC3, PIN_MODE_MUX_ALT3); // RX
  pin_mode(PIN_PTC4, PIN_MODE_MUX_ALT3); // TX
  
  UART1.bdh.raw = ((struct UART_BDH_t) {
    .sbrh = (divisor >> 13) & 0x1F
  }).raw;
  UART1.bdl.raw = ((struct UART_BDL_t) {
    .sbrl = (divisor >> 5) & 0xFF
  }).raw;
  UART1.c4.raw = ((struct UART_C4_t) {
    .brfa = divisor & 0x1F
  }).raw;
  UART1.c1.raw = ((struct UART_C1_t) {
    .ilt = 1,
		.pe = (format == SERIAL_8E1
		     ||format == SERIAL_8O1),
		.pt = (format == SERIAL_8O1),
		.m  = (format == SERIAL_8N2
		     ||format == SERIAL_8E1
				 || format == SERIAL_8O1)
  }).raw;
	UART1.c3.raw = ((struct UART_C3_t) {
		.t8 = 1
	}).raw;
  UART1.twfifo = 2;
  UART1.rwfifo = 4;
  UART1.pfifo.raw = ((struct UART_PFIFO_t) {
    .txfe = 1,
    .rxfe = 1
  }).raw;
	UART1.c2.raw  = ((struct UART_C2_t) {
    .te = 1,
    .re = 1,
    .rie = 1,
    .ilie = 1
  }).raw;
  int_enable(IRQ_UART1_status);
}

void serial1_end(void) {
	if(!(SIM.scgc4.uart1)) return;
	while(tx_en);
	int_disable(IRQ_UART1_status);
	UART1.c2.raw = ((struct UART_C2_t) {}).raw;
	rx_buf_head = 0;
	rx_buf_tail = 0;
}

void serial1_put(uint8_t c) {
	uint32_t head;
	if(!(SIM.scgc4.uart1)) return;
	head = tx_buf_head;
	if(++head >= BUF_SIZE) head = 0;
	while(tx_buf_tail == head);
	tx_buf[head] = c;
	tx_en = 1;
	tx_buf_head = head;
	UART1.c2.raw = ((struct UART_C2_t) {
    .te = 1,
    .re = 1,
    .rie = 1,
    .ilie = 1,
	  .tie = 1
	}).raw;
}

int serial1_available(void) {
	uint32_t head, tail;
	head = rx_buf_head;
	tail = rx_buf_tail;
	if (head >= tail) return head - tail;
	return BUF_SIZE + head - tail;
}

uint8_t serial1_peek(void) {
	uint32_t head, tail;
	int c;
	head = rx_buf_head;
	tail = rx_buf_tail;
	if (head == tail) return -1;
	if (++tail >= BUF_SIZE) tail = 0;
	c = rx_buf[tail];
	return c;
}

uint8_t serial1_get(void) {
	uint32_t head, tail;
	int c;
	head = rx_buf_head;
	tail = rx_buf_tail;
	if (head == tail) return -1;
	if (++tail >= BUF_SIZE) tail = 0;
	c = rx_buf[tail];
	rx_buf_tail = tail;
	return c;
}

void __isr_uart1(void) {
	uint32_t head, newhead, tail;
	uint8_t avail;
	uint8_t volatile c;

	if(UART1.s1.rdrf || UART1.s1.idle) {
		avail = UART1.rcfifo;
		if(avail == 0) {
			c = UART1.d;
    	UART1.cfifo.raw = ((struct UART_CFIFO_t) {
    	  .rxflush = 1
    	}).raw;
		} else {
			head = rx_buf_head;
			tail = rx_buf_tail;
			do {
				c = UART1.d;
				newhead = head + 1;
				if (newhead >= BUF_SIZE) newhead = 0;
				if (newhead != tail) {
					head = newhead;
					rx_buf[head] = c;
				}
			} while (--avail > 0);
			rx_buf_head = head;
		}
	}
	if ((UART1.c2.tie) && (UART1.s1.tdre)) {
		head = tx_buf_head;
		tail = tx_buf_tail;
		do {
			if (tail == head) break;
      if (++tail >= BUF_SIZE) tail = 0;
			avail = UART1_S1;
      c = tx_buf[tail];
			UART1.d = c;
		} while (UART1_TCFIFO < 8);
		tx_buf_tail = tail;
		if (UART1.s1.tdre) UART1.c2.raw = UART1.c2.raw = ((struct UART_C2_t) {
		  .te = 1,
		  .re = 1,
		  .rie = 1,
		  .ilie = 1,
		  .tcie = 1
    }).raw;
	}
	if ((UART1.c2.tcie) && (UART1.s1.tc)) {
		tx_en = 0;
		UART1.c2.raw = ((struct UART_C2_t) {
		  .te = 1,
		  .re = 1,
		  .rie = 1,
		  .ilie = 1
    }).raw;
	}
}

// Allows overriding default ISR by sketch without any overhead as calls are optimized away
void isr_uart1() __attribute__ ((weak, alias ("__isr_uart1")));
void UART1_status_Handler(void) {
  isr_uart1();
}
