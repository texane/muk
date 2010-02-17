/*
** Made by texane <texane@gmail.com>
** 
** Started on  Mon Sep 10 23:16:17 2007 texane
** Last update Mon Oct 15 15:08:56 2007 texane
*/



#include "mm.h"
#include "vm.h"
#include "phys.h"
#include "../arch/arch.h"
#include "../sys/types.h"
#include "../libc/libc.h"
#include "../debug/debug.h"



/* cpu specific
 */

typedef struct pde
{
  uint8_t present : 1;
  uint8_t writable : 1;
  uint8_t user : 1;
  uint8_t pwt : 1;
  uint8_t cachable : 1;
  uint8_t accessed : 1;
  uint8_t zero : 1;
  uint8_t size : 1;
  uint8_t global : 1;
  uint8_t avail : 3;
  uint32_t address : 20;
} __attribute__((packed)) pde_t;


typedef struct pde4m
{
  uint8_t present : 1;
  uint8_t writable : 1;
  uint8_t user : 1;
  uint8_t pwt : 1;
  uint8_t cachable : 1;
  uint8_t accessed : 1;
  uint8_t dirty : 1;
  uint8_t size : 1;
  uint8_t global : 1;
  uint8_t avail : 3;
  uint8_t pat : 1;
  uint32_t reserved : 9;
  uint32_t address : 10;
} __attribute__((packed)) pde4m_t;


typedef struct pte
{
  uint8_t present : 1;
  uint8_t writable : 1;
  uint8_t user : 1;
  uint8_t pwt : 1;
  uint8_t cachable : 1;
  uint8_t accessed : 1;
  uint8_t dirty : 1;
  uint8_t pat : 1;
  uint8_t global : 1;
  uint8_t avail : 3;
  uint32_t address : 20;
} __attribute__((packed)) pte_t;


inline static void cpu_enable_vm(void)
{
  /* cr0 is used to control the cpu
     cr4 is used to control extension
  */

  uint32_t tmp;

  asm volatile (
		"movl %%cr4, %0 \n\t"
		
		/* set pse and pge */
		"orl $0x00000090, %0 \n\t"
		
		/* clear pae */
		"andl $0xffffffdf, %0 \n\t"
		
		"movl %0, %%cr4 \n\t"
		
		/* enable paging */
		"movl %%cr0, %0 \n\t"
		"orl $0x80000000, %0 \n\t"
		"movl %0, %%cr0 \n\t"

		: "=r"(tmp)
		);
}


inline static void cpu_disable_vm(void)
{
  asm volatile (
		"movl %cr0, %eax \n\t"
		"andl $0x7fffffff, %eax \n\t"
		"movl %eax, %cr0 \n\t"
		:
		:
		: "eax"
		);
}


inline static void cpu_switch_as(vm_as_t* as)
{
  /* cr3 contains the pagedir physical address
     must be aligned on a 4K boundary
  */

  uintptr_t pagedir;
  uint32_t tmp;

  pagedir = (uintptr_t)as->pagedir;

  asm volatile (
		"orl $0x08, %0 \n\t"
		"movl %%cr3, %1 \n\t"
		"cmpl %0, %1 \n\t"
		"je 1f \n\t"
		"movl %0, %%cr3 \n\t"
		"1: \n\t"
		: "=r"(pagedir), "=r"(tmp)
		: "0"(pagedir)
		);
}


inline static void cpu_invalidate_page(uintptr_t vaddr)
{
  asm volatile (
		"invlpg %0 \n\t"
		:
		: "m"(vaddr)
		);
}


inline static vaddr_t cpu_get_faulting_addr(void)
{
  /* cr2 contains the linear faulting address
   */

  paddr_t addr;

  asm volatile (
		"movl %%cr2, %%eax\n\t"
		"movl %%eax, %0\n\t"
		: "=m"(addr)
		);

  return addr;
}


inline static uint32_t vaddr_get_pd_index(vaddr_t vaddr)
{
  return (uint32_t)vaddr >> 22;
}


inline static uint32_t vaddr_get_pt_index(vaddr_t vaddr)
{
  return ((uint32_t)vaddr >> 12) & 0x3ff;
}


inline static uint32_t vaddr_get_pg_offset(vaddr_t vaddr)
{
  return (uint32_t)vaddr & 0xfff;
}



/* handle a page fault
 */

static void vm_handle_fault(void)
{
  vaddr_t vaddr;
  paddr_t paddr;

  TRACE_ENTRY();

  vaddr = cpu_get_faulting_addr();
  serial_printl("[?] faulting on 0x%x\n", (uint32_t)vaddr);

  /* map the address
   */
  paddr = phys_alloc();

  vm_map(vm_get_current_as(), vaddr, paddr);

  serial_printl("[?] fault handled\n");
}



/* switch vm to as
 */

static vm_as_t* g_current_as = NULL;

