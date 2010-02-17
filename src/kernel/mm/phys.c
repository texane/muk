/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sat Oct 13 19:14:16 2007 texane
** Last update Sat Nov 17 19:56:41 2007 texane
*/



#include "phys.h"
#include "../libc/libc.h"
#include "../debug/debug.h"
#include "../boot/multiboot.h"
#include "../sys/types.h"
#include "../arch/cpu.h"


/* physical allocator types
 */

typedef struct phys_desc
{
  struct phys_desc* next;
  struct phys_desc* prev;
  uint32_t ref;
} phys_desc_t;

typedef struct
{
  phys_desc_t* avail;
  uint32_t nframes;
  uint32_t size;
  uint32_t count;
  phys_desc_t descs[0];
} phys_allocator_t;

static phys_allocator_t* g_allocator = NULL;



/* physical page frame routines
 */

inline static bool_t phys_is_aligned(paddr_t addr)
{
  return ((uint32_t)addr & (PHYS_FRAME_SIZE - 1)) ? false : true;
}


inline static bool_t phys_desc_is_used(const phys_desc_t* desc)
{
  return desc->ref == 0 ? false : true;
}


inline static bool_t phys_desc_is_unused(const phys_desc_t* desc)
{
  return phys_desc_is_used(desc) == true ? false : true;
}


inline static phys_desc_t* paddr_to_phys_desc(paddr_t addr)
{
  if ((uint32_t)addr & (PHYS_FRAME_SIZE - 1))
    { serial_printl("[!] bug bug bug bug\n"); }

  /* offset in the desc array */
  return &g_allocator->descs[(unsigned long)addr / PHYS_FRAME_SIZE];
}


inline static paddr_t phys_desc_to_paddr(const phys_desc_t* desc)
{
  /* offset in the desc array * sizeof page frame */
  return
    (paddr_t)((unsigned long)(desc - g_allocator->descs) * PHYS_FRAME_SIZE);
}


inline static uint32_t phys_desc_to_index(const phys_desc_t* desc)
{
  return (uint32_t)(desc - g_allocator->descs);
}


static void phys_desc_unlink(phys_desc_t* desc)
{
  /* list head */
  if (g_allocator->avail == desc)
    g_allocator->avail = desc->next;
  else
    desc->prev->next = desc->next;

  if (desc->next != NULL)
    desc->next->prev = NULL;

  desc->next = NULL;
  desc->prev = NULL;
}


static void phys_desc_link(phys_desc_t* desc)
{
  if (g_allocator->avail != NULL)
    {
      desc->next = g_allocator->avail;
      g_allocator->avail->prev = desc;
    }
  g_allocator->avail = desc;
}


static void phys_desc_ref(phys_desc_t* desc)
{
  if (desc->ref == 0)
    phys_desc_unlink(desc);
  ++desc->ref;
}


static void phys_desc_unref(phys_desc_t* desc)
{
  --desc->ref;
  if (desc->ref == 0)
    phys_desc_link(desc);
}


static bool_t phys_desc_is_valid(const phys_desc_t* desc)
{
  /* descriptor is inside the desc array */
  if (phys_desc_to_index(desc) < g_allocator->count)
    return true;
  return false;
}


inline static uint32_t size_to_frame_count(uint32_t size)
{
  /* size to page frame count */
  return size / PHYS_FRAME_SIZE + ((size & (PHYS_FRAME_SIZE - 1)) ? 1 : 0);
}



/* multiboot routines
 */

static int mbi_find_space(const multiboot_info_t* mbi,
			  uint32_t space,
			  phys_allocator_t** res)
{
  memory_map_t* mmap;
  paddr_t addr;
  uint32_t size;

  *res = NULL;

  for (mmap = (memory_map_t*)mbi->mmap_addr;
       mmap < (memory_map_t*)mbi->mmap_addr + mbi->mmap_length;
       mmap = (memory_map_t*)((unsigned long)mmap + mmap->size +
			      sizeof(mmap->size)))
    {
      /* ensure address is at page boundary
       */
      addr = (paddr_t)mmap->base_addr_low;
      size = mmap->length_low & ~(PHYS_FRAME_SIZE - 1);
      if (phys_is_aligned(addr) == false)
	{
	  size = (uint32_t)addr % PHYS_FRAME_SIZE;
	  if ((mmap->length_low - size) >= PHYS_FRAME_SIZE)
	    {
	      addr = (paddr_t)mmap->base_addr_low + (PHYS_FRAME_SIZE - size);
	      size = mmap->length_low - size;
	    }
	  else
	    {
	      addr = NULL;
	      size = 0;
	    }
	}

      if ((mmap->type == 1) &&
	  (size >= space))
	{
	  *res = (phys_allocator_t*)mmap->base_addr_low;
	  return 0;
	}
    }

  return -1;
}


