/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sun Sep  9 02:03:45 2007 texane
** Last update Mon Nov 19 02:25:41 2007 texane
*/



#include "../net/net.h"
#include "../net/eth.h"
#include "../libc/libc.h"
#include "../debug/debug.h"
#include "../sys/types.h"
#include "../drivers/rtl8139.h"



/* initialize ethernet protocol
 */

error_t eth_initialize(void)
{
  return ERROR_SUCCESS;
}



/* write ethernet header
 */

void eth_write_header(uchar_t* buf, size_t* size)
{
  eth_hdr_t* eth;

  eth = (eth_hdr_t*)buf;
  memset(eth, 0, sizeof(eth_hdr_t));
  eth->type = htons(ETH_PROTO_IP);
  memset(eth->dest, 0xff, sizeof(eth->dest));

  *size = sizeof(eth_hdr_t);
}



/* ethernet recv operation
 */

error_t eth_recv(struct net_if* net_if, struct net_buf* nb)
{
  eth_hdr_t* eth;
  error_t err;
  size_t size;
  uchar_t* buf;
  uchar_t addr[ETH_ADDR_LEN];

  err = net_buf_read(nb, &buf, &size);
  if (error_is_failure(err))
    return err;

  /* check size */
  if ((size < ETH_MIN_LEN) || (size > ETH_MAX_LEN))
    return ERROR_INVALID_SIZE;

  /* ethernet header */
  eth = (eth_hdr_t*)buf;
  net_buf_advance(nb, sizeof(eth_hdr_t));

  /* check destination */
  net_if_get_hw_addr(net_if, addr);
  if (memcmp(addr, eth->dest, ETH_ADDR_LEN))
    {
      memset(addr, 0xff, ETH_ADDR_LEN);
      if (memcmp(addr, eth->dest, ETH_ADDR_LEN))
	return ERROR_NOT_FOUND;
    }

  /* meta */
  memcpy(nb->hw_src, eth->src, ETH_ADDR_LEN);
  memcpy(nb->hw_dst, eth->dest, ETH_ADDR_LEN);

  /* handle protocol */
  err = ERROR_SUCCESS;
  switch (ntohs(eth->type))
    {
    case ETH_PROTO_IP:
      err = ip_recv(net_if, nb);
      break;

    case ETH_PROTO_ARP:
      err = arp_recv(net_if, nb);
      break;

    default:
      serial_printl("[?] eth::type == %x\n", ntohs(eth->type));
      err = ERROR_NOT_FOUND;
      break;
    }

  return err;
}



/* ethernet send operation
 */

error_t eth_send(struct net_if* net_if,
		 struct net_buf* net_buf,
		 const uint8_t* dest_addr,
		 uint16_t proto_id)
{
  error_t error;
  eth_hdr_t eth_hdr;

  /* make ethernet headers
   */
  memset(&eth_hdr, 0, sizeof(eth_hdr_t));
  eth_hdr.type = htons(proto_id);
  net_if_get_hw_addr(net_if, eth_hdr.src);
  memcpy(eth_hdr.dest, dest_addr, ETH_ADDR_LEN);

  /* write ethernet headers
   */
  net_buf_rewind(net_buf, sizeof(eth_hdr));
  error = net_buf_write(net_buf, (const uint8_t*)&eth_hdr, sizeof(eth_hdr));
  if (error_is_failure(error))
    return error;

  error = net_if_send(net_if, net_buf);

  return error;
}
