/*
** Made by texane <texane@gmail.com>
** 
** Started on  Mon Nov 19 04:46:58 2007 texane
** Last update Mon Nov 19 23:54:19 2007 texane
*/



#ifndef EVENT_H_INCLUDED
# define EVENT_H_INCLUDED



#include "../sys/types.h"



struct event;
struct task;
struct timeval;



extern struct event* g_event_list;



error_t event_initialize(void);
error_t event_create(struct event**);
error_t event_destroy(struct event*);
error_t event_signal(struct event*);
bool_t event_is_signaled(const struct event*);
void event_clear_signal(struct event*);
error_t event_wait(struct event*);
void event_set_timeout(struct event*, const struct timeval*);
void event_get_timeout(struct event*, struct timeval*);
error_t event_signal_tasks(struct event*);
struct event* event_get_next(struct event*);
void event_test(void);



#endif /* ! EVENT_H_INCLUDED */
