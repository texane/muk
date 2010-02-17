/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sun Sep  9 12:37:36 2007 texane
** Last update Sun Oct 14 20:58:33 2007 texane
*/


#ifndef DEBUG_H_INCLUDED
# define DEBUG_H_INCLUDED


#include "../sys/types.h"


#define SERIAL_BAUDRATE_MAX 115200

/* Default parameters */
#ifndef DEBUG_SERIAL_PORT
# define DEBUG_SERIAL_PORT 0
#endif
#ifndef DEBUG_SERIAL_SPEED
# define DEBUG_SERIAL_SPEED 9600
#endif

/* The offsets of UART registers.  */
#define UART_TX         0
#define UART_RX         0
#define UART_DLL        0
#define UART_IER        1
#define UART_DLH        1
#define UART_IIR        2
#define UART_FCR        2
#define UART_LCR        3
#define UART_MCR        4
#define UART_LSR        5
#define UART_MSR        6
#define UART_SR         7

/* For LSR bits.  */
#define UART_DATA_READY         0x01
#define UART_EMPTY_TRANSMITTER  0x20

/* The type of parity.  */
#define UART_NO_PARITY          0x00
#define UART_ODD_PARITY         0x08
#define UART_EVEN_PARITY        0x18

/* The type of word length.  */
#define UART_5BITS_WORD 0x00
#define UART_6BITS_WORD 0x01
#define UART_7BITS_WORD 0x02
#define UART_8BITS_WORD 0x03

/* The type of the length of stop bit.  */
#define UART_1_STOP_BIT         0x00
#define UART_2_STOP_BITS        0x04

/* the switch of DLAB.  */
#define UART_DLAB               0x80

/* Enable the FIFO.  */
#define UART_ENABLE_FIFO        0xC7

/* Turn on DTR, RTS, and OUT2.  */
#define UART_ENABLE_MODEM       0x0B

/* Function prototypes.  */

/* 0 On success */
int serial_init (unsigned short unit, unsigned int speed,
		 int word_len, int parity, int stop_bit_len);

/* 0 on success  */
int serial_putchar (char c);
void serial_printl(char *format, ...)
  __attribute__ ((format (printf, 1, 2)));

int serial_isready (void);
char serial_getchar (void);
void hexdump(const uchar_t*, size_t);


#define FATAL() serial_printl("[!] FATAL: %s@%s\n", __FUNCTION__, __FILE__)
#define BUG() serial_printl("[!] BUG: %s@%s\n", __FUNCTION__, __FILE__)
#define TO_REMOVE() serial_printl("[!] TO_REMOVE: %s@%s\n", __FUNCTION__, __FILE__)
#define NOT_IMPLEMENTED() serial_printl("[!] NOT_IMPLEMENTED: %s@%s\n", __FUNCTION__, __FILE__)
#define FIXME() serial_printl("[!] FIXME: %s@%s\n", __FUNCTION__, __FILE__)
#define TRACE_ENTRY() serial_printl("[?] ENTRY: %s@%s\n", __FUNCTION__, __FILE__)
#define TRACE_EXIT() serial_printl("[?] EXIT: %s@%s\n", __FUNCTION__, __FILE__)


#endif /* ! DEUBG_H_INCLUDED */
