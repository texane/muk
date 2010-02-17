/*
** Made by texane <texane@gmail.com>
** 
** Started on  Tue Dec 11 02:31:11 2007 texane
** Last update Wed Dec 12 03:23:05 2007 texane
*/



#ifndef KEYBOARD_H_INCLUDED
# define KEYBOARD_H_INCLUDED



#include "../sys/types.h"



struct event;



error_t keyboard_initialize(void);
error_t keyboard_release(void);
error_t keyboard_read(uint8_t*, uint32_t, uint32_t*);
error_t keyboard_putc(uint8_t);
error_t keyboard_flush(void);



#endif /* ! KEYBOARD_H_INCLUDED */
