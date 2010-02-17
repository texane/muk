/*
** Made by texane <texane@gmail.com>
** 
** Started on  Mon Nov 12 23:00:18 2007 texane
** Last update Mon Nov 19 03:07:08 2007 texane
*/



#include "../net/net.h"
#include "../sys/types.h"
#include "../debug/debug.h"
#include "../libc/libc.h"
#include "../mm/mm.h"
#include "../task/task.h"
#include "../sched/sched.h"



error_t bootp_recv(struct net_if* net_if,
		   struct net_buf* net_buf,
		   const uchar_t* data,
		   size_t size)
{
  const bootp_hdr_t* hdr;

  TRACE_ENTRY();

  if (size < sizeof(bootp_hdr_t))
    return ERROR_INVALID_SIZE;

  hdr = (const bootp_hdr_t*)data;

  if (hdr->xid != 0x2a2a2a2a)
    return ERROR_SUCCESS;

  serial_printl("bootp\n");
  serial_printl("{\n");
  serial_printl(" .op    : %x\n", hdr->op);
  serial_printl(" .htype : %x\n", hdr->htype);
  serial_printl(" .hlen  : %x\n", hdr->hlen);
  serial_printl(" .hops  : %x\n", hdr->hops);
  serial_printl(" .xid   : %x\n", hdr->xid);
  serial_printl(" .yiaddr: %x\n", hdr->yiaddr);
  serial_printl("}\n");

  net_if_set_if_addr(net_if, ntohl(hdr->yiaddr));

  return ERROR_SUCCESS;
}



error_t bootp_send_request(struct net_if* net_if)
{
  uchar_t* buf;
  eth_hdr_t* eth;
  ip_hdr_t* ip;
  udp_hdr_t* udp;
  bootp_hdr_t* bootp;
  net_buf_t* nb;
  error_t err;

  TRACE_ENTRY();

  err = net_buf_alloc(&nb);
  if (error_is_failure(err))
    return err;

  buf = mm_alloc(sizeof(eth_hdr_t) +
		 sizeof(ip_hdr_t) +
		 sizeof(udp_hdr_t) +
		 sizeof(bootp_hdr_t));
  if (buf == NULL)
    {
      net_buf_free(nb);
      return ERROR_NO_MEMORY;
    }

  /* bootp
   */
  bootp = (bootp_hdr_t*)&buf[sizeof(eth_hdr_t) +
			     sizeof(ip_hdr_t) +
			     sizeof(udp_hdr_t)];
  memset(bootp, 0, sizeof(bootp_hdr_t));
  bootp->op = 1;
  bootp->htype = 1;
  bootp->hlen = 6;
  bootp->xid = 0x2a2a2a2a;
  bootp->secs = htons(10);
  net_if_get_hw_addr(net_if, bootp->chaddr);

  /* udp
   */
  udp = (udp_hdr_t*)&buf[sizeof(eth_hdr_t) + sizeof(ip_hdr_t)];
  udp->dport = htons(UDP_PROTO_BOOTPS);
  udp->sport = htons(UDP_PROTO_BOOTPC);
  udp->length = htons(sizeof(udp_hdr_t) + sizeof(bootp_hdr_t));
  udp->check = 0x0;
  
  /* ip
   */
  ip = (ip_hdr_t*)&buf[sizeof(eth_hdr_t)];
  memset(ip, 0, sizeof(ip_hdr_t));
  ip->ihl = sizeof(ip_hdr_t) / sizeof(uint32_t);
  ip->version = 4;
  ip->ttl = 128;
  ip->saddr = 0x00000000;
  ip->daddr = 0xffffffff;
  ip->proto = IP_PROTO_UDP;
  ip->tot_len = htons(sizeof(ip_hdr_t) +
		      sizeof(udp_hdr_t) +
		      sizeof(bootp_hdr_t));
  ip->check = ip_compute_checksum((uchar_t*)ip,
				  ip->ihl * sizeof(uint32_t));

  /* ethernet
   */
  eth = (eth_hdr_t*)buf;
  memset(eth->dest, 0xff, ETH_ADDR_LEN);
  net_if_get_hw_addr(net_if, eth->src);
  eth->type = htons(ETH_PROTO_IP);

  /* send
   */
  nb->size =
    sizeof(eth_hdr_t) +
    sizeof(ip_hdr_t) +
    sizeof(udp_hdr_t) +
    sizeof(bootp_hdr_t);
  nb->buf = buf;
  err = net_if_send(net_if, nb);
  net_buf_free(nb);

  return err;
}



/* bootp client task
 */

static error_t __attribute__((fastcall)) do_bootp_client(task_t* task)
{
  struct net_sock* net_sock;
  struct net_if* net_if;
  error_t error;
  size_t count;
  bool_t is_done;
  struct sockaddr_in local_addr;
  struct sockaddr_in remote_addr;
  bootp_hdr_t bootp_req;
  bootp_hdr_t bootp_resp;

  TRACE_ENTRY();

  /* get local interface
   */
  net_if = task->param;

  /* create udp socket
   */
  error = net_sock_create_udp(&net_sock);
  if (error_is_failure(error))
    return error;

  /* remote is INADDR_BROADCAST:BOOTPS
   */
  memset(&remote_addr, 0, sizeof(struct sockaddr_in));
  remote_addr.sin_family = AF_INET;
  remote_addr.sin_port = htons(UDP_PROTO_BOOTPS);
  remote_addr.sin_addr.s_addr = INADDR_BROADCAST;

  /* local is INADDR_ANY:BOOTPC
   */
  memset(&local_addr, 0, sizeof(struct sockaddr_in));
  local_addr.sin_family = AF_INET;
  local_addr.sin_port = htons(UDP_PROTO_BOOTPC);
  local_addr.sin_addr.s_addr = INADDR_ANY;

  error = net_sock_bind(net_sock, &local_addr);
  if (error_is_failure(error))
    {
      net_sock_destroy(net_sock);
      return error;
    }

  /* prepare bootp request
   */
  memset(&bootp_req, 0, sizeof(bootp_hdr_t));
  bootp_req.op = 1;
  bootp_req.htype = 1;
  bootp_req.hlen = 6;
  bootp_req.xid = 0x2a2a2a2a;
  bootp_req.secs = htons(10);
  net_if_get_hw_addr(net_if, bootp_req.chaddr);

  /* send bootp request
   */
  is_done = false;
  while (is_done == false)
    {
      error = net_sock_send(net_sock,
			    (const uint8_t*)&bootp_req,
			    sizeof(bootp_req),
			    &remote_addr,
			    &count);
      if (error_is_success(error))
	{
	  is_done = true;

	  error = net_sock_recv(net_sock,
				(uint8_t*)&bootp_resp,
				sizeof(bootp_resp),
				&remote_addr,
				&count);
	  if (error_is_success(error))
	    net_if_set_if_addr(net_if, ntohl(bootp_resp.yiaddr));
	}
      else
	{
	  is_done = true;
	}
    }

  /* destroy socket
   */
  net_sock_destroy(net_sock);

  /* assign static address
   */
  if (error_is_failure(error))
    net_if_set_if_addr(net_if, NET_LOCAL_ADDR);

  return error;
}


error_t bootp_create_client_task(struct net_if* net_if)
{
  struct task* task;

  task = task_create_with_entry(do_bootp_client, net_if);
  if (task == NULL)
    return ERROR_NO_MEMORY;

  task_set_state(task, TASK_STATE_READY);
  task_set_timeslice(task, TASK_SCHED_TIMESLICE);
  sched_add_task(task);

  return ERROR_SUCCESS;
}