error_t vm_switch_as(vm_as_t* as)
{
  g_current_as = as;

  cpu_switch_as(as);

  return ERROR_SUCCESS;
}



/* create kenrel as
 */

#if 0

static error_t build_identity_4m(vm_as_t* as,
				 uintptr_t base,
				 uint32_t size)
{
  uint32_t i;
  pde4m_t* pde;

  /* not rounded to 4M boundary
   */
  if (base & (0x400000 - 1))
    return ERROR_INVALID_ADDR;

  /* round size
   */
  if (size & (0x400000 - 1))
    size = (size + 0x400000) & ~(0x400000 - 1);
  
  i = size / 0x400000;

  while (i)
    {
      /* index is i - 1
       */
      --i;

      /* current pde
       */
      pde = (pde4m_t*)(as->pagedir + i);

      /* 4M global pages
       */
      pde->present = 1;
      pde->writable = 1;
      pde->size = 1;
      pde->global = 1;
      pde->pwt = 1;
      pde->cachable = 1;
      pde->address = (base + i * 0x400000) >> 22;
    }

  return ERROR_SUCCESS;
}

#endif


static error_t build_identity(vm_as_t* as,
			      vaddr_t where,
			      paddr_t paddr,
			      uint32_t size)
{
  pde_t* pde;
  pte_t* pte;
  paddr_t limit;
  paddr_t pt;

  limit = paddr + size;
  while (paddr < limit)
    {
      /* page directory entry
       */
      pde = &as->pagedir[vaddr_get_pd_index(where)];
      if (pde->present == 0)
	{
	  pt = (paddr_t)phys_alloc();
	  memset((void*)pt, 0, PHYS_FRAME_SIZE);
	  pde->present = 1;
	  pde->writable = 1;
	  pde->pwt = 1;
	  pde->cachable = 1;
	  pde->address = (uint32_t)pt >> 12;
	}

      /* page table entry
       */
      pt = (paddr_t)(pde->address << 12);
      pte = &((pte_t*)pt)[vaddr_get_pt_index(where)];
      if (pte->present == 0)
	{
	  pte->present = 1;
	  pte->writable = 1;
	  pte->cachable = 1;
	  pte->pwt = 1;
	  pte->address = (uint32_t)paddr >> 12;
	}

      /* next page
       */
      where += PHYS_FRAME_SIZE;
      paddr += PHYS_FRAME_SIZE;
    }

  return ERROR_SUCCESS;
}


static vm_as_t* vm_create_kernel_as(void)
{
  vm_as_t* as;
  
  /* allocate as
   */
  as = (vm_as_t*)phys_alloc();
  if (as == NULL)
    return NULL;

  /* allocate the page directory
   */
  as->pagedir = (pde_t*)phys_alloc();
  if (as->pagedir == NULL)
    {
      phys_free((paddr_t)as);
      return NULL;
    }
  memset(as->pagedir, 0, PHYS_FRAME_SIZE);

  /* kernel identity mapping
   */
  build_identity(as, NULL, NULL, 4 * 1024 * 1024);

  /* physical mem identity zone
   */
  as->identity = (vaddr_t)0xc0000000;
  build_identity(as, as->identity, NULL, phys_get_mem_size());

  /* map the page directory
   */
  {
    pde_t* pde;
    pte_t* pte;
    paddr_t pt;

    pde = &as->pagedir[vaddr_get_pd_index((vaddr_t)as->pagedir)];
    if (pde->present == 0)
      {
	pt = phys_alloc();
	memset(pt, 0, PHYS_FRAME_SIZE);
	pde->present = 1;
	pde->writable = 1;
	pde->cachable = 1;
	pde->pwt = 1;
	pde->address = (uint32_t)pt >> 12;
      }

    pt = (paddr_t)(pde->address << 12);
    pte = &((pte_t*)pt)[vaddr_get_pt_index((vaddr_t)as->pagedir)];
    if (pte->present == 0)
      {
	pte->present = 1;
	pte->writable = 1;
	pte->cachable = 1;
	pte->pwt = 1;
	pte->address = (uint32_t)as->pagedir >> 12;
      }
  }

  /* map the as
   */
  {
    pde_t* pde;
    pte_t* pte;
    paddr_t pt;

    pde = &as->pagedir[vaddr_get_pd_index((vaddr_t)as)];
    if (pde->present == 0)
      {
	pt = phys_alloc();
	memset(pt, 0, PHYS_FRAME_SIZE);
	pde->present = 1;
	pde->writable = 1;
	pde->cachable = 1;
	pde->pwt = 1;
	pde->address = (uint32_t)pt >> 12;
      }

    pt = (paddr_t)(pde->address << 12);
    pte = &((pte_t*)pt)[vaddr_get_pt_index((vaddr_t)as)];
    if (pte->present == 0)
      {
	pte->present = 1;
	pte->writable = 1;
	pte->cachable = 1;
	pte->pwt = 1;
	pte->address = (uint32_t)as >> 12;
      }
  }

  return as;
}



