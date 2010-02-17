/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sun Aug 19 20:04:49 2007 texane
** Last update Sun Aug 26 19:44:09 2007 texane
*/


#ifndef IDT_H_INLCUDED
# define IDT_H_INCLUDED


#include "../sys/types.h"


typedef struct
{
  ushort_t limit : 16;
  uint_t base : 32;
} __attribute__((packed)) idtr_t;


typedef struct
{
  ushort_t offset_0 : 16;
  ushort_t selector : 16;
  uchar_t reserved : 5;
  uchar_t zero : 3;
  uchar_t size : 5;
  uchar_t dpl : 2;
  uchar_t present : 1;
  ushort_t offset_1 : 16;
} __attribute__((packed)) idte_t;


int idt_initialize(void);
void idt_build_int_gate(idte_t*, uint_t);
void idt_set_handler(int, void (*)(void));


#endif /* ! IDT_H_INCLUDED */
