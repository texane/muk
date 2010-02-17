/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sat Aug 25 20:36:53 2007 texane
** Last update Tue Nov 20 00:01:06 2007 texane
*/



/* notes
   -> ring buffer is really a ring: must write ring[0,1,2,3]
   -> implement stats inside the device
   -> refer to openbsd driver for a good documentation
 */



#include "../arch/idt.h"
#include "../arch/pic.h"
#include "../arch/cpu.h"
#include "../sys/types.h"
#include "../debug/debug.h"
#include "../libc/libc.h"
#include "../mm/mm.h"
#include "../bus/pci.h"
#include "../net/net.h"



/* registers
 */

#define RTL8139_REG_IDR 0x00
#define RTL8139_REG_MAR 0x08
#define RTL8139_REG_TSD 0x10
#define RTL8139_REG_TSAD 0x20
#define RTL8139_REG_RBSTART 0x30
#define RTL8139_REG_ERBCR 0x34
#define RTL8139_REG_ERSR 0x36
#define RTL8139_REG_CR 0x37
#define RTL8139_REG_CAPR 0x38
#define RTL8139_REG_CBR 0x3a
#define RTL8139_REG_IMR 0x3c
#define RTL8139_REG_ISR 0x3e
#define RTL8139_REG_TCR 0x40
#define RTL8139_REG_RCR 0x44
#define RTL8139_REG_TCTR 0x48
#define RTL8139_REG_MPC 0x4c
#define RTL8139_REG_CONFIG0 0x51
#define RTL8139_REG_CONFIG1 0x52
#define RTL8139_REG_TIMERINT 0x54
#define RTL8139_REG_CONFIG3 0x59
#define RTL8139_REG_CONFIG4 0x5a
#define RTL8139_REG_MISR 0x5c
#define RTL8139_REG_CONFIG5 0xd8


/* device
 */

typedef struct
{
  pci_dev_t* pci;
  addr_t regs;
  uchar_t tx_ring;
  uchar_t* tx_buf;
  uchar_t* rx_buf;
  net_buf_t* nb_head;
} rtl8139_dev_t;

static net_dev_t* g_device = NULL;


/* macros
 */

#define RTL8139_RXBUF_LEN (8 * 1024)
#define RTL8139_CAPR_OFF (16)
#define RTL8139_HEADER_LEN (4)

/* allocate a new device
 */
static rtl8139_dev_t* rtl8139_dev_new(pci_dev_t* pci)
{
  rtl8139_dev_t* dev;

  dev = mm_alloc(sizeof(rtl8139_dev_t));
  if (dev == NULL)
    return NULL;

  dev->pci = pci;
  dev->regs = (addr_t)pci->res_base;
  dev->tx_ring = 0;
  dev->tx_buf = mm_alloc(256);
  dev->rx_buf = mm_alloc(/* RTL8139_RXBUF_LEN + 16 */0x1000);
  dev->nb_head = NULL;

  return dev;
}


/* device initialization
 */

static void rtl8139_reset(addr_t regs)
{
  volatile uchar_t* cr;

  cr = (uchar_t*)(regs + RTL8139_REG_CR);
  *cr |= 1 << 4;
  while (*cr & (1 << 4))
    ;
}


static void rtl8139_enable_txrx(addr_t regs)
{
  uchar_t* cr;

  cr = (uchar_t*)(regs + RTL8139_REG_CR);

  /* enable tx and rx */
  *cr = (1 << 3) | (1 << 2);
}


static void rtl8139_setup_rx(addr_t regs, uchar_t* buf)
{
  uint_t* rcr;
  uint_t* mpc;
  uint_t* rbstart;
  ushort_t* misr;

  /* receive count register */
  rcr = (uint_t*)(regs + RTL8139_REG_RCR);
  *rcr = 
    (0x07 << 13) | /* no rx fifo thresold */
    (0x00 << 11) | /* rx buffer len is 8k + 16 */
    (0x00 <<  8) | /* dma burst size 16 */
    (0x3f <<  0);  /* accept all packets */

  /* configure rx buffer */
  rbstart = (uint_t*)(regs + RTL8139_REG_RBSTART);
  *rbstart = (uint_t)buf;

  /* rx missed to 0 */
  mpc = (uint_t*)(regs + RTL8139_REG_MPC);
  *mpc = 0;

  /* no rx early interrupts */
  misr = (ushort_t*)(regs + RTL8139_REG_MISR);
  *misr = 0;
}


