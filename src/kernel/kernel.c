/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sat Oct 13 03:02:17 2007 texane
** Last update Wed Dec 12 02:28:12 2007 texane
*/



#include "kernel.h"
#include "boot/multiboot.h"
#include "debug/debug.h"
#include "arch/arch.h"
#include "sched/sched.h"
#include "event/event.h"
#include "task/task.h"
#include "idle/idle.h"
#include "muksh/muksh.h"
#include "mm/mm.h"
#include "mm/vm.h"
#include "net/net.h"
#include "bus/bus.h"
#include "libc/libc.h"
#include "../unit/unit.h"



/* kernel info
 */

uint8_t g_kernel_start[0];
uint8_t g_kernel_end[0];

kernel_info_t* g_kernel_info = NULL;

extern uint8_t multiboot_header[0];

static void kernel_dump(const kernel_info_t* info)
{
  serial_printl("kernel_info(0x%x)\n", (uint32_t)g_kernel_info);
  serial_printl("{\n");
  serial_printl(" .kernel_addr = 0x%x\n", (uint32_t)info->kernel_addr);
  serial_printl(" .kernel_size = 0x%x\n", (uint32_t)info->kernel_size);
  serial_printl("}\n");
}


static error_t kernel_init(const multiboot_info_t* mbi)
{
  multiboot_header_t* header;
  memory_map_t* mmap;
  paddr_t paddr;
  uint32_t size;
  uint32_t diff;

  TRACE_ENTRY();

  /* check header
   */
  header = (multiboot_header_t*)multiboot_header;
  if (header->magic != MULTIBOOT_HEADER_MAGIC)
    return ERROR_FAILURE;

  /* find space for the kernel
   */
  g_kernel_info = NULL;
  for (mmap = (memory_map_t*)mbi->mmap_addr;
       (mmap < (memory_map_t*)mbi->mmap_addr + mbi->mmap_length) &&
	 (g_kernel_info == NULL);
       mmap = (memory_map_t*)((paddr_t)mmap + mmap->size + sizeof(mmap->size)))
    if (mmap->type == 1)
      {
	paddr = (paddr_t)mmap->base_addr_low;
	size = mmap->length_low;

	/* special case for NULL
	 */
	if ((paddr == NULL) && (size > PHYS_FRAME_SIZE))
	  {
	    paddr += PHYS_FRAME_SIZE;
	    size -= PHYS_FRAME_SIZE;
	  }

	/* align on page boundary
	 */
	diff = (uint32_t)paddr & (PHYS_FRAME_SIZE - 1);
	if (diff)
	  {
	    if (diff < size)
	      {
		paddr += diff;
		size -= diff;
	      }
	    else
	      {
		paddr = NULL;
		size = 0;
	      }
	  }

	if ((paddr != NULL) && (size >= PHYS_FRAME_SIZE))
	  g_kernel_info = (kernel_info_t*)paddr;
      }

  if (g_kernel_info == NULL)
    return ERROR_NO_MEMORY;

  /* reset
   */
  g_kernel_info->kernel_addr = NULL;
  g_kernel_info->kernel_size = 0;

  /* use multiboot infos
   */
  if (header->flags & (1 << 15))
    {
      g_kernel_info->kernel_addr = (paddr_t)header->load_addr;
      g_kernel_info->kernel_size = header->load_end_addr - header->load_addr;
    }
  else
    {
      g_kernel_info->kernel_addr = g_kernel_start;
      g_kernel_info->kernel_size = g_kernel_end - g_kernel_start;
    }

  kernel_dump(g_kernel_info);

  TRACE_EXIT();

  return ERROR_SUCCESS;
}



void kernel_main(unsigned long magic,
		 unsigned long addr)
{
  multiboot_info_t *mbi;

  if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
    return;

  mbi = (multiboot_info_t*)addr;

  /* kernel init
   */
  serial_init(DEBUG_SERIAL_PORT,
	      DEBUG_SERIAL_SPEED,
	      UART_8BITS_WORD,
	      UART_NO_PARITY,
	      UART_1_STOP_BIT);
  cls();

  cpu_cli();
  printf("[x] interrupts disabled\n");

  gdt_initialize();
  printf("[x] gdt initialized\n");

  idt_initialize();
  printf("[x] idt initialized\n");

  breakpoint_initialize();

#if defined(USE_APIC)
  apic_initialize();
  serial_printl("[x] apic initialized\n");
#else
  pic_initialize();
  serial_printl("[x] pic initialized\n");
#endif /* USE_APIC */

  /* initialize the kernel
   */
  {
    kernel_init(mbi);
  }

  /* memory initialization
   */
  {
    phys_init(mbi);
    phys_debug();
/*     vm_init(); */
/*     unit_test_vm(); */
/*     cpu_hlt(); */
  }

#if defined(USE_PCI)
  pci_initialize();
  pci_list();
#endif

  cpu_sti();

#if defined(USE_TASK)
 {
   /* subsystems
    */
   event_initialize();
   sched_initialize();
   task_initialize();

   /* tasks
    */
   idle_initialize();
   muksh_initialize();
   net_initialize();

/*    task_test(); */

   /* start scheduling
    */
   sched_start();
 }
#endif

 /* endless loop
  */
 serial_printl("[?] kernel loop\n");
 while (1)
   {
     serial_printl("k");
     cpu_hlt();
   }
}
