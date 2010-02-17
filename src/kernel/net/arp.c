/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sun Aug 26 20:39:32 2007 texane
** Last update Tue Nov 20 00:03:59 2007 texane
*/



#include "../debug/debug.h"
#include "../sys/types.h"
#include "../libc/libc.h"
#include "../arch/cpu.h"
#include "../mm/mm.h"
#include "../net/net.h"


/* arp table
 */

static arp_entry_t* g_arp_entries = null;

void arp_foreach_entry(int (*f)(arp_entry_t*, void*), void* p)
{
  arp_entry_t* pos;

  if (is_null(f))
    return ;

  for (pos = g_arp_entries; !is_null(pos); pos = pos->next)
    if ((pos->bits & ARP_ENTRY_INUSE) && f(pos, p))
      return ;
}


static int print_entry(arp_entry_t* e, void* p)
{
  serial_printl("[+] { %x:%x:%x:%x:%x:%x <-> %x }\n",
		e->hw_addr[0], e->hw_addr[1], e->hw_addr[2],
		e->hw_addr[3], e->hw_addr[4], e->hw_addr[5],
		(uint_t)(ntohl(e->proto_addr)));

  return 0;
}

void arp_print_table(void)
{
  arp_foreach_entry(print_entry, null);
}


static error_t arp_alloc_entry(arp_entry_t** e)
{
  arp_entry_t* pos;

  *e = null;

  /* find unused entry */
  for (pos = g_arp_entries; !is_null(pos); pos = pos->next)
    {
      if ((pos->bits & ARP_ENTRY_INUSE) == 0)
	{
	  pos->bits |= ARP_ENTRY_INUSE;
	  *e = pos;
	  return ERROR_SUCCESS;
	}
    }

  /* not found, allocate new one */
  *e = mm_alloc(sizeof(arp_entry_t));
  if (is_null(*e))
    return ERROR_NO_MEMORY;
  (*e)->next = null;
  (*e)->bits = ARP_ENTRY_INUSE;
  (*e)->proto_addr = IP_ADDR_ANY;
  memset((*e)->hw_addr, 0, ETH_ADDR_LEN);

  return ERROR_SUCCESS;
}


static void arp_free_entry(arp_entry_t* e)
{
  e->bits &= ~ARP_ENTRY_INUSE;
}


error_t arp_add_entry(uchar_t* hw_addr, ulong_t proto_addr)
{
  error_t err;
  arp_entry_t* ent;

  /* allocate */
  err = arp_alloc_entry(&ent);
  if (error_is_failure(err))
    return err;

  /* fill */
  ent->proto_addr = proto_addr;
  memcpy(ent->hw_addr, hw_addr, sizeof(ent->hw_addr));

  /* link */
  ent->next = g_arp_entries;
  g_arp_entries = ent;

  return ERROR_SUCCESS;
}


error_t arp_find_entry_by_proto(ulong_t proto_addr, arp_entry_t** e)
{
  arp_entry_t* pos;

  for (pos = g_arp_entries; pos; pos = pos->next)
    {
      if ((pos->bits & ARP_ENTRY_INUSE) && (pos->proto_addr == proto_addr))
	{
	  *e = pos;
	  return ERROR_SUCCESS;
	}
    }

  return ERROR_NOT_FOUND;
}


error_t arp_find_entry_by_hw(uchar_t* hw_addr, arp_entry_t** e)
{
  arp_entry_t* pos;

  /* find unused entry */
  for (pos = g_arp_entries; pos; pos = pos->next)
    {
      if ((pos->bits & ARP_ENTRY_INUSE) && !memcmp(pos->hw_addr, hw_addr, ETH_ADDR_LEN))
	{
	  *e = pos;
	  return ERROR_SUCCESS;
	}
    }

  return ERROR_NOT_FOUND;
}


error_t arp_del_entry_by_hw(uchar_t* hw_addr)
{
  error_t err;
  arp_entry_t* ent;

  err = arp_find_entry_by_hw(hw_addr, &ent);
  if (error_is_failure(err))
    return err;

  arp_free_entry(ent);

  return ERROR_SUCCESS;
}


error_t arp_del_entry_by_proto(ulong_t proto_addr)
{
  error_t err;
  arp_entry_t* ent;

  err = arp_find_entry_by_proto(proto_addr, &ent);
  if (error_is_failure(err))
    return err;

  arp_free_entry(ent);

  return ERROR_SUCCESS;
}