static void rtl8139_setup_tx(addr_t regs, uchar_t* buf)
{
  uint_t* tcr;
  uint_t* tsad;
  uint_t i;

  /* transmit conf register */
  tcr = (uint_t*)(regs + RTL8139_REG_TCR);
  *tcr = 0 | (1 << 16);
  
  /* set buffer descriptors */
  tsad = (uint_t*)(regs + RTL8139_REG_TSAD);
  for (i = 0; i < 4; ++i)
    {
      /* should be of the size of our tx buffer */
      FIXME();
      tsad[i] = (uint_t)(buf + i * 64);
    }
}


static void rtl8139_enable_ints(addr_t regs)
{
  ushort_t* imr;

  /* unmask tx, ter, rx, rer, rxovw ints */
  imr = (ushort_t*)(regs + RTL8139_REG_IMR);
  *imr = 0x1f;
}


static void rtl8139_disable_ints(addr_t regs)
{
  ushort_t* imr;

  imr = (ushort_t*)(regs + RTL8139_REG_IMR);
  *imr = 0x0;
}


static void rtl8139_wait_txrx(addr_t regs)
{
  volatile uchar_t* cr;

  /* are rx and tx enabled */
  cr = (uchar_t*)(regs + RTL8139_REG_CR);
  while (((*cr) & (0xc)) != (0xc))
    ;
}


/* initialize a device
 */
static int rtl8139_dev_initialize(rtl8139_dev_t* dev)
{
  rtl8139_reset(dev->regs);
  rtl8139_enable_txrx(dev->regs);
  rtl8139_setup_rx(dev->regs, dev->rx_buf);
  rtl8139_setup_tx(dev->regs, dev->tx_buf);
  rtl8139_wait_txrx(dev->regs);
  rtl8139_enable_ints(dev->regs);

  return 0;
}


/* send a buffer
 */
static error_t rtl8139_dev_send(rtl8139_dev_t* dev, uchar_t* buf, size_t size)
{
  volatile uint_t* tsd;

  tsd = (uint_t*)(dev->regs + RTL8139_REG_TSD);
  memcpy(dev->tx_buf + dev->tx_ring * 64, buf, size);
  tsd[dev->tx_ring] = size;
  
  return ERROR_SUCCESS;
}


/* set device info
 */
static int __attribute__((unused)) rtl8139_dev_ioctl(rtl8139_dev_t* dev)
{
  NOT_IMPLEMENTED();

  return -1;
}


/* print device info
 */
static void __attribute__((unused)) rtl8139_dev_print(rtl8139_dev_t* dev)
{
  uchar_t* hwaddr;

  hwaddr = (uchar_t*)dev->regs;
  serial_printl("[?] %s: %x:%x:%x:%x:%x:%x\n",
		__FUNCTION__,
		hwaddr[0], hwaddr[1], hwaddr[2],
		hwaddr[3], hwaddr[4], hwaddr[5]);
}



#include "../arch/cpu.h"
static void __attribute__((unused)) rtl8139_timeout(addr_t regs)
{
  volatile uint_t* timerint;
  volatile ushort_t* isr;

  timerint = (uint_t*)(regs + RTL8139_REG_TIMERINT);
  *timerint = 1000;

  isr = (ushort_t*)(regs + RTL8139_REG_ISR);
}


/* interrupt handler
 */

