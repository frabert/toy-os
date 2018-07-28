#include "kheap.h"
#include "kassert.h"
#include "paging.h"

extern os::Paging::PageDirectory* kernel_directory;

extern uint8_t end;
uintptr_t placement_address = (uintptr_t)&end;

os::Heap* kernel_heap = nullptr;

void* kmalloc(size_t size) {
  if (kernel_heap != nullptr) {
    return kernel_heap->alloc(size, false); 
  } else {
    uintptr_t tmp = placement_address;
    placement_address += size;
    return (void*)tmp;
  }
}

void* kmalloc_align(size_t size, void** physical) {
  if (kernel_heap != nullptr) {
    void *addr = kernel_heap->alloc(size, true);
    auto page = os::Paging::get_page((uintptr_t)addr, 0, kernel_directory);
    *physical = (void*)(page->frame*0x1000 + ((uintptr_t)addr&0xFFF));
    return addr;
  } else {
    if (placement_address & 0xFFFFF000) {
      // Align the placement address;
      placement_address &= 0xFFFFF000;
      placement_address += 0x1000;
    }

    *physical = (void*)placement_address;
    uintptr_t tmp = placement_address;
    placement_address += size;
    return (void*)tmp;
  }
}

void kfree(void* p) {
  KASSERT(kernel_heap != nullptr);
  kernel_heap->free(p);
}

void *krealloc(void* p, size_t s) {
  KASSERT(kernel_heap != nullptr);
  return kernel_heap->realloc(p, s);
}

void *operator new(size_t size)
{
  return kmalloc(size);
}
 
void *operator new[](size_t size)
{
  return kmalloc(size);
}
 
void operator delete(void *p)
{
  kfree(p);
}
 
void operator delete[](void *p)
{
  kfree(p);
}
 
void operator delete(void *p, size_t s)
{
  kfree(p);
}
 
void operator delete[](void *p, size_t s)
{
  kfree(p);
}

struct Footer {
    uint32_t magic;     // Magic number, same as in Header.
    os::Header *header; // Pointer to the block header.
};

os::Heap::Heap(uintptr_t start, uintptr_t end_addr, uintptr_t max, bool supervisor, bool readonly)
    : m_index((Header**)start, HEAP_INDEX_SIZE) {
  // All our assumptions are made on startAddress and endAddress being page-aligned.
  KASSERT(start%0x1000 == 0);
  KASSERT(end_addr%0x1000 == 0);
    
  // Shift the start address forward to resemble where we can start putting data.
  start += sizeof(Header*)*HEAP_INDEX_SIZE;

  // Make sure the start address is page-aligned.
  if ((start & 0xFFFFF000) != 0) {
    start &= 0xFFFFF000;
    start += 0x1000;
  }

  // Write the start, end and max addresses into the heap structure.
  m_start_address = start;
  m_end_address = end_addr;
  m_max_address = max;
  m_supervisor = supervisor;
  m_readonly = readonly;

  // We start off with one large hole in the index.
  Header *hole = (Header *)start;
  hole->size = end_addr-start;
  hole->magic = HEAP_MAGIC;
  hole->is_hole = true;
  m_index.insert(hole);
}

os::Heap::container_type::iterator os::Heap::findSmallestHole(size_t size, bool page_align) {
  // Find the smallest hole that will fit.
  auto iterator = m_index.begin();
  while (iterator != m_index.end()) {
    Header *header = *iterator;
    // If the user has requested the memory be page-aligned
    if (page_align) {
      // Page-align the starting point of this header.
      uintptr_t location = (uintptr_t)header;
      int32_t offset = 0;
      if (((location+sizeof(Header)) & 0xFFFFF000) != 0)
          offset = 0x1000 /* page size */  - (location+sizeof(Header))%0x1000;
      int32_t hole_size = (int32_t)header->size - offset;
      // Can we fit now?
      if (hole_size >= (int32_t)size)
          break;
    }
    else if (header->size >= size)
        break;
    iterator++;
  }

  return iterator;
}

void os::Heap::expand(size_t newSize) {
  // Sanity check.
  KASSERT(newSize > m_end_address - m_start_address);

  // Get the nearest following page boundary.
  if ((newSize&0xFFFFF000) != 0)
  {
      newSize &= 0xFFFFF000;
      newSize += 0x1000;
  }

  // Make sure we are not overreaching ourselves.
  KASSERT(m_start_address+newSize <= m_max_address);

  // This should always be on a page boundary.
  size_t old_size = m_end_address-m_start_address;

  uint32_t i = old_size;
  while (i < newSize)
  {
    os::Paging::alloc_frame( os::Paging::get_page(m_start_address+i, 1, kernel_directory),
                 m_supervisor, m_readonly);
    i += 0x1000 /* page size */;
  }
  m_end_address = m_start_address+newSize;
}

