/*
** Made by texane <texane@gmail.com>
** 
** Started on  Wed Aug 15 19:30:32 2007 texane
** Last update Wed Aug 15 21:30:18 2007 texane
*/


#ifndef MSR_H_INCLUDED
# define MSR_H_INCLUDED


#include "../sys/types.h"


typedef enum
  {
    MSR_REG_IA32_APIC_BASE = 0x1b,
    MSR_REG_INVALID
  } msr_reg_t;


int msr_read(msr_reg_t, uint64_t*);
int msr_write(msr_reg_t, uint64_t);


#endif /* ! MSR_H_INCLUDED */
