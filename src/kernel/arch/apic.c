/*
** Made by texane <texane@gmail.com>
** 
** Started on  Wed Aug 15 18:39:25 2007 texane
** Last update Sun Aug 26 17:19:01 2007 texane
*/


/* adavanced programmable interrupt controller
 */

/* todos
   -> setup an interrupt handler for the apic internal errors
   -> guess the cpu frequence
 */

/* notes
   . at power reset, apic disabled
   . when software disabled, pending ints are held
   and must be masked or handled by the cpu
   . vector supported by the apic are in the range [16 - 255]
   if int is in this range, an # illegal vector interrupt is raised
   . [16 - 31] is reserved by intel but no exception is raised if used
   by the apic
 */


#include "../arch/apic.h"
#include "../arch/cpu.h"
#include "../arch/msr.h"
#include "../debug/debug.h"
#include "../sys/types.h"


#define apic_base (unsigned char*)0xfee00000


typedef enum
  {
    APIC_REG_ID = 0x20,
    APIC_REG_VERS = 0x30,
    APIC_REG_EOI = 0xb0,
    APIC_REG_SVR = 0xf0,

    APIC_REG_TMR = 0x180,
    APIC_REG_ISR = 0x100,
    APIC_REG_IRR = 0x200,

    APIC_REG_ESR = 0x280,

    /* lvts */
    APIC_REG_LVT_TIMER = 0x320,
    APIC_REG_LVT_TERM_MONITOR = 0x330,
    APIC_REG_LVT_PERF_COUNTER = 0x340,
    APIC_REG_LVT_LINT0 = 0x350,
    APIC_REG_LVT_LINT1 = 0x350,
    APIC_REG_LVT_ERROR = 0x360,

    /* timer */
    APIC_REG_INITIAL_COUNT = 0x380,
    APIC_REG_CURRENT_COUNT = 0x390,
    APIC_REG_DIVIDE_CONFIG = 0x3e0,

    APIC_REG_INVALID
  } apic_reg_t;


/* read a register in apic space
 */
inline static int read_apic_reg(apic_reg_t reg, uint_t* val)
{
  *val = *(unsigned int*)(apic_base + (off_t)reg);
  return 0;
}


/* write a register in apic space
 */
inline static int write_apic_reg(apic_reg_t reg, uint_t val)
{
  *(unsigned int*)(apic_base + (off_t)reg) = val;
  return 0;
}


/* is apic present
 */
inline static bool_t is_present(void)
{
  unsigned int features;

  __asm__("xorl %%eax, %%eax\n\t"
	  "incb %%al\n\t"
	  "cpuid\n\t"
	  "mov %%edx, %0\n\t"
	  : "=m"(features)
	  :
	  : "eax", "ebx", "ecx", "edx");
  
  return (features & (1 << 8)) >> 8;
}


/* is apic enabled
 */
inline static bool_t is_global_enable_flag(void)
{
  uint64_t val;

  msr_read(MSR_REG_IA32_APIC_BASE, &val);
  if (val & (1 << 11))
    return true;
  return false;
}


/* enable apic
 */
inline static void enable(void)
{
  uint_t val;
  
  /* set svr[8] to enable */
  read_apic_reg(APIC_REG_SVR, &val);
  val |= 1 << 8;
  write_apic_reg(APIC_REG_SVR, val);
}


/* disable apic
 */
inline static void disable(void)
{
  uint_t val;

  /* clear svr[8] to disable */
  read_apic_reg(APIC_REG_SVR, &val);
  val &= ~(1 << 8);
  write_apic_reg(APIC_REG_SVR, val);

  /* todo: handle pending ints in irr and isr */
}


/* clear the apic previous ints
 */
static void clear(void)
{
  /* mask interrupt delivering */
  write_apic_reg(APIC_REG_LVT_TIMER, 1 << 16);
  write_apic_reg(APIC_REG_LVT_TERM_MONITOR, 1 << 16);
  write_apic_reg(APIC_REG_LVT_PERF_COUNTER, 1 << 16);
  write_apic_reg(APIC_REG_LVT_LINT0, 1 << 16);
  write_apic_reg(APIC_REG_LVT_LINT1, 1 << 16);
  write_apic_reg(APIC_REG_LVT_ERROR, 1 << 16);
}


/* setup timer lvt
 */
inline static int setup_timer(void)
{
  uint_t val;
  uint_t freq;

  /* one int every (bus_freq / 1)
   */
  freq = cpu_get_freq();
  read_apic_reg(APIC_REG_DIVIDE_CONFIG, &val);
  val = (val & ~0xf) | 0xb;
  write_apic_reg(APIC_REG_DIVIDE_CONFIG, val);
  write_apic_reg(APIC_REG_INITIAL_COUNT, freq);

  /* set a periodic timer (bit 17) at vector
     32 in idt (bits 0-7)
  */
  val = 32 | (1 << 17);
  write_apic_reg(APIC_REG_LVT_TIMER, val);
  
  return 0;
}


/* setup lint0 lvt
 */
inline static int setup_lint0(void)
{
  write_apic_reg(APIC_REG_LVT_LINT1, (1 << 15) | (7 << 8) | 33);

  return 0;
}


/* exported
 */
int apic_initialize(void)
{
  if (is_present() == false)
    return -1;

  /* software enable disable not present */
  if (is_global_enable_flag() == false)
    return false;

  /* has to be in disabled state to be enabled */
  disable();
  clear();
  enable();

  setup_timer();
  setup_lint0();

  return 0;
}


/* exported
 */
int apic_release(void)
{
  disable();
  
  return 0;
}


/* exported
 */
int apic_get_addr(paddr_t* addr)
{
  uint64_t val;

  *addr = null;

  if (msr_read(MSR_REG_IA32_APIC_BASE, &val) == -1)
    return -1;

  /* fixme: address is on 64 36 bits, truncated here
   */
  *addr = (paddr_t)((unsigned int)val & 0xfffff000);

  return 0;
}


/* exported
 */
int apic_set_addr(paddr_t addr)
{
  return -1;
}


/* exported
 */
void apic_ack(void)
{
  /* write the eoi register */
  write_apic_reg(APIC_REG_EOI, 0);
}


/* exported
 */
void apic_debug(void)
{
  paddr_t addr;

  apic_get_addr(&addr);

  serial_printl("apic\n");
  serial_printl("{\n");
  serial_printl(" .base == %x\n", (unsigned int)addr);
  serial_printl("}\n");
}
