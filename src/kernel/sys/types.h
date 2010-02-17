/*
** Made by texane <texane@gmail.com>
** 
** Started on  Wed Aug 15 19:10:20 2007 texane
** Last update Mon Nov 19 21:16:05 2007 texane
*/



#ifndef TYPES_H_INCLUDED
# define TYPES_H_INCLUDED



typedef unsigned char* addr_t;
typedef unsigned char* paddr_t;
typedef unsigned char* vaddr_t;
typedef unsigned int count_t;


typedef unsigned char uchar_t;
typedef unsigned int uint_t;
typedef unsigned short ushort_t;
typedef unsigned long ulong_t;



/* c99 types
 */

#include "stdint.h"
#include "stdbool.h"

typedef int ssize_t;
typedef unsigned int off_t;
typedef unsigned int size_t;



/* error
 */

typedef enum
{
  ERROR_SUCCESS = 0,
  ERROR_NOT_IMPLEMENTED,
  ERROR_NOT_SUPPORTED,
  ERROR_NOT_FOUND,
  ERROR_NO_MEMORY,
  ERROR_NO_DATA,
  ERROR_INVALID_PROTO,
  ERROR_INVALID_ADDR,
  ERROR_INVALID_SIZE,
  ERROR_ALREADY_EXIST,
  ERROR_ALREADY_USED,
  ERROR_NO_DEVICE,
  ERROR_FAILURE
} error_t;

inline static bool_t error_is_success(error_t e)
{
  if (e == ERROR_SUCCESS)
    return true;
  return false;
}

inline static bool_t error_is_failure(error_t e)
{
  return error_is_success(e) == true ? false : true;
}



/* todos: move into sys/macros.h
 */

#define NULL ((void*)0)
#define null NULL
#define IS_NULL(p) ((p) == NULL)
#define is_null(p) ((p) == NULL)

#define offset_of(t, f) (off_t)(&((t*)null)->f)



#endif /* ! TYPES_H_INCLUDED */
