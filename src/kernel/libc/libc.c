/*
** Made by texane <texane@gmail.com>
** 
** Started on  Wed Aug 15 23:37:15 2007 texane
** Last update Sat Oct 13 02:10:59 2007 texane
*/


#include "libc.h"


void memset(void* addr, unsigned char val, unsigned int size)
{
  unsigned int off;

  for (off = 0; off < size; ++off)
    ((unsigned char*)addr)[off] = val;
}


void memcpy(void* dst, const void* src, unsigned int size)
{
  unsigned int off;

  for (off = 0; off < size; ++off)
    ((unsigned char*)dst)[off] = ((const unsigned char*)src)[off];
}


int memcmp(const unsigned char* a, const unsigned char* b, unsigned int len)
{
  unsigned int off;

  off = 0;
  while (*a == *b)
    {
      ++off;
      if (off == len)
	return 0;
      ++a;
      ++b;
    }

  return -1;
}



/* Macros.  */

/* Some screen stuff.  */
/* The number of columns.  */
#define COLUMNS			80
/* The number of lines.  */
#define LINES			24
/* The attribute of an character.  */
#define ATTRIBUTE		7
/* The video memory address.  */
#define VIDEO			0xB8000

/* Variables.  */
/* Save the X position.  */
static int xpos;
/* Save the Y position.  */
static int ypos;
/* Point to the video memory.  */
static volatile unsigned char *video;


void cls(void)
{
  int i;

  video = (unsigned char *) VIDEO;
  
  for (i = 0; i < COLUMNS * LINES * 2; i++)
    *(video + i) = 0;

  xpos = 0;
  ypos = 0;
}


/* Put the character C on the screen.  */
void putchar(int c)
{
  if (c == '\n' || c == '\r')
    {
    newline:
      xpos = 0;
      ypos++;
      if (ypos >= LINES)
	ypos = 0;
      return;
    }

  *(video + (xpos + ypos * COLUMNS) * 2) = c & 0xFF;
  *(video + (xpos + ypos * COLUMNS) * 2 + 1) = ATTRIBUTE;

  xpos++;
  if (xpos >= COLUMNS)
    goto newline;
}

void printf(const char *format, ...)
{
  char **arg = (char **) &format;
  int c;
  char buf[20];

  arg++;
  
  while ((c = *format++) != 0)
    {
      if (c != '%')
	putchar (c);
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
		putchar (*p++);
	      break;

	    default:
	      putchar (*((int *) arg++));
	      break;
	    }
	}
    }
}
