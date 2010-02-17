/*
** Made by texane <texane@gmail.com>
** 
** Started on  Thu Aug 16 02:06:21 2007 texane
** Last update Thu Nov  8 01:10:55 2007 texane
*/



#include "../debug/debug.h"
#include "../arch/gdt.h"
#include "../sys/types.h"
#include "../libc/libc.h"


#define GDT_NR_ENTRIES 3
static gdte_t gdt[GDT_NR_ENTRIES];


void gdt_build_basic_descriptor(gdte_t* entry)
{
  memset(entry, 0, sizeof(gdte_t));

  /* flat 0-4GB segments, 32 bits */
  entry->limit_0 = 0xffff;
  entry->limit_1 = 0xf;
  entry->g = 1;
  entry->size = 1;
  entry->present = 1;

  /* code and data, 3.5 */
  entry->system = 1;
}


void gdt_build_code_descriptor(gdte_t* entry)
{
  gdt_build_basic_descriptor(entry);
  /* execute, read, conforming */
  entry->type = 0xb;
}


void gdt_build_data_descriptor(gdte_t* entry)
{
  gdt_build_basic_descriptor(entry);
  /* read, write */
  entry->type = 0x3;
}


inline static void enable_gdt(uint32_t base, ushort_t limit)
{
  gdtr_t gdtr;

  /* build the gdtr */
  gdtr.base = base;
  gdtr.limit = limit;

  /* load seg registers */
  asm("lgdt %0\n\t"
      "ljmp %1, $1f\n\t"
      "1:\n\t"
      "movw %2, %%ax\n\t"
      "movw %%ax, %%ds\n\t"
      "movw %%ax, %%ss\n\t"
      "movw %%ax, %%es\n\t"
      "movw %%ax, %%fs\n\t"
      "movw %%ax, %%gs\n\t"
      :
      : "m"(gdtr), "i"(GDT_CS_SELECTOR), "i"(GDT_DS_SELECTOR)
      : "memory", "%eax"
      );
}


int gdt_initialize(void)
{
  /* reset the gdt */
  memset(gdt, 0, sizeof(gdt));

  /* make descriptors */
  gdt_build_code_descriptor(&gdt[1]);
  gdt_build_data_descriptor(&gdt[2]);

  /* load segment registers */
  enable_gdt((uint_t)gdt, sizeof(gdt) - 1);

  return 0;
}
