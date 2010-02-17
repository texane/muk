/*
** Made by texane <texane@gmail.com>
** 
** Started on  Wed Aug 22 23:28:57 2007 texane
** Last update Sat Nov 17 03:22:05 2007 texane
*/


/* links
   -> www.tldp.org/LDP/tlk/dd/pci.html
   -> 
 */


/* todos
   -> only scan the main bus, using type 0 addressing; if the address is
   to another bus, then make addr type1
   -> should be able to pci_dev_write_config_dword() once initialized
   -> configure the main pci-to-pci bridge
   -> int line of the device
   -> base address of the device
   -> name of the device
   -> should check the bios for precise information about config access
   -> enable the device, check the status and see?
   -> is the structure of pci_header rightly layouted?
 */


/* notes:
   -> pci io space base addressing is bound to the motherboard device
   -> on qemu, i440fx is the pci bridge (@ref to the bellard's doc)
   -> see the motherboard doc for the iomap and access to pci registers
   -> from the doc,  00:11:00, 00:13:00, 00:1b:00 available for pci expansion slot
   -> from the pci spec, what is interessting: ch{1,2,3,6}
   -> pci bus is multiplexed so that it uses same pins for both data and
   addresses, with a bus transaction consisting of one address phase followed
   by one or more data phase (for 64 bits)
   -> pci contains 3 address spaces: memory, io, config
   -> device relocation, interrupt binding
   -> every device must implement a configuration space
   -> 256 bytes space, std header and device dependent region
 */



#include "../debug/debug.h"
#include "../libc/libc.h"
#include "../arch/cpu.h"
#include "../bus/pci.h"
#include "../bus/pci_ids.h"
#include "../mm/mm.h"


/* remove */ #include "../drivers/rtl8139.h"


/* pci type 1 address
 */
typedef struct
{
  union
  {
    struct
    {
      uchar_t type : 2;
      uchar_t reg : 6;
      uchar_t func : 3;
      uchar_t dev : 5;
      uchar_t bus : 8;
      uchar_t reserved : 7;
      uchar_t enable : 1;
    } bits;
    uint_t addr;
  } u;
} __attribute__((packed)) pci_addr_t;


typedef struct
{
  ushort_t vendor_id : 16;
  ushort_t device_id : 16;
  ushort_t command : 16;
  ushort_t status : 16;
  uchar_t rev_id : 8;
  uint_t class_code : 24;
  uchar_t cacheln_size : 8;
  uchar_t latency_timer : 8;
  uchar_t header_type : 8;
  uchar_t bist : 8;
  uint_t base_addr_regs[6];
  uint_t cb_cis_pointer : 32;
  ushort_t subsystem_vendor_id : 16;
  ushort_t subsystem_id : 16;
  uint_t exrom_base_addr : 32;
  uint_t reserved_0 : 32;
  uint_t reserved_1 : 32;
  uchar_t int_ln: 8;
  uchar_t int_pin : 8;
  uchar_t min_gnt : 8;
  uchar_t max_lat : 8;
} __attribute__((packed)) pci_header_t;


/* pci bus global
 */
typedef struct pci_bus
{
  struct pci_bus* next;
  pci_dev_t* devs;
  uchar_t ibus;
} pci_bus_t;

static pci_bus_t* g_pci_bus = NULL;


/* make a type n address */

inline static void pci_addr_make_type0(pci_addr_t* addr, uint_t dev, uchar_t func, uchar_t reg)
{
  memset(addr, 0, sizeof(pci_addr_t));
  addr->u.bits.reg = reg;
  addr->u.bits.func = func;
  addr->u.addr |= dev << 11;
  addr->u.bits.enable = 1;
}

inline static void pci_addr_make_type1(pci_addr_t* addr, uchar_t bus, uchar_t dev, uchar_t func, uchar_t reg)
{
  memset(addr, 0, sizeof(pci_addr_t));
  addr->u.bits.type = 1;
  addr->u.bits.reg = reg;
  addr->u.bits.func = func;
  addr->u.bits.dev = dev;
  addr->u.bits.bus = bus;
  addr->u.bits.enable = 1;
}

/* print a type 1 address */
inline static void pci_addr_print(const pci_addr_t* addr)
{
  printf("{%x:%x:%x}\n", addr->u.bits.bus, addr->u.bits.dev, addr->u.bits.func);
}


/* pci config ios
 */

