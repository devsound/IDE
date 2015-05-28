/*
* Serial driver for UART0 on K20
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

void serial_begin(uint32_t baudrate, enum SERIAL_e format) {
#if EXTERNAL_XTAL==32768
  uint32_t divisor = (F_CPU * 4) / baudrate;
#else
  uint32_t divisor = (F_CPU * 2) / baudrate;
#endif

  SIM.scgc4.uart0 = 1; 	// TODO: Use bitband for clocks
  SIM.scgc5.portd = 1;
  rx_buf_head = 0;
  rx_buf_tail = 0;
  tx_buf_head = 0;
  tx_buf_tail = 0;
  tx_en = 0;
  
  // TODO: Allow alternate pin location PTA1, PTA2!
  pin_mode(PIN_PTD6, PIN_MODE_MUX_ALT3); // RX
  pin_mode(PIN_PTD7, PIN_MODE_MUX_ALT3); // TX
  
  UART0.bdh.raw = ((struct UART_BDH_t) {
    .sbrh = (divisor >> 13) & 0x1F
  }).raw;
  UART0.bdl.raw = ((struct UART_BDL_t) {
    .sbrl = (divisor >> 5) & 0xFF
  }).raw;
  UART0.c4.raw = ((struct UART_C4_t) {
    .brfa = divisor & 0x1F
  }).raw;
  UART0.c1.raw = ((struct UART_C1_t) {
    .ilt = 1,
		.pe = (format == SERIAL_8E1
		     ||format == SERIAL_8O1),
		.pt = (format == SERIAL_8O1),
		.m  = (format == SERIAL_8N2
		     ||format == SERIAL_8E1
				 || format == SERIAL_8O1)
  }).raw;
	UART0.c3.raw = ((struct UART_C3_t) {
		.t8 = 1
	}).raw;
  UART0.twfifo = 2;
  UART0.rwfifo = 4;
  UART0.pfifo.raw = ((struct UART_PFIFO_t) {
    .txfe = 1,
    .rxfe = 1
  }).raw;
	UART0.c2.raw  = ((struct UART_C2_t) {
    .te = 1,
    .re = 1,
    .rie = 1,
    .ilie = 1
  }).raw;
  int_enable(IRQ_UART0_status);
}

void serial_end(void) {
	if(!(SIM.scgc4.uart0)) return;
	while(tx_en);
	int_disable(IRQ_UART0_status);
	UART0.c2.raw = ((struct UART_C2_t) {}).raw;
	rx_buf_head = 0;
	rx_buf_tail = 0;
}

void serial_put(uint8_t c) {
	uint32_t head;
	if(!(SIM.scgc4.uart0)) return;
	head = tx_buf_head;
	if(++head >= BUF_SIZE) head = 0;
	while(tx_buf_tail == head);
	tx_buf[head] = c;
	tx_en = 1;
	tx_buf_head = head;
	UART0.c2.raw = ((struct UART_C2_t) {
    .te = 1,
    .re = 1,
    .rie = 1,
    .ilie = 1,
	  .tie = 1
	}).raw;
}

int serial_available(void) {
	uint32_t head, tail;
	head = rx_buf_head;
	tail = rx_buf_tail;
	if (head >= tail) return head - tail;
	return BUF_SIZE + head - tail;
}

uint8_t serial_peek(void) {
	uint32_t head, tail;
	int c;
	head = rx_buf_head;
	tail = rx_buf_tail;
	if (head == tail) return -1;
	if (++tail >= BUF_SIZE) tail = 0;
	c = rx_buf[tail];
	return c;
}

uint8_t serial_get(void) {
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

/*
void __isr_uart0(void) {
	uint32_t head, newhead = 0, tail;
	uint8_t avail;
	uint8_t volatile c;
  
	if(UART0.s1.rdrf || UART0.s1.idle) {
		avail = UART0.rcfifo;
		c = UART0.d; // Read first byte (or clear IDLE)
		if(!avail) {
		  // No data was available - this is an IDLE interrupt
			if(UART0.sfifo.rxuf) {
			  // RxFIFO underflowed - this is normal, set RXUF to clear
			  UART0.sfifo.raw = ((struct UART_SFIFO_t) {
      	  .rxuf= 1
    	}).raw;
		} else {
        // RxFIFO did not underflow - meaning a byte must have been received
        // after reading RCFIFO but before reading D to clear IDLE. Pass on.
        avail = 1;
      }
		}
		if(avail) {
			head = rx_buf_head; // Fetch temporary copy of head, tail
			tail = rx_buf_tail;
			while(1) {
				newhead = head + 1;
				if (newhead >= BUF_SIZE) newhead = 0;
				if (newhead != tail) {
					head = newhead;
					rx_buf[head] = c;
				}
				if(--avail) c = UART0.d; // Read next byte
				else break; // Done
			}
			rx_buf_head = head; // Push new head to RAM
		}
	}
	if ((UART0.c2.tie) && (UART0.s1.tdre)) {
		head = tx_buf_head;
		tail = tx_buf_tail;
		do {
			if (tail == head) break;
			if (++tail >= BUF_SIZE) tail = 0;
			avail = UART0_S1;
			c = tx_buf[tail];
			UART0.d = c;
		} while (UART0_TCFIFO < 8);
		tx_buf_tail = tail;
		if (UART0.s1.tdre) UART0.c2.raw = UART0.c2.raw = ((struct UART_C2_t) {
		  .te = 1,
		  .re = 1,
		  .rie = 1,
		  .ilie = 1,
		  .tcie = 1
    }).raw;
	}
	if ((UART0.c2.tcie) && (UART0.s1.tc)) {
		tx_en = 0;
		UART0.c2.raw = ((struct UART_C2_t) {
		  .te = 1,
		  .re = 1,
		  .rie = 1,
		  .ilie = 1
    }).raw;
	}
}
*/
void __isr_uart0(void) {
	uint32_t head, newhead, tail;
	uint8_t avail;
	uint8_t volatile c;
  
	if(UART0.s1.rdrf || UART0.s1.idle) {
		avail = UART0.rcfifo;
		if(avail == 0) {
			c = UART0.d;
    	UART0.cfifo.raw = ((struct UART_CFIFO_t) {
    	  .rxflush = 1
    	}).raw;
		} else {
			head = rx_buf_head;
			tail = rx_buf_tail;
			do {
				c = UART0.d;
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
	if ((UART0.c2.tie) && (UART0.s1.tdre)) {
		head = tx_buf_head;
		tail = tx_buf_tail;
		do {
			if (tail == head) break;
			if (++tail >= BUF_SIZE) tail = 0;
			avail = UART0_S1;
			c = tx_buf[tail];
			UART0.d = c;
		} while (UART0_TCFIFO < 8);
		tx_buf_tail = tail;
		if (UART0.s1.tdre) UART0.c2.raw = UART0.c2.raw = ((struct UART_C2_t) {
		  .te = 1,
		  .re = 1,
		  .rie = 1,
		  .ilie = 1,
		  .tcie = 1
    }).raw;
	}
	if ((UART0.c2.tcie) && (UART0.s1.tc)) {
		tx_en = 0;
		UART0.c2.raw = ((struct UART_C2_t) {
		  .te = 1,
		  .re = 1,
		  .rie = 1,
		  .ilie = 1
    }).raw;
	}
}

// Allows overriding default ISR by sketch without any overhead as calls are optimized away
void isr_uart0() __attribute__ ((weak, alias ("__isr_uart0")));
void UART0_status_Handler(void) {
  isr_uart0();
}