inline static void handle_tx_interrupt(addr_t regs)
{
  ushort_t* isr;
  uint_t* tsd;
  uint_t status;
  rtl8139_dev_t* dev;

  dev = (rtl8139_dev_t*)g_device->priv;

  /* txed or error */
  tsd = (uint_t*)(regs + RTL8139_REG_TSD);
  status = tsd[dev->tx_ring];
  if (status & ((1 << 29) | (1 << 15) | (1 << 14)))
    {
    }

  /* next ring entry */
  if (++dev->tx_ring > 3)
    dev->tx_ring = 0;

  /* ack the int */
  isr = (ushort_t*)(regs + RTL8139_REG_ISR);
  *isr |= 1 << 2;
}

inline static void handle_rx_interrupt(addr_t regs, ushort_t status)
{
  /* todos
     . disable rx interrupts while processing the packet
   */

  uint_t header;
  size_t rx_size;
  off_t rx_off;
  size_t pkt_size;
  ushort_t* capr;
  ushort_t* cbr;
  volatile uchar_t* cr;
  net_buf_t* nb;
  net_buf_t* pos;
  net_dev_t* net_dev;
  rtl8139_dev_t* rtk_dev;

  net_dev = g_device;
  rtk_dev = (rtl8139_dev_t*)net_dev->priv;

  /* get registers */
  capr = (ushort_t*)(regs + RTL8139_REG_CAPR);
  cbr = (ushort_t*)(regs + RTL8139_REG_CBR);
  cr = (uchar_t*)(regs + RTL8139_REG_CR);

  /* not empty */
  while (!(*cr & 1))
    {
      /* current packet in ring */
      rx_off = (*capr + RTL8139_CAPR_OFF) % RTL8139_RXBUF_LEN;
      header = *(uint_t*)(rtk_dev->rx_buf + rx_off);

      /* size include the status ushort_t, little endian */
      rx_size = header >> 16;
      if (rx_size == 0xfff0)
	BUG();

      /* current paket size */
      pkt_size = rx_size - ETH_CRC_LEN;

      /* skip header */
      rx_off += RTL8139_HEADER_LEN;

      /* create netbuf */
      if (error_is_success(net_buf_alloc(&nb)))
	{
	  net_buf_append(nb, rtk_dev->rx_buf + rx_off, pkt_size);
	  /* link the netbuf */
	  nb->next = rtk_dev->nb_head;
	  rtk_dev->nb_head = nb;
	}

      /* update capr */
      rx_off = (rx_off + pkt_size + RTL8139_HEADER_LEN + 3) & ~3;
      if (rx_off >= RTL8139_RXBUF_LEN)
	rx_off = rx_off % RTL8139_RXBUF_LEN;
      *capr = rx_off - RTL8139_CAPR_OFF;
    }

  /* tell network interface something is available
   */
  nb = rtk_dev->nb_head;
  while (nb != NULL)
    {
      pos = nb;
      nb = nb->next;
      if (net_dev->net_if != NULL)
	net_if_rx(net_dev->net_if, pos);
      else
	net_buf_free(pos);
    }
  rtk_dev->nb_head = NULL;
}


static void rtl8139_handle_interrupt(void)
{
  ushort_t status;
  volatile ushort_t* isr;
  rtl8139_dev_t* dev;

  dev = (rtl8139_dev_t*)g_device->priv;

  /* interrupt status register */
  isr = (ushort_t*)(dev->regs + RTL8139_REG_ISR);
  status = *isr;
  if (status & ((1 << 2) | (1 << 3)))
    {
      *isr = status;
      rtl8139_disable_ints(dev->regs);
      handle_tx_interrupt(dev->regs);
      rtl8139_enable_ints(dev->regs);
    }
  else if (status & ((1 << 0) | (1 << 1) | (1 << 4)))
    {
      *isr = status;
      rtl8139_disable_ints(dev->regs);
      handle_rx_interrupt(dev->regs, status);
      rtl8139_enable_ints(dev->regs);
    }
  else
    NOT_IMPLEMENTED();
}



/* implement net_dev_t
 */