int32_t os::Heap::contract(size_t new_size) {
  // Sanity check.
  KASSERT(new_size < m_end_address-m_start_address);

  // Get the nearest following page boundary.
  if (new_size&0x1000) {
    new_size &= 0x1000;
    new_size += 0x1000;
  }

  // Don't contract too far!
  if (new_size < HEAP_MIN_SIZE)
    new_size = HEAP_MIN_SIZE;

  int32_t old_size = m_end_address-m_start_address;
  int32_t i = old_size - 0x1000;
  while (new_size < i) {
    os::Paging::free_frame(os::Paging::get_page(m_start_address+i, 0, kernel_directory));
    i -= 0x1000;
  }

  m_end_address = m_start_address + new_size;
  return new_size;
}

void* os::Heap::alloc(size_t size, bool align) {
  // Make sure we take the size of header/footer into account.
  size_t new_size = size + sizeof(Header) + sizeof(Footer);
  // Find the smallest hole that will fit.
  auto iterator = findSmallestHole(new_size, align);

  if (iterator == m_index.end()) {
    // Save some previous data.
    size_t old_length = m_end_address - m_start_address;
    size_t old_end_address = m_end_address;

    // We need to allocate some more space.
    expand(old_length+new_size);
    size_t new_length = m_end_address-m_start_address;

    // Find the endmost header. (Not endmost in size, but in location).
    iterator = m_index.begin();
    // Vars to hold the index of, and value of, the endmost header found so far.
    auto idx = m_index.end(); uintptr_t value = 0;
    while (iterator != m_index.end()) {
      uintptr_t tmp = (uintptr_t)*iterator;
      if (tmp > value) {
          value = tmp;
          idx = iterator;
      }
      iterator++;
    }

    // If we didn't find ANY headers, we need to add one.
    if (idx == m_index.end()) {
      Header *header = (Header *)old_end_address;
      header->magic = HEAP_MAGIC;
      header->size = new_length - old_length;
      header->is_hole = 1;
      Footer *footer = (Footer *) (old_end_address + header->size - sizeof(Footer));
      footer->magic = HEAP_MAGIC;
      footer->header = header;
      m_index.insert(header);
    } else {
      // The last header needs adjusting.
      Header *header = *idx;
      header->size += new_length - old_length;
      // Rewrite the footer.
      Footer *footer = (Footer *) ( (uintptr_t)header + header->size - sizeof(Footer) );
      footer->header = header;
      footer->magic = HEAP_MAGIC;
    }
    // We now have enough space. Recurse, and call the function again.
    return alloc(size, align);
  }

  Header *orig_hole_header = *iterator;
  uintptr_t orig_hole_pos = (uintptr_t)orig_hole_header;
  size_t orig_hole_size = orig_hole_header->size;
  // Here we work out if we should split the hole we found into two parts.
  // Is the original hole size - requested hole size less than the overhead for adding a new hole?
  if (orig_hole_size-new_size < sizeof(Header)+sizeof(Footer)) {
    // Then just increase the requested size to the size of the hole we found.
    size += orig_hole_size-new_size;
    new_size = orig_hole_size;
  }

  // If we need to page-align the data, do it now and make a new hole in front of our block.
  if (align && orig_hole_pos&0xFFFFF000) {
    uintptr_t new_location   = orig_hole_pos + 0x1000 /* page size */ - (orig_hole_pos&0xFFF) - sizeof(Header);
    Header *hole_header = (Header *)orig_hole_pos;
    hole_header->size     = 0x1000 /* page size */ - (orig_hole_pos&0xFFF) - sizeof(Header);
    hole_header->magic    = HEAP_MAGIC;
    hole_header->is_hole  = 1;
    Footer *hole_footer = (Footer *) ( (uintptr_t)new_location - sizeof(Footer) );
    hole_footer->magic    = HEAP_MAGIC;
    hole_footer->header   = hole_header;
    orig_hole_pos         = new_location;
    orig_hole_size        = orig_hole_size - hole_header->size;
  } else {
    // Else we don't need this hole any more, delete it from the index.
    m_index.remove(iterator);
  }

  // Overwrite the original header...
  Header *block_header  = (Header *)orig_hole_pos;
  block_header->magic     = HEAP_MAGIC;
  block_header->is_hole   = 0;
  block_header->size      = new_size;
  // ...And the footer
  Footer *block_footer  = (Footer *) (orig_hole_pos + sizeof(Header) + size);
  block_footer->magic     = HEAP_MAGIC;
  block_footer->header    = block_header;

  // We may need to write a new hole after the allocated block.
  // We do this only if the new hole would have positive size...
  if (orig_hole_size - new_size > 0) {
    Header *hole_header = (Header *) (orig_hole_pos + sizeof(Header) + size + sizeof(Footer));
    hole_header->magic    = HEAP_MAGIC;
    hole_header->is_hole  = 1;
    hole_header->size     = orig_hole_size - new_size;
    Footer *hole_footer = (Footer *) ( (uintptr_t)hole_header + orig_hole_size - new_size - sizeof(Footer) );
    if ((uintptr_t)hole_footer < m_end_address) {
      hole_footer->magic = HEAP_MAGIC;
      hole_footer->header = hole_header;
    }
    // Put the new hole in the index;
    m_index.insert(hole_header);
  }
  
  // ...And we're done!
  return (void *) ( (uintptr_t)block_header+sizeof(Header) );
}