static inline uint_t pci_read_config_addr(void)
{
#define I440FX_PCI_CONFIG_ADDR 0x0cf8
  return cpu_inl(I440FX_PCI_CONFIG_ADDR);
}

static inline void pci_write_config_addr(uint_t val)
{
  cpu_outl(I440FX_PCI_CONFIG_ADDR, val);
}

static inline uint_t pci_read_config_data(void)
{
#define I440FX_PCI_CONFIG_DATA 0x0cfc
  return cpu_inl(I440FX_PCI_CONFIG_DATA);
}

static inline void pci_write_config_data(uint_t val)
{
  cpu_outl(I440FX_PCI_CONFIG_DATA, val);
}

static void __attribute__((unused)) pci_write_config(uchar_t bus, uchar_t dev, uchar_t func, uchar_t reg, uint_t val)
{
  pci_addr_t addr;

  pci_addr_make_type0(&addr, dev, func, reg >> 2);
  pci_write_config_addr(addr.u.addr);
  pci_write_config_data(val);
}

static uint_t pci_read_config(uchar_t bus, uchar_t dev, uchar_t func, uchar_t reg)
{
  pci_addr_t addr;
  uint_t val;

  pci_addr_make_type0(&addr, dev, func, reg >> 2);
  pci_write_config_addr(addr.u.addr);
  val = pci_read_config_data();

  return val;
}

static uchar_t __attribute__((unused)) pci_read_config_byte(uchar_t bus, uchar_t dev, uchar_t func, uchar_t reg)
{
  uint_t val;

  val = pci_read_config(bus, dev, func, reg & ~3);
  val >>= (reg & 3) * 8;
  return val;
}

static ushort_t __attribute__((unused)) pci_read_config_word(uchar_t bus, uchar_t dev, uchar_t func, uchar_t reg)
{
  uint_t val;

  val = pci_read_config(bus, dev, func, reg & ~3);
  val >>= (reg & 2) * 8;
  return val;
}

static uint_t __attribute__((unused)) pci_read_config_dword(uchar_t bus, uchar_t dev, uchar_t func, uchar_t reg)
{
  return pci_read_config(bus, dev, func, reg);
}

static void pci_read_config_header(uchar_t bus, uchar_t dev, uchar_t func, pci_header_t* header)
{
  uchar_t ireg;
  uint_t* regs;

  for (ireg = 0, regs = (uint_t*)header; ireg < sizeof(pci_header_t); ireg += sizeof(uint_t), ++regs)
    *regs = pci_read_config_dword(bus, dev, func, ireg);
}



static pci_dev_t* pci_dev_new(pci_bus_t* bus, int idev, const pci_header_t* header)
{
  pci_dev_t* dev;

  dev = mm_alloc(sizeof(pci_dev_t));
  if (is_null(dev))
    return null;

  dev->next = null;
  dev->prev = null;
  dev->bus = bus;
  dev->idev = idev;

  dev->res_ismem = false;
  dev->res_base = 0;
  dev->res_size = 0;

  if (!is_null(header))
    {
      dev->vendor_id = header->vendor_id;
      dev->device_id = header->device_id;
      dev->class_code = header->class_code;
      dev->int_ln = header->int_ln;
    }

  return dev;
}


static int pci_dev_setup(pci_dev_t* dev)
{
  uchar_t iaddr;
  uint_t val;
  uint_t addr;
  uint_t size;
  bool_t is_mapped;

  /* network device only */
  if (((dev->class_code & 0xff0000) >> 16) != PCI_BASE_CLASS_NETWORK)
    return -1;

  /* device addresses */
  for (iaddr = 0; iaddr < 24; iaddr += sizeof(uint_t))
    {
      is_mapped = false;

      /* save, get the size and restore */
      addr = pci_read_config_dword(dev->bus->ibus, dev->idev, 0, 0x10 + iaddr);
      pci_write_config(dev->bus->ibus, dev->idev, 0, 0x10 + iaddr, ~0);
      size = pci_read_config(dev->bus->ibus, dev->idev, 0, 0x10 + iaddr);
      pci_write_config(dev->bus->ibus, dev->idev, 0, 0x10 + iaddr, addr);

      /* allocate resources */
      if ((size) && (size != 0xffffffff))
	{
	  /* register is memory mapped */
	  if ((size & 0x1) == 0)
	    {
	      dev->res_ismem = true;

	      /* map size */
	      size = ~(size & ~0x1) + 1;
	      if (size)
		{
		  dev->res_base = addr & ~0xf;
		  dev->res_size = size;
		  is_mapped = true;
		}
	    }

	  /* register is io mapped */
	  else
	    {
	      NOT_IMPLEMENTED();
	    }
	}
	  
      /* enable io or mem access */
      if (is_mapped == true)
	{
#define PCI_CONFIG_HEADER_COMMAND 0x4
	  val = pci_read_config_dword(dev->bus->ibus, dev->idev, 0, PCI_CONFIG_HEADER_COMMAND);
	  if (dev->res_ismem == true)
	    {
	      /* enable the memory space */
	      val = (val & ~0x3) | (1 << 1);
	      pci_write_config(dev->bus->ibus, dev->idev, 0, PCI_CONFIG_HEADER_COMMAND, val);

	      /* configure irq line */
/* #define PCI_CONFIG_HEADER_IRQLINE 0x3c */
/* 	      val = pci_read_config_dword(dev->bus->ibus, dev->idev, 0, PCI_CONFIG_HEADER_IRQLINE); */
/* 	      val = (val & ~0xff) | 0x1; */
/* 	      pci_write_config(dev->bus->ibus, dev->idev, 0, PCI_CONFIG_HEADER_IRQLINE, val); */
	    }
	  else
	    {
	      NOT_IMPLEMENTED();
	    }
	}

    }

  return 0;
}


