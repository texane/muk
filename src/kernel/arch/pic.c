/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sat Aug 25 18:09:08 2007 texane
** Last update Thu Sep 13 01:12:03 2007 texane
*/



/* implement the 8259a pic
 */

/* notes
   -> irr: stores all the int levels that are to be served
   -> isr: stores all the int levels that are being served
   -> imr: int lines to be masked
   -> icw: initialization command word
   -> ocw: operation command word
 */



#include "../debug/debug.h"
#include "../libc/libc.h"
#include "../arch/cpu.h"
#include "../arch/pic.h"
#include "../arch/idt.h"



static void i8259a_initialize(void)
{
#define PIC_MASTER 0x20
#define PIC_SLAVE 0xa0
  cpu_outb(PIC_MASTER, 0x11);
  cpu_outb(PIC_SLAVE, 0x11);

  cpu_outb(PIC_MASTER + 1, 0x20);
  cpu_outb(PIC_SLAVE + 1, 0x28);

  cpu_outb(PIC_MASTER + 1, 0x4);  
  cpu_outb(PIC_SLAVE + 1, 0x2);

  cpu_outb(PIC_MASTER + 1, 0x1);
  cpu_outb(PIC_SLAVE + 1, 0x1);

  cpu_outb(PIC_MASTER + 1, 0xfb);
  cpu_outb(PIC_SLAVE + 1, 0xff);
}


static void i8259a_enable_irq(int i)
{
  if (i < 8)
    cpu_outb(PIC_MASTER + 1, cpu_inb(PIC_MASTER + 1) & ~(1 << (i)));
  else
    cpu_outb(PIC_SLAVE + 1, cpu_inb(PIC_SLAVE + 1) & ~(1 << (i - 8)));
}


static void i8259a_disable_irq(int i)
{
  if (i < 8)
    cpu_outb(PIC_MASTER + 1, cpu_inb(PIC_MASTER + 1) | (1 << (i)));
  else
    cpu_outb(PIC_SLAVE + 1, cpu_inb(PIC_SLAVE + 1) | (1 << (i - 8)));
}


int pic_initialize(void)
{
  i8259a_initialize();

  return 0;
}


int pic_enable_irq(int i, void (*h)(void))
{
  idt_set_handler(i + 32, h);
  i8259a_enable_irq(i);
 
  return 0;
}


int pic_disable_irq(int i)
{
  i8259a_disable_irq(i);
  idt_set_handler(i + 32, null);

  return 0;
}


void pic_ack(int i)
{
  uchar_t mask;

  /* not initiated by the pic
   */
  if (i < 32)
    return ;

  i -= 32;

  if (i < 8)
    {
      mask = cpu_inb(0x21);
      cpu_outb(0x21, mask);
      cpu_outb(0x20, 0x60 + i);
    }
  else
    {
      mask = cpu_inb(0xa1);
      cpu_outb(0xa1, mask);
      cpu_outb(0xa0, 0x60 + (i & 0x7));
      cpu_outb(0x20, 0x60 + 2);
    }
}
