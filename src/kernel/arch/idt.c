/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sun Aug 19 20:05:49 2007 texane
** Last update Wed Dec 12 02:35:19 2007 texane
*/



#include "../libc/libc.h"
#include "../sys/types.h"
#include "../debug/debug.h"
#include "../sched/sched.h"
#include "../arch/idt.h"
#include "../arch/gdt.h"
#include "../arch/apic.h"
#include "../arch/cpu.h"
#include "../drivers/rtl8139.h"



/* interrupt related types
 */

typedef void (*intr_handler_t)(void);


/* globals
 */

static idte_t idt[255];
static intr_handler_t handlers[255] = { NULL, };



/* generate handlers
 */

#if defined(USE_APIC)
# define ACK_PIC "call apic_ack\n\t"
#else
# define ACK_PIC "call pic_ack\n\t"
#endif

#define DEFINE_INTR_VECTOR( nr )				\
extern void intr_vector_ ## nr(void);				\
static void __attribute__((used)) __intr_vector_ ## nr(void)	\
{								\
  __asm__("intr_vector_" #nr ":\n\t"				\
								\
	  /* when no privilege change: eflags, cs, eip */	\
	  /* otherwise:       ss, esp, eflags, cs, eip */	\
								\
	  /* save current task context */			\
	  "pusha\n\t"						\
	  "subl  $2, %%esp\n\t"					\
	  "pushw %%ss\n\t"					\
	  "pushw %%ds\n\t"					\
	  "pushw %%es\n\t"					\
	  "pushw %%fs\n\t"					\
	  "pushw %%gs\n\t"					\
	  "mov %0, %%eax\n\t"					\
								\
	  /* ensure current is not null */			\
	  "orl $0, %%eax\n\t"					\
	  "jz 1f\n\t"						\
	  "mov %%esp, (%%eax)\n\t"				\
	  "1:\n\t"						\
								\
	  "pushl $" #nr "\n\t"					\
	  "call intr_vector_generic\n\t"			\
	  /* ack the pic */  					\
          ACK_PIC						\
	  "addl $4, %%esp\n\t"					\
								\
	  /* switch to the current task stack */		\
	  "orl $0, %0\n\t"					\
          "jz 1f\n\t"						\
	  "movl %0, %%eax\n\t"					\
	  "movl (%%eax), %%esp\n\t"				\
	  "1:"							\
								\
	  /* restore current task context */			\
	  "popw  %%gs\n\t"					\
	  "popw  %%fs\n\t"					\
	  "popw  %%es\n\t"					\
	  "popw  %%ds\n\t"					\
	  "popw  %%ss\n\t"					\
	  "addl  $2, %%esp\n\t"					\
	  "popa\n\t"						\
								\
          /* run the interrupted task */			\
	  "iret\n\t"						\
								\
	  :							\
	  : "m"(g_current_task)					\
	  );							\
}


#define DEFINE_INTR_VECTOR_WITH_CODE( nr )			\
extern void intr_vector_ ## nr(void);				\
static void __attribute__((used)) __intr_vector_ ## nr(void)	\
{								\
  __asm__("intr_vector_" #nr ":\n\t"				\
								\
	  /* remove error code */				\
	  "addl $4, %%esp\n\t"					\
								\
	  /* save current task context */			\
	  "pusha\n\t"						\
	  "subl  $2,%%esp\n\t"					\
	  "pushw %%ss\n\t"					\
	  "pushw %%ds\n\t"					\
	  "pushw %%es\n\t"					\
	  "pushw %%fs\n\t"					\
	  "pushw %%gs\n\t"					\
	  "mov %0, %%eax\n\t"					\
								\
	  /* ensure current is not null */			\
	  "orl $0, %%eax\n\t"					\
	  "jz 1f\n\t"						\
	  "mov %%esp, (%%eax)\n\t"				\
	  "1:\n\t"						\
								\
	  "pushl $" #nr "\n\t"					\
	  "call intr_vector_generic\n\t"			\
	  /* ack the pic */  					\
          ACK_PIC						\
	  "addl $4, %%esp\n\t"					\
								\
	  /* switch to the current task stack */		\
	  "orl $0, %0\n\t"					\
          "jz 1f\n\t"						\
	  "movl %0, %%eax\n\t"					\
	  "movl (%%eax), %%esp\n\t"				\
	  "1:"							\
								\
	  /* restore current task context */			\
	  "popw  %%gs\n\t"					\
	  "popw  %%fs\n\t"					\
	  "popw  %%es\n\t"					\
	  "popw  %%ds\n\t"					\
	  "popw  %%ss\n\t"					\
	  "addl  $2,%%esp\n\t"					\
	  "popa\n\t"						\
								\
          /* run the interrupted task */			\
	  "iret\n\t"						\
								\
	  :							\
	  : "m"(g_current_task)					\
	  );							\
}


