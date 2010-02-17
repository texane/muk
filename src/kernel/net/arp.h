/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sun Aug 26 20:37:54 2007 texane
** Last update Sun Nov 18 03:24:52 2007 texane
*/


#ifndef ARP_H_INCLUDED
# define ARP_H_INCLUDED



#include "../sys/types.h"



struct net_if;
struct net_dev;
struct net_buf;



typedef struct arp_entry
{
  struct arp_entry* next;
#define ARP_ENTRY_INUSE 1
  uchar_t bits;
  ulong_t proto_addr;
  uchar_t hw_addr[6];
} arp_entry_t;



/* opcode ids
 */

#define ARP_OP_REQUEST   1               /* ARP request                  */
#define ARP_OP_REPLY     2               /* ARP reply                    */
#define ARP_OP_RREQUEST  3               /* RARP request                 */
#define ARP_OP_RREPLY    4               /* RARP reply                   */
#define ARP_OP_InREQUEST 8               /* InARP request                */
#define ARP_OP_InREPLY   9               /* InARP reply                  */
#define ARP_OP_NAK       10              /* (ATM)ARP NAK                 */



/* hw ids
 */

#define ARP_HW_NETROM   0               /* from KA9Q: NET/ROM pseudo    */
#define ARP_HW_ETHER    1               /* Ethernet 10Mbps              */
#define ARP_HW_EETHER   2               /* Experimental Ethernet        */
#define ARP_HW_AX25     3               /* AX.25 Level 2                */
#define ARP_HW_PRONET   4               /* PROnet token ring            */
#define ARP_HW_CHAOS    5               /* Chaosnet                     */
#define ARP_HW_IEEE802  6               /* IEEE 802.2 Ethernet/TR/TB    */
#define ARP_HW_ARCNET   7               /* ARCnet                       */
#define ARP_HW_APPLETLK 8               /* APPLEtalk                    */
#define ARP_HW_DLCI     15              /* Frame Relay DLCI             */
#define ARP_HW_ATM      19              /* ATM                          */
#define ARP_HW_METRICOM 23              /* Metricom STRIP (new IANA id) */
#define ARP_HW_IEEE1394 24              /* IEEE 1394 IPv4 - RFC 2734    */
#define ARP_HW_EUI64    27              /* EUI-64                       */
#define ARP_HW_INFINIBAND 32            /* InfiniBand                   */



/* arp header
 */

typedef struct
{
  ushort_t hw_type;
  ushort_t proto_type;
  uchar_t hw_len;
  uchar_t proto_len;
  ushort_t opcode;
} __attribute__((packed)) arp_hdr_t;



error_t arp_initialize(void);
void arp_foreach_entry(int (*)(arp_entry_t*, void*), void*);
void arp_print_table(void);
error_t arp_add_entry(uchar_t*, ulong_t);
error_t arp_find_entry_by_proto(ulong_t, arp_entry_t**);
error_t arp_find_entry_by_hw(uchar_t*, arp_entry_t**);
error_t arp_del_entry_by_proto(ulong_t);
error_t arp_del_entry_by_hw(uchar_t*);
error_t arp_resolve(struct net_if*, ulong_t);
error_t arp_recv(struct net_if*, struct net_buf*);



#endif /* ! ARP_H_INCLUDED */
