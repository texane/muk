/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sat Aug 25 18:11:00 2007 texane
** Last update Thu Sep 13 00:52:40 2007 texane
*/



#ifndef PIC_H_INCLUDED
# define PIC_H_INCLUDED


int pic_initialize(void);
int pic_enable_irq(int, void (*)());
int pic_disable_irq(int);
void pic_ack(int);


#endif /* ! PIC_H_INCLUDED */
