/*
** Made by texane <texane@gmail.com>
** 
** Started on  Mon Nov 19 21:13:30 2007 texane
** Last update Mon Nov 19 23:34:05 2007 texane
*/



#ifndef SYS_TIME_H_INCLUDED
# define SYS_TIME_H_INCLUDED



#include "../sched/sched.h"
#include "../sys/types.h"



/* timeval
 */

struct timeval
{
  uint32_t tv_sec;
  uint32_t tv_usec;
};


inline static bool_t timeval_is_infinite(const struct timeval* tm)
{
  return ((tm->tv_sec == ~0) && (tm->tv_usec == ~0));
}


inline static void timeval_make_infinite(struct timeval* tm)
{
  tm->tv_sec = ~0;
  tm->tv_usec = ~0;
}


inline static void timeval_to_ticks(const struct timeval* tm, uint32_t* ticks)
{
  *ticks =
    tm->tv_sec * SCHED_TIMER_FREQ +
    (tm->tv_usec * SCHED_TIMER_FREQ) / 1000000;
}


inline static void ticks_to_timeval(uint32_t ticks, struct timeval* tm)
{
  tm->tv_sec = ticks / SCHED_TIMER_FREQ;
  if (tm->tv_sec)
    tm->tv_usec = ((ticks % SCHED_TIMER_FREQ) * 1000000) / SCHED_TIMER_FREQ;
}


inline static void timeval_sub_one_tick(struct timeval* tm)
{
#define USEC_PER_TICK (1000000 / SCHED_TIMER_FREQ)

  if (tm->tv_usec > USEC_PER_TICK)
    {
      tm->tv_usec -= USEC_PER_TICK;
      return ;
    }
  else
    {
      if (tm->tv_sec != 0)
	{
	  tm->tv_sec -= 1;
	  tm->tv_usec = 10000000 - (USEC_PER_TICK - tm->tv_usec);
	}
      else
	{
	  tm->tv_sec = 0;
	  tm->tv_usec = 0;
	}
    }
}


inline static bool_t timeval_is_null(const struct timeval* tm)
{
  if (!tm->tv_sec && !tm->tv_usec)
    return true;
  return false;
}



#endif /* ! SYS_TIME_H_INCLUDED */
