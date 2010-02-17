/*
** Made by texane <texane@gmail.com>
** 
** Started on  Thu Sep 13 01:06:52 2007 texane
** Last update Thu Sep 13 01:21:19 2007 texane
*/



#ifndef BREAKPOINT_H_INCLUDED
# define BREAKPOINT_H_INCLUDED



#include "../sys/types.h"



error_t breakpoint_initialize(void);
error_t breakpoint_cleanup(void);
error_t breakpoint_set(uintptr_t, void (*)(void));
error_t breakpoint_unset(uintptr_t, void (*)(void));



#endif /* ! BREAKPOINT_H_INCLUDED */
