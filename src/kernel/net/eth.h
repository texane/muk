/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sun Sep  9 01:30:48 2007 texane
** Last update Sun Nov 18 17:46:58 2007 texane
*/


#ifndef ETH_H_INCLUDED
# define ETH_H_INCLUDED


#include "../sys/types.h"



struct net_if;
struct net_dev;
struct net_buf;



/* macros
 */

#define ETH_CRC_LEN 4
#define ETH_MIN_LEN 14
#define ETH_MAX_LEN 1518
#define ETH_ADDR_LEN 6



/* protocols
 */

#define ETH_PROTO_LOOP      0x0060          /* Ethernet Loopback packet     */
#define ETH_PROTO_PUP       0x0200          /* Xerox PUP packet             */
#define ETH_PROTO_PUPAT     0x0201          /* Xerox PUP Addr Trans packet  */
#define ETH_PROTO_IP        0x0800          /* Internet Protocol packet     */
#define ETH_PROTO_X25       0x0805          /* CCITT X.25                   */
#define ETH_PROTO_ARP       0x0806          /* Address Resolution packet    */
#define ETH_PROTO_BPQ       0x08FF          /* G8BPQ AX.25 Ethernet Packet  [ NOT AN OFFICIALLY REGISTERED ID ] */
#define ETH_PROTO_IEEEPUP   0x0a00          /* Xerox IEEE802.3 PUP packet */
#define ETH_PROTO_IEEEPUPAT 0x0a01          /* Xerox IEEE802.3 PUP Addr Trans packet */
#define ETH_PROTO_DEC       0x6000          /* DEC Assigned proto           */
#define ETH_PROTO_DNA_DL    0x6001          /* DEC DNA Dump/Load            */
#define ETH_PROTO_DNA_RC    0x6002          /* DEC DNA Remote Console       */
#define ETH_PROTO_DNA_RT    0x6003          /* DEC DNA Routing              */
#define ETH_PROTO_LAT       0x6004          /* DEC LAT                      */
#define ETH_PROTO_DIAG      0x6005          /* DEC Diagnostics              */
#define ETH_PROTO_CUST      0x6006          /* DEC Customer use             */
#define ETH_PROTO_SCA       0x6007          /* DEC Systems Comms Arch       */
#define ETH_PROTO_RARP      0x8035          /* Reverse Addr Res packet      */
#define ETH_PROTO_ATALK     0x809B          /* Appletalk DDP                */
#define ETH_PROTO_AARP      0x80F3          /* Appletalk AARP               */
#define ETH_PROTO_8021Q     0x8100          /* 802.1Q VLAN Extended Header  */
#define ETH_PROTO_IPX       0x8137          /* IPX over DIX                 */
#define ETH_PROTO_IPV6      0x86DD          /* IPv6 over bluebook           */
#define ETH_PROTO_SLOW      0x8809          /* Slow Protocol. See 802.3ad 43B */
#define ETH_PROTO_WCCP      0x883E          /* Web-cache coordination protocol defined in draft-wilson-wrec-wccp-v2-00.txt */
#define ETH_PROTO_PPP_DISC  0x8863          /* PPPoE discovery messages     */
#define ETH_PROTO_PPP_SES   0x8864          /* PPPoE session messages       */
#define ETH_PROTO_MPLS_UC   0x8847          /* MPLS Unicast traffic         */
#define ETH_PROTO_MPLS_MC   0x8848          /* MPLS Multicast traffic       */
#define ETH_PROTO_ATMMPOA   0x884c          /* MultiProtocol Over ATM       */
#define ETH_PROTO_ATMFATE   0x8884          /* Frame-based ATM Transport over Ethernet */
#define ETH_PROTO_AOE       0x88A2          /* ATA over Ethernet            */
#define ETH_PROTO_TIPC      0x88CA          /* TIPC                         */



/* ethernet header
 */

typedef struct
{
  uchar_t dest[ETH_ADDR_LEN];
  uchar_t src[ETH_ADDR_LEN];
  ushort_t type;
} __attribute__((packed)) eth_hdr_t;



error_t eth_initialize(void);
void eth_write_header(uchar_t*, size_t*);
error_t eth_recv(struct net_if*, struct net_buf*);
error_t eth_send(struct net_if*, struct net_buf*, const uint8_t*, uint16_t);



#endif /* ! ETH_H_INCLUDED */