/* init virtual memory
 */

static vm_as_t* g_kernel_as = NULL;

error_t vm_init(void)
{
  TRACE_ENTRY();

  /* allocate the kernel as
   */
  g_kernel_as = vm_create_kernel_as();
  if (g_kernel_as == NULL)
    return ERROR_NO_MEMORY;

  /* page fault handler
   */
  idt_set_handler(14, vm_handle_fault);

  /* switch to the kernel as
   */
  vm_switch_as(g_kernel_as);

  /* enable the virtual memory
   */
  cpu_enable_vm();

  TRACE_EXIT();

  return ERROR_SUCCESS;
}



/* set a virtual to physical mapping
 */

error_t vm_map(vm_as_t* as,
	       vaddr_t vaddr,
	       paddr_t paddr)
{
  /* assume pde contains a valid mapping
  */

  pde_t* pde;
  pte_t* pte;
  paddr_t pt;

  TRACE_ENTRY();

  /* page dir entry
   */
  pde = (pde_t*)&as->pagedir[vaddr_get_pd_index(vaddr)];
  if (pde->present == 0)
    {
      /* allocate page table
       */
      pt = phys_alloc();
      pde->present = 1;
      pde->writable = 1;
      pde->pwt = 1;
      pde->cachable = 1;
      pde->address = (uint32_t)pt >> 12;

      /* zero the table, use identity
       */
      pt = as->identity + (uint32_t)pt;
      memset(pt, 0, PHYS_FRAME_SIZE);
    }

  /* page table entry. Access its contents
     through the as identity mapping.
   */
  pt = as->identity + (pde->address << 12);
  pte = &((pte_t*)pt)[vaddr_get_pt_index(vaddr)];
  if (pte->present == 0)
    {
      pte->present = 1;
      pte->writable = 1;
      pte->pwt = 1;
      pte->cachable = 1;
      pte->address = (uint32_t)paddr >> 12;
    }

  /* invalidate the tlb
   */
  cpu_invalidate_page((uintptr_t)vaddr);

  return ERROR_SUCCESS;
}



/* unmap
 */

error_t vm_unmap(vm_as_t* as, vaddr_t vaddr)
{
  pde_t* pde;
  pte_t* pte;
  paddr_t pt;

  TRACE_ENTRY();
  
  pde = (pde_t*)&as->pagedir[vaddr_get_pd_index(vaddr)];
  if (pde->present == 0)
    return ERROR_INVALID_ADDR;

  pt = as->identity + (pde->address << 12);
  pte = &((pte_t*)pt)[vaddr_get_pt_index(vaddr)];
  if (pte->present == 0)
    return ERROR_INVALID_ADDR;

  pte->present = 0;

  return ERROR_SUCCESS;
}



/* map a contiguous range
 */

error_t vm_map_range(vm_as_t* as,
		     vaddr_t vaddr,
		     paddr_t paddr,
		     uint32_t size)
{
  uint32_t count;
  uint32_t i;

  TRACE_ENTRY();

  /* overflow
   */
  if ((vaddr + size) <= vaddr)
    return ERROR_INVALID_ADDR;
  if (((uint32_t)paddr + size) > phys_get_mem_size())
    return ERROR_INVALID_ADDR;
  if ((paddr + size) <= paddr)
    return ERROR_INVALID_ADDR;

  /* get page count
   */
  count = size / VM_PAGE_SIZE;
  if (size & ~VM_PAGE_MASK)
    ++count;

  /* map the range
   */
  for (i = 0; i < count; ++i)
    {
      vm_map(as, vaddr, paddr);
      vaddr += VM_PAGE_SIZE;
      paddr += VM_PAGE_SIZE;
    }

  return ERROR_SUCCESS;
}



/* unmap a contiguous range
 */

error_t vm_unmap_range(vm_as_t* as,
		       vaddr_t vaddr,
		       uint32_t size)
{
  uint32_t count;
  uint32_t i;

  TRACE_ENTRY();

  /* overflow
   */
  if ((vaddr + size) <= vaddr)
    return ERROR_INVALID_ADDR;

  /* get page count
   */
  count = size / VM_PAGE_SIZE;
  if (size & ~VM_PAGE_MASK)
    ++count;

  /* map the range
   */
  for (i = 0; i < count; ++i)
    {
      vm_unmap(as, vaddr);
      vaddr += VM_PAGE_SIZE;
    }

  return ERROR_SUCCESS;
}



/* get the kernel as
 */

vm_as_t* vm_get_kernel_as(void)
{
  return g_kernel_as;
}



/* get the current as
 */

vm_as_t* vm_get_current_as(void)
{
  return g_current_as;
}


/* dump the as
 */

void vm_dump_as(vm_as_t* as)
{
}