static void mbi_dump(const multiboot_info_t* mbi)
{
  memory_map_t* mmap;

  for (mmap = (memory_map_t*)mbi->mmap_addr;
       mmap < (memory_map_t*)(mbi->mmap_addr + mbi->mmap_length);
       mmap = (memory_map_t*)((unsigned long)mmap + mmap->size +
			      sizeof(mmap->size)))
    {
      serial_printl(" [+] 0x%x - 0x%x (%s)\n",
		    (uint32_t)mmap->base_addr_low,
		    (uint32_t)(mmap->base_addr_low + mmap->length_low),
		    mmap->type == 1 ? "avail" : "reserved");
    }
}


static void mbi_reserve_space(const multiboot_info_t* mbi)
{
  memory_map_t* mmap;
  phys_desc_t* desc;
  paddr_t addr;
  uint32_t size;
  uint32_t count;
  uint32_t i;

  if (!(mbi->flags & (1 << 6)))
    return ;
  
  /* find available memory areas
   */
  for (mmap = (memory_map_t*)mbi->mmap_addr;
       mmap < (memory_map_t*)(mbi->mmap_addr + mbi->mmap_length);
       mmap = (memory_map_t*)((unsigned long)mmap + mmap->size +
			      sizeof(mmap->size)))
    {
      /* ensure address is at page boundary
       */
      addr = (paddr_t)mmap->base_addr_low;
      size = mmap->length_low & ~(PHYS_FRAME_SIZE - 1);
      if (phys_is_aligned(addr) == false)
	{
	  size = (uint32_t)addr % PHYS_FRAME_SIZE;
	  if ((mmap->length_low - size < mmap->length_low) &&
	      (mmap->length_low - size >= PHYS_FRAME_SIZE))
	    {
	      addr = (paddr_t)(mmap->base_addr_low + (PHYS_FRAME_SIZE - size));
	      size = mmap->length_low - size;
	    }
	  else
	    {
	      addr = NULL;
	      size = 0;
	    }
	}

      /* add the map, check for overflow
       */
      if (size &&
	  ((addr + size) > addr) &&
	  ((uint32_t)(addr + size) <= g_allocator->size))
	{
	  desc = paddr_to_phys_desc(addr);
	  count = size_to_frame_count(size);

	  /* available memory
	   */
	  if (mmap->type == 1)
	    {
	      serial_printl(" [+] adding %x - %x\n",
			    (uint32_t)addr,
			    (uint32_t)(addr + size));

	      for (i = 0; i < count; ++i)
		phys_desc_link(&desc[i]);
	    }
	  else
	    {
	      /* reference page
	       */

	      serial_printl(" [+] adding2 %x - %x\n",
			    (uint32_t)addr,
			    (uint32_t)(addr + size));

	      for (i = 0; i < count; ++i)
		++desc[i].ref;
	    }
	}
    }
}


static uint32_t mbi_get_mem_size(const multiboot_info_t* mbi)
{
  /* walk the memory map and find
     the highest address available.
     round it to the nearest multiple
     of page size. this one is the
     available memory size.
   */

  memory_map_t* mmap;
  uint32_t size;
  uint32_t addr;

  size = 0;
  for (mmap = (memory_map_t*)mbi->mmap_addr;
       mmap < (memory_map_t*)(mbi->mmap_addr + mbi->mmap_length);
       mmap = (memory_map_t*)((unsigned long)mmap + mmap->size +
			      sizeof(mmap->size)))
    {
      if (mmap->type == 1)
	{
	  addr = (uint32_t)(mmap->base_addr_low + mmap->length_low);
	  addr = addr & ~(PHYS_FRAME_SIZE - 1);
	  if (addr > size)
	    size = addr;
	}
    }

  return size;
}



/* physical allocator routines
 */

error_t phys_init(const multiboot_info_t* mbi)
{
  phys_desc_t* desc;
  uint32_t i;
  uint32_t count;
  uint32_t sz_mem;
  uint32_t sz_allocator;

  mbi_dump(mbi);

  /* get amount of memory from mbi
   */
  sz_mem = mbi_get_mem_size(mbi);
  if (sz_mem == 0)
    return ERROR_NO_MEMORY;

  /* allocate the allocator
   */
  count = size_to_frame_count(sz_mem);
  sz_allocator = sizeof(phys_allocator_t) + count * sizeof(phys_desc_t);
  if (mbi_find_space(mbi, sz_allocator, &g_allocator) == -1)
    return ERROR_NO_MEMORY;
  memset(g_allocator, 0, sz_allocator);

  /* initialize
   */
  g_allocator->nframes =
    sz_allocator / PHYS_FRAME_SIZE +
    (sz_allocator & (PHYS_FRAME_SIZE - 1)) ? 1 : 0;
  g_allocator->size = sz_mem;
  g_allocator->count = count;
  g_allocator->avail = NULL;

  /* mark pages accordingly to multiboot
   */
  mbi_reserve_space(mbi);

  /* mark allocator pages as used
   */
  desc = paddr_to_phys_desc((paddr_t)g_allocator);
  count = sz_allocator / PHYS_FRAME_SIZE;
  if (sz_allocator & (PHYS_FRAME_SIZE - 1))
    ++count;
  for (i = 0; i < count; ++i)
    phys_desc_ref(&desc[i]);

  /* mark kernel pages as used
   */

  /* mark module pages as used
   */

  /* NULL is not a valid address
   */
  phys_desc_ref(&desc[0]);

  return ERROR_SUCCESS;
}


