/*
** Made by texane <texane@gmail.com>
** 
** Started on  Thu Nov 15 01:22:14 2007 texane
** Last update Tue Nov 20 03:24:35 2007 texane
*/



#ifndef TASK_H_INCLUDED
# define TASK_H_INCLUDED



#include "../sys/types.h"



struct event;
struct task;

typedef uint32_t task_id_t;

typedef error_t (* __attribute__((fastcall)) task_entry_t)(struct task*);

typedef void (*task_dtor_t)(struct task*);



typedef enum
  {
    TASK_STATE_READY = 0,
    TASK_STATE_RUNNING,
    TASK_STATE_WAIT,
    TASK_STATE_INTERRUPTED,
    TASK_STATE_INVALID
  } task_state_t;



#define TASK_SCHED_TIMESLICE 3

#define TASK_STACK_SIZE 0x1000



typedef struct task
{
  /* dont move this one, fixed offset @arch/idt.c */
  uintptr_t sp;
  /* dont move this one, fixed offset @arch/idt.c */

  struct event* event;
  task_id_t id;
  volatile task_state_t state;
  uint32_t timeslice;
  error_t error;
  task_entry_t entry;
  void* param;
  task_dtor_t dtor;
  struct task* next;
  struct task* prev;
} task_t;



error_t task_initialize(void);
task_t* task_create_with_entry(task_entry_t, void*);
void task_destroy(task_t*);
void task_set_dtor(task_t*, task_dtor_t);
error_t task_set_state(task_t*, task_state_t);
error_t task_wait_event(struct task*, struct event*);
void task_switch(struct task*, struct task*);
task_state_t task_get_state(task_t*);
void task_set_param(struct task*, void*);
error_t task_set_timeslice(task_t*, uint32_t);
void task_test(void);



#endif /* ! TASK_H_INCLUDED */