/* initialize
 */

error_t arp_initialize(void)
{
  g_arp_entries = null;

  return ERROR_SUCCESS;
}



/* process arp packet
 */

inline static error_t build_arp_query(uchar_t* hw_local,
				      ulong_t proto_local,
				      ulong_t proto_target,
				      uchar_t* buf,
				      size_t* size)
{
  /* who-has */

  arp_hdr_t* arp;

  /* arp header */
  arp = (arp_hdr_t*)buf;
  memset(arp, 0, sizeof(arp_hdr_t));
  arp->hw_type = htons(ARP_HW_ETHER);
  arp->proto_type = htons(ETH_PROTO_IP);
  arp->hw_len = ETH_ADDR_LEN;
  arp->proto_len = IP_ADDR_LEN;
  arp->opcode = ntohs(ARP_OP_REQUEST);

  buf += sizeof(arp_hdr_t);

  /* arp query */
  memcpy(buf, hw_local, ETH_ADDR_LEN);
  buf += ETH_ADDR_LEN;
  memcpy(buf, &proto_local, IP_ADDR_LEN);
  buf += IP_ADDR_LEN;
  /* target was me */
  memset(buf, 0, ETH_ADDR_LEN);
  buf += ETH_ADDR_LEN;
  memcpy(buf, &proto_target, IP_ADDR_LEN);
  buf += IP_ADDR_LEN;

  *size = sizeof(arp_hdr_t) + (IP_ADDR_LEN + ETH_ADDR_LEN) * 2;

  return ERROR_SUCCESS;
}


inline static error_t build_arp_reply(const uchar_t* hw_target, const uchar_t* hw_sender, ulong_t proto_sender, uchar_t* buf, size_t* size)
{
  /* is-at */

  arp_hdr_t* arp;

  arp = (arp_hdr_t*)buf;
  memset(arp, 0, sizeof(arp_hdr_t));
  arp->hw_type = htons(ARP_HW_ETHER);
  arp->proto_type = htons(ETH_PROTO_IP);
  arp->hw_len = ETH_ADDR_LEN;
  arp->proto_len = IP_ADDR_LEN;
  arp->opcode = ntohs(ARP_OP_REPLY);

  buf += sizeof(arp_hdr_t);

  memcpy(buf, hw_sender, ETH_ADDR_LEN);
  buf += ETH_ADDR_LEN;
  memcpy(buf, &proto_sender, IP_ADDR_LEN);
  buf += IP_ADDR_LEN;
  memcpy(buf, hw_target, ETH_ADDR_LEN);
  buf += ETH_ADDR_LEN;
  memset(buf, 0xff, IP_ADDR_LEN);
  buf += IP_ADDR_LEN;

  *size = sizeof(arp_hdr_t) + 2 * IP_ADDR_LEN + 2 * ETH_ADDR_LEN;

  return ERROR_SUCCESS;
}


inline static error_t process_arp_query(struct net_if* net_if,
					const arp_hdr_t* arp,
					struct net_buf* nb_query)
{
  /* who-has */

  size_t min;
  size_t size;
  uchar_t* buf;
  uint32_t proto_local;
  uint_t proto_target;

  /* read who-has query */
  net_buf_read(nb_query, &buf, &size);
  min = arp->hw_len * 2 + arp->proto_len * 2;
  if (size < min)
    return ERROR_INVALID_SIZE;
  net_buf_advance(nb_query, min);

  /* get local address */
  net_if_get_if_addr(net_if, &proto_local);
  proto_local = htonl(proto_local);

  /* match against local net address */
  proto_target = *(ulong_t*)(buf + arp->hw_len + arp->proto_len + arp->hw_len);
  if (proto_target != proto_local)
    return ERROR_NOT_FOUND;
  
  /* respond to the query */
  {
    /* fixme: reuse the request net buffer
     */

    net_buf_t* nb_reply;
    eth_hdr_t* eth;
    uchar_t hw_local[ETH_ADDR_LEN];

    net_if_get_hw_addr(net_if, hw_local);

    buf = mm_alloc(sizeof(eth_hdr_t) + sizeof(arp_hdr_t) + min);
    if (buf != NULL)
      {
	eth = (eth_hdr_t*)buf;
	memcpy(eth->dest, nb_query->hw_src, ETH_ADDR_LEN);
	memcpy(eth->src, hw_local, ETH_ADDR_LEN);
	eth->type = htons(ETH_PROTO_ARP);

	build_arp_reply(eth->dest, hw_local, proto_local, buf + sizeof(eth_hdr_t), &size);
	net_buf_alloc(&nb_reply);
	nb_reply->buf = buf;
	nb_reply->size = sizeof(eth_hdr_t) + size;
	net_if_send(net_if, nb_reply);
	net_buf_free(nb_reply);
      }
  }