error_t rtl8139_send(net_dev_t* dev, struct net_buf* nb)
{
  rtl8139_dev_t* priv;
  error_t error;
  uint8_t* buf;
  size_t size;

  priv = (rtl8139_dev_t*)dev->priv;

  error = net_buf_read(nb, &buf, &size);
  if (error_is_failure(error))
    return error;

  error = rtl8139_dev_send(priv, buf, size);

  return error;
}


error_t rtl8139_get_addr(net_dev_t* dev, uchar_t* addr)
{
  rtl8139_dev_t* priv;
  uchar_t* idr;

  priv = (rtl8139_dev_t*)dev->priv;
  idr = (uchar_t*)(priv->regs + RTL8139_REG_IDR);
  memcpy(addr, idr, 6);

  return ERROR_SUCCESS;
}


error_t rtl8139_set_addr(net_dev_t* dev, uchar_t* addr)
{
  rtl8139_dev_t* priv;
  uchar_t* idr;

  priv = (rtl8139_dev_t*)dev->priv;
  idr = (uchar_t*)(priv->regs + RTL8139_REG_IDR);
  memcpy(idr, addr, 6);

  return ERROR_SUCCESS;
}



/* initialize
 */
error_t rtl8139_initialize(net_dev_t** res)
{
  rtl8139_dev_t* dev;
  pci_dev_t* pci;
  error_t error;

  *res = NULL;

  /* find the pci device
   */
  pci = pci_dev_find_rtl8139();
  if (is_null(pci))
    return ERROR_NOT_FOUND;

#if !defined(USE_APIC)
  pic_enable_irq(0xb, rtl8139_handle_interrupt);
#endif

  /* allocate private data
   */
  dev = rtl8139_dev_new(pci);
  if (dev == NULL)
    return ERROR_NO_MEMORY;

  /* initialize the device
   */
  if (rtl8139_dev_initialize(dev))
    return ERROR_FAILURE;

  /* allocate new device
   */
  error = net_dev_create(&g_device, dev);
  if (error_is_failure(error))
    return error;

  g_device->send = rtl8139_send;
  g_device->get_addr = rtl8139_get_addr;
  g_device->set_addr = rtl8139_set_addr;

  *res = g_device;

  return ERROR_SUCCESS;
}



/* cleanup
 */
error_t rtl8139_cleanup(void)
{
#if !defined(USE_APIC)
  pic_disable_irq(0xb);
#endif

  if (!is_null(g_device))
    {
      mm_free(g_device->priv);
      mm_free(g_device);
      g_device = null;
    }

  return ERROR_SUCCESS;
}



/* testing
 */
void rtl8139_test(void)
{
  net_buf_t* nb_head;
  net_buf_t* nb_tail;
  net_buf_t* nb_pos;
  rtl8139_dev_t* dev;

  TRACE_ENTRY();

  dev = (rtl8139_dev_t*)g_device->priv;

  while (1)
    {
/*       icmp_ping(g_device, NET_REMOTE_ADDR); */
      cpu_hlt();
#if 0
      cpu_cli();
      arp_print_table();
      cpu_sti();
#endif
    }

  while (1)
    {
      nb_head = null;
      nb_tail = null;

      cpu_hlt();

      /* safely get the net buffers */
      cpu_cli();
      if (!is_null(dev->nb_head))
	{
	  for (nb_pos = dev->nb_head; nb_pos->next; nb_pos = nb_pos->next)
	    ;

	  if (!is_null(nb_tail))
	    nb_tail->next = dev->nb_head;
	  else
	    nb_head = dev->nb_head;
	  nb_tail = nb_pos;
	}
      dev->nb_head = null;
      cpu_sti();

      /* print and release net buffers */
      while (nb_head)
	{
	  nb_pos = nb_head;
	  nb_head = nb_head->next;
	  /* hexdump(nb_pos->buf, nb_pos->size); */
	  mm_free(nb_pos->buf);
	  mm_free(nb_pos);
	}

      arp_print_table();
    }
}
