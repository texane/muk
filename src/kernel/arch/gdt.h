/*
** Made by texane <texane@gmail.com>
** 
** Started on  Thu Aug 16 02:08:52 2007 texane
** Last update Wed Aug 22 01:52:16 2007 texane
*/


#ifndef GDT_H_INCLUDED
# define GDT_H_INCLUDED


#include "../sys/types.h"


/* flat segmented model */
#define GDT_CS_SELECTOR (0x1 << 3)
#define GDT_DS_SELECTOR (0x2 << 3)
#define GDT_ES_SELECTOR (0x2 << 3)
#define GDT_FS_SELECTOR (0x2 << 3)
#define GDT_GS_SELECTOR (0x2 << 3)
#define GDT_SS_SELECTOR (0x2 << 3)
#define GDT_NULL_SELECTOR (0x0 << 3)


typedef struct
{
  ushort_t limit : 16;
  uint_t base : 32;
} __attribute__((packed)) gdtr_t;


typedef struct
{
  ushort_t limit_0 : 16;
  ushort_t base_0 : 16;
  uchar_t base_1 : 8;
  uchar_t type : 4;
  uchar_t system : 1;
  uchar_t dpl : 2;
  uchar_t present : 1;
  uchar_t limit_1 : 4;
  uchar_t avail : 1;
  uchar_t lbit : 1;
  uchar_t size : 1;
  uchar_t g : 1;
  uchar_t base_2 : 8;
} __attribute__((packed)) gdte_t;


int gdt_initialize(void);
void gdt_build_basic_descriptor(gdte_t*);
void gdt_build_code_descriptor(gdte_t*);
void gdt_build_data_descriptor(gdte_t*);


#endif /* ! GDT_H_INCLUDED */
