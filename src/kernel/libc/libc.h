/*
** Made by texane <texane@gmail.com>
** 
** Started on  Wed Aug 15 23:36:00 2007 texane
** Last update Sun Sep  9 15:10:23 2007 texane
*/


#ifndef LIBC_H_INCLUDED
# define LIBC_H_INCLUDED


void memset(void*, unsigned char, unsigned int);
void memcpy(void*, const void*, unsigned int);
int memcmp(const unsigned char*, const unsigned char*, unsigned int);
void printf (const char *format, ...);
void cls(void);
void itoa(char*, int, int);
void putchar(int);



#endif /* ! LIBC_H_INCLUDED */