error_t phys_release(void)
{
  return ERROR_SUCCESS;
}


void phys_debug(void)
{
  phys_desc_t* pos;
  uint32_t n;
  uint32_t i;
  uint32_t ref;

  serial_printl("allocator\n");
  serial_printl("{\n");

  serial_printl(" . g_all: %x\n", (uint32_t)g_allocator);
  serial_printl(" . memsz: %x\n", (uint32_t)phys_get_mem_size());
  serial_printl(" . descs: %x\n", (uint32_t)g_allocator->descs);
  serial_printl(" . avail: %x\n", (uint32_t)g_allocator->avail);
  serial_printl(" . count: %d\n", g_allocator->count);

  pos = g_allocator->descs;
  n = 0;
  while (n < g_allocator->count)
    {
      i = n;
      ref = pos[i].ref ? 1 : 0;
      while ((i < g_allocator->count) && (ref ? (pos[i].ref) : !pos[i].ref))
	++i;
      serial_printl(" [+] %s: 0x%x - 0x%x [%d]\n",
		    ref ? "used" : "avail",
		    (uint32_t)phys_desc_to_paddr(&pos[n]),
		    (uint32_t)phys_desc_to_paddr(&pos[i]),
		    i - n);
      n = i;
    }
  serial_printl("}\n");
}


void phys_test(void)
{
  paddr_t a;
  paddr_t b;
  paddr_t c;

  serial_printl("[?] test\n");

  phys_debug();

  a = phys_alloc();
  serial_printl("%x\n", (uint32_t)a);
  phys_free(a);

  a = phys_alloc();
  serial_printl("%x\n", (uint32_t)a);
  phys_free(a);

  phys_debug();
  a = phys_alloc();
  serial_printl("%x\n", (uint32_t)a);
  b = phys_alloc();
  serial_printl("%x\n", (uint32_t)b);
  c = phys_alloc();
  serial_printl("%x\n", (uint32_t)c);
  phys_debug();

  phys_free(a);
  phys_free(b);
  phys_free(c);

  phys_debug();
  b = phys_alloc_range(101);
  serial_printl("b == %x\n", (uint32_t)b);
  phys_free_range(b, 101);
  phys_debug();

  phys_debug();
}


uint32_t phys_get_mem_size(void)
{
  return g_allocator->size;
}


void phys_get_allocator_range(paddr_t* addr,
			      uint32_t* count)
{
  *addr = (paddr_t)g_allocator;
  *count = g_allocator->nframes;
}


paddr_t phys_alloc(void)
{
  phys_desc_t* desc;

  if (g_allocator->avail == NULL)
    return (paddr_t)NULL;

  /* pick the first avail desc
   */
  desc = g_allocator->avail;

  /* reference the descriptor
   */
  phys_desc_ref(desc);

  return phys_desc_to_paddr(desc);
}


void phys_free(paddr_t addr)
{
  phys_desc_unref(paddr_to_phys_desc(addr));
}


paddr_t phys_alloc_range(uint32_t count)
{
  /* fixme: some descriptors are walked twice
   */

  phys_desc_t* pos;
  phys_desc_t* res;
  uint32_t i;

  /* walk all free descriptors
   */
  res = NULL;
  for (pos = g_allocator->avail;
       (res == NULL) && (pos != NULL);
       pos = pos->next)
    {
      /* find contiguous area
       */
      for (i = 0;
	   (i < count) &&
	     (phys_desc_is_unused(&pos[i]) == true) &&
	     (phys_desc_is_valid(&pos[i]) == true);
	   ++i)
	;

      /* found */
      if (i == count)
	res = pos;
    }

  /* not found
   */
  if (res == NULL)
    return 0;

  /* reference descriptors
   */
  for (i = 0; i < count; ++i)
    phys_desc_ref(&res[i]);

  return phys_desc_to_paddr(res);
}


void phys_free_range(paddr_t addr, uint32_t count)
{
  while (count--)
    {
      phys_desc_unref(paddr_to_phys_desc(addr));
      addr += PHYS_FRAME_SIZE;
    }
}