static pci_bus_t* pci_bus_new(uchar_t ibus)
{
  pci_bus_t* bus;

  bus = mm_alloc(sizeof(pci_bus_t));
  if (is_null(bus))
    return null;

  bus->next = null;
  bus->devs = null;
  bus->ibus = ibus;

  return bus;
}


static void pci_dev_print(const pci_dev_t* dev)
{
  serial_printl("dev\n");
  serial_printl("{\n");
  serial_printl(" .vendor id == 0x%x\n", dev->vendor_id);
  serial_printl(" .device id == 0x%x\n", dev->device_id);
  serial_printl(" .class_code == 0x%x\n", dev->class_code);
  serial_printl(" .int_ln == 0x%x\n", dev->int_ln);
  serial_printl(" .res_base == 0x%x\n", dev->res_base);
  serial_printl(" .res_size == 0x%x\n", dev->res_size);
  serial_printl(" .res_ismem == %d\n", dev->res_ismem);
  serial_printl("}\n");
}


static void pci_bus_scan(pci_bus_t* bus)
{
  /* according to specs
     walk every possible pci slot and read the vendor
     id. Since 0xffff is not a valid vendor id, the
     device is considered as not present.
   */

  uchar_t idev;
  ushort_t vendor_id;
  pci_header_t header;
  pci_dev_t* dev;

  for (idev = 0; idev <= 0x1f; ++idev)
    {
      vendor_id = pci_read_config_word(bus->ibus, idev, 0, 0);
      if (vendor_id != 0xffff)
	{
	  pci_read_config_header(bus->ibus, idev, 0, &header);

	  /* add a new pci device */
	  dev = pci_dev_new(bus, idev, &header);
	  if (!is_null(dev))
	    {
	      /* link the device */
	      if (!is_null(bus->devs))
		{
		  bus->devs->prev = dev;
		  dev->next = bus->devs;
		}
	      bus->devs = dev;

	      /* setup the device */
	      pci_dev_setup(dev);
	    }
	}
    }
}


int pci_initialize(void)
{
  if (!is_null(g_pci_bus))
    return -1;

  /* todos: check if there are pci bus */

  /* create a new bus */
  g_pci_bus = pci_bus_new(0);
  if (is_null(g_pci_bus))
    return -1;
  pci_bus_scan(g_pci_bus);

  return 0;
}


int pci_list(void)
{
  pci_bus_t* bus;
  pci_dev_t* dev;

  cls();

  for (bus = g_pci_bus; !is_null(bus); bus = bus->next)
    for (dev = bus->devs; !is_null(dev); dev = dev->next)
      {
	if (((dev->class_code & 0xff0000) >> 16) == PCI_BASE_CLASS_NETWORK)
	  pci_dev_print(dev);
      }

  return 0;
}


pci_dev_t* pci_dev_find_rtl8139(void)
{
  pci_bus_t* bus;
  pci_dev_t* dev;

  for (bus = g_pci_bus; !is_null(bus); bus = bus->next)
    for (dev = bus->devs; !is_null(dev); dev = dev->next)
      if (((dev->class_code & 0xff0000) >> 16) == PCI_BASE_CLASS_NETWORK)
	return dev;
  
  return NULL;
}