void os::Heap::free(void *p) {
  // Exit gracefully for null pointers.
  if (p == nullptr)
    return;

  // Get the header and footer associated with this pointer.
  Header *header = (Header*) ( (uintptr_t)p - sizeof(Header) );
  Footer *footer = (Footer*) ( (uintptr_t)header + header->size - sizeof(Footer) );

  // Sanity checks.
  KASSERT(header->magic == HEAP_MAGIC);
  KASSERT(footer->magic == HEAP_MAGIC);

  // Make us a hole.
  header->is_hole = true;

  // Do we want to add this header into the 'free holes' index?
  bool do_add = true;

  // Unify left
  // If the thing immediately to the left of us is a footer...
  Footer *test_footer = (Footer*) ( (uintptr_t)header - sizeof(Footer) );
  if (test_footer->magic == HEAP_MAGIC &&
    test_footer->header->is_hole == 1) {
    size_t cache_size = header->size; // Cache our current size.
    header = test_footer->header;     // Rewrite our header with the new one.
    footer->header = header;          // Rewrite our footer to point to the new header.
    header->size += cache_size;       // Change the size.
    do_add = false;                       // Since this header is already in the index, we don't want to add it again.
  }

  // Unify right
  // If the thing immediately to the right of us is a header...
  Header *test_header = (Header*) ( (uintptr_t)footer + sizeof(Footer) );
  if (test_header->magic == HEAP_MAGIC &&
    test_header->is_hole) {
    header->size += test_header->size; // Increase our size.
    test_footer = (Footer*) ( (uintptr_t)test_header + // Rewrite it's footer to point to our header.
                                test_header->size - sizeof(Footer) );
    footer = test_footer;
    // Find and remove this header from the index.
    auto iterator = m_index.begin();
    while ( (iterator != m_index.end()) &&
            *iterator != test_header )
        iterator++;

    // Make sure we actually found the item.
    KASSERT(iterator != m_index.end());
    // Remove it.
    m_index.remove(iterator);
  }

  // If the footer location is the end address, we can contract.
  if ( (uintptr_t)footer+sizeof(Footer) == m_end_address) {
    size_t old_length = m_end_address-m_start_address;
    size_t new_length = contract( (uintptr_t)header - m_start_address);
    // Check how big we will be after resizing.
    if (header->size - (old_length-new_length) > 0) {
      // We will still exist, so resize us.
      header->size -= old_length-new_length;
      footer = (Footer*) ( (uintptr_t)header + header->size - sizeof(Footer) );
      footer->magic = HEAP_MAGIC;
      footer->header = header;
    } else {
      // We will no longer exist :(. Remove us from the index.
      auto iterator = m_index.begin();
      while ( (iterator != m_index.end()) &&
              (*iterator != test_header) )
          iterator++;
      // If we didn't find ourselves, we have nothing to remove.
      if (iterator != m_index.end())
        m_index.remove(iterator);
    }
  }

  // If required, add us to the index.
  if (do_add)
    m_index.insert(header);
}

void *os::Heap::realloc(void* p, size_t s) {
  KASSERT(p != nullptr);
  // Get the header and footer associated with this pointer.
  Header *header = (Header*) ( (uintptr_t)p - sizeof(Header) );
  Footer *footer = (Footer*) ( (uintptr_t)header + header->size - sizeof(Footer) );

  // Sanity checks.
  KASSERT(header->magic == HEAP_MAGIC);
  KASSERT(footer->magic == HEAP_MAGIC);

  // TODO: make this more efficient
  void* new_mem = alloc(s);
  if(s > header->size) {
    memcpy(new_mem, p, header->size);
  } else {
    memcpy(new_mem, p, s);
  }
  free(p);
  return new_mem;
}