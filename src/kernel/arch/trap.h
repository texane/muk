/*
** Made by texane <texane@gmail.com>
** 
** Started on  Wed Aug 15 21:50:25 2007 texane
** Last update Wed Aug 15 22:09:19 2007 texane
*/


#ifndef TRAP_H_INCLUDED
# define TRAP_H_INCLUDED


typedef enum
  {
    TRAP_ID_TIMER = 0,
    TRAP_ID_KEYBOARD,
    TRAP_ID_PAGEFAULT,
    TRAP_ID_INVALID
  } trap_id_t;


typedef void (*trap_handler_t)(void);


int trap_init(void);
int trap_add_handler(trap_id_t, trap_handler_t);
int trap_remove_handler(trap_id_t, trap_handler_t);
void trap_debug(void);


#endif /* ! TRAP_H_INCLUDED */
