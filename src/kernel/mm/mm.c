/*
** Made by texane <texane@gmail.com>
** 
** Started on  Wed Aug 22 23:21:28 2007 texane
** Last update Mon Nov 12 15:01:42 2007 texane
*/



#include "mm.h"
#include "phys.h"
#include "../sys/types.h"
#include "../libc/libc.h"
#include "../debug/debug.h"
#include "../boot/multiboot.h"



/* memory chunk
 */
typedef struct chunk
{
  size_t size;
  bool_t is_last;
  bool_t is_used;
  unsigned char storage[0];
} chunk_t;


/* memory allocator
 */
typedef struct
{
  chunk_t* head;
  chunk_t* tail;
  addr_t base;
  size_t size;
} allocator_t;



/* global allocator
 */
static allocator_t* g_allocator = null;


/* get next chunk
 */
static inline chunk_t* next_chunk(chunk_t* chunk)
{
  if (chunk->is_last == false)
    return (chunk_t*)((addr_t)chunk + chunk->size);
  return null;
}


/* get chunk containing a pointer
 */
static inline chunk_t* containing_chunk(void* p)
{
  return (chunk_t*)((addr_t)p - offset_of(chunk_t, storage));
}


static inline void alloc_chunk(chunk_t* chunk, size_t size)
{
  chunk->size = sizeof(chunk_t) + size;
  chunk->is_used = true;
  chunk->is_last = true;
}


/* exported
 */

int mm_initialize(addr_t base, size_t size)
{
  chunk_t* chunk;

  if (g_allocator != null)
    return -1;

  if (size < (sizeof(chunk_t) + sizeof(allocator_t)))
    return -1;

  /* self allocate */
  chunk = (chunk_t*)base;
  alloc_chunk(chunk, sizeof(allocator_t));

  /* init allocator */
  g_allocator = (allocator_t*)&chunk->storage;
  g_allocator->head = chunk;
  g_allocator->tail = chunk;
  g_allocator->base = base;
  g_allocator->size = size;

  return 0;
}


uint32_t mm_get_size(void)
{
  return g_allocator->size;
}


void* mm_alloc(size_t size)
{
#if 0
  chunk_t* chunk;

  /* todo: check if enough space */
  chunk = g_allocator->tail;
  chunk->is_last = false;
  chunk = next_chunk(chunk);
  alloc_chunk(chunk, size);
  g_allocator->tail = chunk;

  return chunk->storage;
#else
  if (size > PHYS_FRAME_SIZE)
    {
      size_t count;

      if (size & (PHYS_FRAME_SIZE - 1))
	count = size / PHYS_FRAME_SIZE + 1;
      else
	count = size / PHYS_FRAME_SIZE;
      return (void*)phys_alloc_range(count);
    }

  return (void*)phys_alloc();
#endif
}


void mm_free(void* p)
{
#if 0
  chunk_t* chunk;

  chunk = containing_chunk(p);
  chunk->is_used = false;
#else
  phys_free((paddr_t)p);
#endif
}


void mm_debug(void)
{
  chunk_t* pos;

  serial_printl("mm\n");
  serial_printl("{\n");
  serial_printl(".base == 0x%x\n", (uint_t)g_allocator->base);
  serial_printl(".size == 0x%x\n", (uint_t)g_allocator->size);

  pos = containing_chunk(g_allocator);
  while ((pos = next_chunk(pos)))
    serial_printl("{0x%x, %d}\n", (uint_t)pos, pos->size);

  serial_printl("}\n");
}


void mm_test(void)
{
  int i;
  void* v[10];

  for (i = 0; i < sizeof(v) / sizeof(void*); ++i)
    v[i] = mm_alloc(sizeof(int));
  mm_debug();

  for (i = 0; i < sizeof(v) / sizeof(void*); ++i)
    mm_free(v[i]);
  mm_debug();
}



/* page frame allocator
 */
typedef struct pf_desc
{
  struct pf_desc* next;
  struct pf_desc* prev;
  count_t ref;
} pf_desc_t;