#define INTR_VECTOR_NAME( nr )			\
intr_vector_ ## nr


DEFINE_INTR_VECTOR(0);
DEFINE_INTR_VECTOR(1);
DEFINE_INTR_VECTOR(2);
DEFINE_INTR_VECTOR(3);
DEFINE_INTR_VECTOR(4);
DEFINE_INTR_VECTOR(5);
DEFINE_INTR_VECTOR(6);
DEFINE_INTR_VECTOR(7);
DEFINE_INTR_VECTOR_WITH_CODE(8);
DEFINE_INTR_VECTOR(9);
DEFINE_INTR_VECTOR_WITH_CODE(10);
DEFINE_INTR_VECTOR_WITH_CODE(11);
DEFINE_INTR_VECTOR_WITH_CODE(12);
DEFINE_INTR_VECTOR_WITH_CODE(13);
DEFINE_INTR_VECTOR_WITH_CODE(14);
DEFINE_INTR_VECTOR(15);
DEFINE_INTR_VECTOR(16);
DEFINE_INTR_VECTOR_WITH_CODE(17);
DEFINE_INTR_VECTOR(18);
DEFINE_INTR_VECTOR(19);

DEFINE_INTR_VECTOR(32);
DEFINE_INTR_VECTOR(33);
DEFINE_INTR_VECTOR(43);


static void __attribute__((used)) intr_vector_generic(int nr)
{
  if (handlers[nr] != NULL)
    (handlers[nr])();
}


void idt_build_int_gate(idte_t* gate, unsigned int handler)
{
  memset(gate, 0, sizeof(idte_t));

  gate->offset_0 = (handler >>  0) & 0xffff;
  gate->offset_1 = (handler >> 16) & 0xffff;
  gate->selector = GDT_CS_SELECTOR;
  gate->size = 0x0e;
  gate->dpl = 0;
  gate->present = 1;
}


inline static void set_idte(unsigned int index, idte_t* entry)
{
  memcpy(&idt[index], entry, sizeof(idte_t));  
}


inline static void lidtr(addr_t base)
{
  idtr_t idtr;

  idtr.base = (uint_t)base;
  idtr.limit = 255 * sizeof(idte_t) - 1;

  __asm__("lidt %0\n\t"
	  :
	  :"m"(idtr));
}


/* exported
 */
int idt_initialize(void)
{
  memset(idt, 0, sizeof(idt));

  /* intel vectors
   */
  idt_build_int_gate(&idt[0], (uint_t)intr_vector_0);
  idt_build_int_gate(&idt[1], (uint_t)intr_vector_1);
  idt_build_int_gate(&idt[2], (uint_t)intr_vector_2);
  idt_build_int_gate(&idt[3], (uint_t)intr_vector_3);
  idt_build_int_gate(&idt[4], (uint_t)intr_vector_4);
  idt_build_int_gate(&idt[5], (uint_t)intr_vector_5);
  idt_build_int_gate(&idt[6], (uint_t)intr_vector_6);
  idt_build_int_gate(&idt[7], (uint_t)intr_vector_7);
  idt_build_int_gate(&idt[8], (uint_t)intr_vector_8);
  idt_build_int_gate(&idt[9], (uint_t)intr_vector_9);
  idt_build_int_gate(&idt[10], (uint_t)intr_vector_10);
  idt_build_int_gate(&idt[11], (uint_t)intr_vector_11);
  idt_build_int_gate(&idt[12], (uint_t)intr_vector_12);
  idt_build_int_gate(&idt[13], (uint_t)intr_vector_13);

  /* page fault handler
   */
  idt_build_int_gate(&idt[14], (uint_t)intr_vector_14);

  idt_build_int_gate(&idt[15], (uint_t)intr_vector_15);
  idt_build_int_gate(&idt[16], (uint_t)intr_vector_16);
  idt_build_int_gate(&idt[17], (uint_t)intr_vector_17);
  idt_build_int_gate(&idt[18], (uint_t)intr_vector_18);
  idt_build_int_gate(&idt[19], (uint_t)intr_vector_19);

  /* timer handler
   */
  idt_build_int_gate(&idt[32], (uint_t)intr_vector_32);

  /* keyboard vectors
   */
  idt_build_int_gate(&idt[33], (uint_t)intr_vector_33);

  /* network handler
   */
  idt_build_int_gate(&idt[43], (uint_t)intr_vector_43);

  /* store idt
   */
  lidtr((addr_t)&idt);

  return 0;
}


void idt_set_handler(int i, void (*handler)(void))
{
  handlers[i] = handler;
}
