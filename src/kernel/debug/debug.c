/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sat Oct 13 02:10:38 2007 texane
** Last update Sat Oct 13 02:10:44 2007 texane
*/



#include "debug.h"


/* strings
 */

int strlen(const char* s)
{
  int len;

  len = 0;
  while (*s)
    {
      ++s;
      ++len;
    }
  return len;
}

char* strcpy(char* to, const char* from)
{
  char* s;

  s = to;

  while (*from)
    *to++ = *from++;

  *to = 0;

  return s;
}


/* numerics
 */

void itoa(char *buf, int base, int d)
{
  char *p = buf;
  char *p1, *p2;
  unsigned long ud = d;
  int divisor = 10;
  
  /* If %d is specified and D is minus, put `-' in the head.  */
  if (base == 'd' && d < 0)
    {
      *p++ = '-';
      buf++;
      ud = -d;
    }
  else if (base == 'x')
    divisor = 16;

  /* Divide UD by DIVISOR until UD == 0.  */
  do
    {
      int remainder = ud % divisor;
      
      *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    }
  while (ud /= divisor);

  /* Terminate BUF.  */
  *p = 0;
  
  /* Reverse BUF.  */
  p1 = buf;
  p2 = p - 1;
  while (p1 < p2)
    {
      char tmp = *p1;
      *p1 = *p2;
      *p2 = tmp;
      p1++;
      p2--;
    }
}



/* Store the port number of a serial unit.  */
static unsigned short serial_port;

static unsigned short  _serial_iobases [] = { 0x3f8, 0x2f8, 0x3e8, 0x2e8 } ;

#define inb(port)						\
({								\
  unsigned char _v;						\
  __asm__ volatile (						\
	"inb %w1,%0"						\
	:"=a" (_v)						\
	:"Nd" (port)						\
	);							\
  _v;								\
})


#define outb(value, port)					\
  __asm__ volatile (	       					\
	"outb %b0,%w1"						\
	::"a" (value),"Nd" (port)				\
	)							\

#define serial_inb inb
#define serial_outb(port,val) outb(val,port)

int
serial_isready (void)
{
  unsigned char status;

  status = serial_inb (serial_port + UART_LSR);
  return (status & UART_DATA_READY) ? : -1;
}


char
serial_getchar (void)
{
  /* Wait until data is ready.  */
  while ((serial_inb (serial_port + UART_LSR) & UART_DATA_READY) == 0)
    ;

  /* Read and return the data.  */
  return serial_inb (serial_port + UART_RX);
}


/* 0 on success */
int
serial_putchar (char c)
{
  /* Perhaps a timeout is necessary.  */
  int timeout = 10000;

  /* Wait until the transmitter holding register is empty.  */
  while ((serial_inb (serial_port + UART_LSR) & UART_EMPTY_TRANSMITTER) == 0)
    if (--timeout == 0)
      /* There is something wrong. But what can I do?  */
      return -1;

  serial_outb (serial_port + UART_TX, c);
  return 0;
}

void serial_printl(char *format, ...) 
{
  char **arg = (char **) &format;
  int c;
  char buf[20];
  
  arg++;
  

  while ((c = *format++) != 0)
    {
      if (c != '%')
	serial_putchar (c);
      else
	{
	  char *p;
	  
	  c = *format++;
	  switch (c)
	    {
	    case 'd':
	    case 'u':
	    case 'x':
	      itoa (buf, c, *((int *) arg++));
	      p = buf;
	      goto string;
	      break;

	    case 's':
	      p = *arg++;
	      if (! p)
		p = "(null)";

	    string:
	      while (*p)
		serial_putchar (*p++);
	      break;

	    default:
	      serial_putchar (*((int *) arg++));
	      break;
	    }
	}
    }
}

/* 0 On success */
int serial_init (unsigned short unit, unsigned int speed,
		 int word_len, int parity, int stop_bit_len)
{
  unsigned short div = 0;
  unsigned char status = 0;

  if (unit >= 4)
    return -1;
  serial_port = _serial_iobases[unit];
  
  /* Turn off the interrupt.  */
  serial_outb (serial_port + UART_IER, 0);

  /* Set DLAB.  */
  serial_outb (serial_port + UART_LCR, UART_DLAB);
  
  /* Set the baud rate.  */
  if (speed > SERIAL_BAUDRATE_MAX)
    return -1;

  div = SERIAL_BAUDRATE_MAX / speed;
  
  serial_outb (serial_port + UART_DLL, div & 0xFF);
  serial_outb (serial_port + UART_DLH, div >> 8);
  
  /* Set the line status.  */
  status |= parity | word_len | stop_bit_len;
  serial_outb (serial_port + UART_LCR, status);

  /* Enable the FIFO.  */
  serial_outb (serial_port + UART_FCR, UART_ENABLE_FIFO);

  /* Turn on DTR, RTS, and OUT2.  */
  serial_outb (serial_port + UART_MCR, UART_ENABLE_MODEM);

  /* Drain the input buffer.  */
  while (serial_isready () != -1)
    (void) serial_getchar ();
  
  return 0;
}


void hexdump(const uchar_t* buf, size_t size)
{
  size_t i;

  serial_printl("-- %d --\n", size);
  for (i = 0; i < size; ++i)
    {
      if ((!(i % 16)) && i)
	serial_printl("\n");
      serial_printl("%x ", buf[i]);
    }
  serial_printl("\n");
}