typedef struct
{
  pf_desc_t* avail;
  count_t count;
  pf_desc_t descs[0];
} pf_allocator_t;

static pf_allocator_t* g_pf_allocator = null;

#define PFRAME_SIZE 0x1000


inline static bool_t pf_desc_is_used(const pf_desc_t* desc)
{
  return desc->ref == 0 ? false : true;
}


inline static bool_t pf_desc_is_unused(const pf_desc_t* desc)
{
  return pf_desc_is_used(desc) == true ? false : true;
}


inline static pf_desc_t* paddr_to_pf_desc(paddr_t addr)
{
  /* offset in the desc array */
  return &g_pf_allocator->descs[(unsigned long)addr / PFRAME_SIZE];
}


inline static paddr_t pf_desc_to_paddr(const pf_desc_t* desc)
{
  /* offset in the desc array * sizeof page frame */
  return
    (paddr_t)((unsigned long)(desc - g_pf_allocator->descs) * PFRAME_SIZE);
}


inline static count_t pf_desc_to_index(const pf_desc_t* desc)
{
  /* offset in the desc array */
  return (count_t)(desc - g_pf_allocator->descs);
}


inline static count_t size_to_pf_count(size_t size)
{
  /* size to page frame count */
  return (size / PFRAME_SIZE) + ((size & ~(PFRAME_SIZE - 1)) ? 1 : 0);
}


static void pf_desc_unlink(pf_desc_t* desc)
{
  /* list header */
  if (g_pf_allocator->avail == desc)
    g_pf_allocator->avail = desc->next;
  else
    desc->prev->next = desc->next;

  if (!is_null(desc->next))
    desc->next->prev = null;

  desc->next = null;
  desc->prev = null;
}


static void pf_desc_link(pf_desc_t* desc)
{
  if (!is_null(g_pf_allocator->avail))
    {
      desc->next = g_pf_allocator->avail;
      g_pf_allocator->avail->prev = desc;
    }
  g_pf_allocator->avail = desc;
}


static void pf_desc_ref(pf_desc_t* desc)
{
  if (desc->ref == 0)
    pf_desc_unlink(desc);
  ++desc->ref;
}


static void pf_desc_unref(pf_desc_t* desc)
{
  --desc->ref;
  if (desc->ref == 0)
    pf_desc_link(desc);
}


static bool_t pf_desc_is_valid(const pf_desc_t* desc)
{
  /* descriptor is inside the desc array */
  if (pf_desc_to_index(desc) < g_pf_allocator->count)
    return true;

  return false;
}


static paddr_t pframe_alloc_one(void)
{
  pf_desc_t* desc;
  
  if (is_null(g_pf_allocator->avail))
    return 0;

  /* pick the first avail desc */
  desc = g_pf_allocator->avail;

  /* reference the descriptor */
  pf_desc_ref(desc);

  return pf_desc_to_paddr(desc);
}


static paddr_t pframe_alloc_counted(count_t count)
{
  /* fixme: some descriptors are walked twice
   */

  pf_desc_t* pos;
  pf_desc_t* res;
  size_t i;

  /* walk all free descriptors */
  res = null;
  for (pos = g_pf_allocator->avail;
       is_null(res) && !is_null(pos);
       pos = pos->next)
    {
      /* find contiguous area */
      for (i = 0;
	   (i < count) && (pf_desc_is_unused(&pos[i]) == true) && (pf_desc_is_valid(&pos[i]) == true);
	   ++i)
	;

      /* pframe not found */
      if (i == count)
	res = pos;
    }

  /* not found */
  if (is_null(res))
    return 0;

  /* reference descriptors */
  for (i = 0; i < count; ++i)
    pf_desc_ref(&res[i]);

  return pf_desc_to_paddr(res);
}


paddr_t pframe_alloc(count_t count)
{
  if (count == 1)
    return pframe_alloc_one();
  return pframe_alloc_counted(count);
}


void pframe_free(paddr_t addr)
{
  pf_desc_unref(paddr_to_pf_desc(addr));
}