  return ERROR_SUCCESS;
}


inline static error_t process_arp_reply(struct net_if* net_if,
					const arp_hdr_t* arp,
					struct net_buf* nb)
{
  /* is-at */

  error_t err;
  size_t min;
  size_t size;
  uchar_t* buf;
  uchar_t* hw_sender;
  ulong_t proto_sender;
  arp_entry_t* ent;

  net_buf_read(nb, &buf, &size);
  min = arp->hw_len * 2 + arp->proto_len * 2;
  if (size < min)
    return ERROR_INVALID_SIZE;

  /* add arp entry */
  hw_sender = buf;
  proto_sender = *(ulong_t*)(buf + arp->hw_len);

  /* address already mapped */
  err = arp_find_entry_by_proto(proto_sender, &ent);
  if (error_is_success(err))
    memcpy(ent->hw_addr, hw_sender, ETH_ADDR_LEN);
  else
    err = arp_add_entry(hw_sender, proto_sender);

  return err;
}


error_t arp_recv(struct net_if* net_if, struct net_buf* nb)
{
  arp_hdr_t* arp;
  error_t err;
  uchar_t* buf;
  size_t size;

  net_buf_read(nb, &buf, &size);
  if (size < sizeof(arp_hdr_t))
    return ERROR_INVALID_SIZE;
  net_buf_advance(nb, sizeof(arp_hdr_t));

  arp = (arp_hdr_t*)buf;

  /* hw address type not supported
   */
  if (ntohs(arp->hw_type) != ARP_HW_ETHER)
    return ERROR_NOT_SUPPORTED;

  /* net address type not supported
   */
  if (ntohs(arp->proto_type) != ETH_PROTO_IP)
    return ERROR_NOT_SUPPORTED;

  err = ERROR_SUCCESS;

  switch (ntohs(arp->opcode))
    {
    case ARP_OP_REQUEST:
      err = process_arp_query(net_if, arp, nb);
      break;

    case ARP_OP_REPLY:
      process_arp_reply(net_if, arp, nb);
      break;

    default:
      err = ERROR_NOT_IMPLEMENTED;
      break;
    }

  return err;
}



/* send a who-has query
 */

error_t arp_resolve(struct net_if* net_if, ulong_t proto_addr)
{
  arp_entry_t* ent;
  net_buf_t* nb;
  error_t err;
  uchar_t* buf;
  size_t size;
  eth_hdr_t* eth;
  uint32_t inaddr;
  uchar_t hw_local[ETH_ADDR_LEN];

  /* lookup entry */
  err = arp_find_entry_by_proto(proto_addr, &ent);
  if (error_is_success(err))
    return ERROR_SUCCESS;

  /* net buffer */
  err = net_buf_alloc(&nb);
  if (error_is_failure(err))
    return err;

  size = sizeof(eth_hdr_t) + sizeof(arp_hdr_t) + (IP_ADDR_LEN + ETH_ADDR_LEN) * 2;
  buf = mm_alloc(size);
  if (buf == NULL)
    {
      net_buf_free(nb);
      return ERROR_NO_MEMORY;
    }

  /* local hw address */
  net_if_get_hw_addr(net_if, hw_local);

  /* ethernet header */
  eth = (eth_hdr_t*)buf;
  eth->type = htons(ETH_PROTO_ARP);
  memset(eth->dest, 0xff, ETH_ADDR_LEN);
  memcpy(eth->src, hw_local, ETH_ADDR_LEN);

  /* arp query */
  net_if_get_if_addr(net_if, &inaddr);
  build_arp_query(hw_local,
		  htonl(inaddr),
		  proto_addr,
		  buf + sizeof(eth_hdr_t),
		  &size);
  size += sizeof(eth_hdr_t);
  net_buf_append(nb, buf, size);

  err = net_if_send(net_if, nb);
  net_buf_free(nb);

  return err;
}
