/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sat Nov 17 15:41:37 2007 texane
** Last update Sat Nov 17 15:46:20 2007 texane
*/



/* implement tftp client
 */



#include "../net/net.h"
#include "../sys/types.h"



typedef struct
{
  uint32_t tid_local;
  uint32_t tid_remote;
} tftp_con_t;



inline static void tftp_con_reset(tftp_con_t* con)
{
  con->tid_local = 0;
  con->tid_remote = 0;
}



error_t tftp_recv(net_dev_t* dev, net_buf_t* nb)
{
  return ERROR_SUCCESS;
}