static void* mbi_find_space(multiboot_info_t* mbi, size_t size)
{
  memory_map_t* mmap;

  for (mmap = (memory_map_t*)mbi->mmap_addr;
       (unsigned long)mmap < mbi->mmap_addr + mbi->mmap_length;
       mmap = (memory_map_t*)((unsigned long)mmap + mmap->size + sizeof(mmap->size)))
    {
      if (mmap->type == 1)
	{
	  if ((size_t)mmap->length_low >= size)
	    return (void*)mmap->base_addr_low;
	}
    }

  return null;
}


static void mbi_reserve_space(multiboot_info_t* mbi)
{
  memory_map_t* mmap;
  pf_desc_t* desc;
  count_t count;
  count_t i;

  if (!(mbi->flags & (1 << 6)))
    return ;

  /* find available memory areas */
  for (mmap = (memory_map_t*)mbi->mmap_addr;
       (unsigned long)mmap < mbi->mmap_addr + mbi->mmap_length;
       mmap = (memory_map_t*)((unsigned long)mmap + mmap->size + sizeof(mmap->size)))
    {
      if (mmap->type == 1)
	{
	  desc = paddr_to_pf_desc((paddr_t)mmap->base_addr_low);
	  count = size_to_pf_count((size_t)mmap->length_low);
	  for (i = 0; i < count; ++i)
	    pf_desc_link(&desc[i]);
	}
    }
}


static size_t mbi_get_mem_size(multiboot_info_t* mbi)
{
  if (!(mbi->flags & 1))
    return 0;
  return mbi->mem_upper * 1024 + 1024 * 1024;
}


int pframe_initialize_with_mbi(multiboot_info_t* mbi)
{
  /* initialize the pframe allocator
   */

  size_t sz_allocator;
  pf_desc_t* desc;
  count_t i;
  count_t count;
  size_t sz_mem;

  /* get amount of memory from mbi */
  sz_mem = mbi_get_mem_size(mbi);
  if (sz_mem == 0)
    return -1;

  /* allocate the allocator */
  count = size_to_pf_count(sz_mem);
  sz_allocator = sizeof(pf_allocator_t) + count * sizeof(pf_desc_t);
  g_pf_allocator = mbi_find_space(mbi, sz_allocator);
  if (is_null(g_pf_allocator))
    return -1;

  /* initialize */
  g_pf_allocator->count = count;
  g_pf_allocator->avail = null;

  /* all page frame avail */
  for (i = 0; i < g_pf_allocator->count; ++i)
    {
      g_pf_allocator->descs[i].next = null;
      g_pf_allocator->descs[i].prev = null;
      g_pf_allocator->descs[i].ref = 0;
      pf_desc_link(&g_pf_allocator->descs[i]);
    }

  /* mark pages accordingly to multiboot */
  mbi_reserve_space(mbi);

  /* mark allocator pages as used */
  desc = paddr_to_pf_desc((paddr_t)g_pf_allocator);
  count = sz_allocator / PFRAME_SIZE;
  if (sz_allocator % PFRAME_SIZE)
    ++count;
  for (i = 0; i < count; ++i)
    pf_desc_ref(&g_pf_allocator->descs[i]);

  return 0;
}


void pframe_debug(void)
{
  pf_desc_t* pos;
  count_t i;

  serial_printl("allocator\n");
  serial_printl("{\n");
  serial_printl(" .count == %u\n", g_pf_allocator->count);
  serial_printl(" .avail == [\n");
  for (pos = g_pf_allocator->avail; pos; pos = pos->next)
    serial_printl("                [+] 0x%x\n", (unsigned int)pf_desc_to_paddr(pos));
  serial_printl("           ]\n");
  serial_printl(" .descs == [\n");
  for (i = 0; i < g_pf_allocator->count; ++i)
    serial_printl("                [+] 0x%x {%u}\n", (unsigned int)pf_desc_to_paddr(&g_pf_allocator->descs[i]), g_pf_allocator->descs[i].ref);
  serial_printl("           ]\n");
  serial_printl("}\n");
}


void pframe_test(void)
{
  pframe_debug();
}
