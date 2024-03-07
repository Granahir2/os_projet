#include "x86/memory.hh"

template <short depth, short log2_page_size = 12>
class BuddyAlloc {
public:
    BuddyAlloc() : is_free(true) {}
    x64::linaddr get_range(size_t size) 
    {
        if (size > total_size)
        {
            return -1;
        }
        
        if (size <= mid_point)
        {
            if (children[0].is_free) 
            {
                is_free = false;
                return children[0].get_range(size);
            }
            else if (children[1].is_free) 
            {
                is_free = false;
                return 1 << (depth - 1 + log2_page_size) + children[1].get_range(size);
            } 
            else 
            {
                return 1;
            }
        } 
        else 
        {
            if (is_free) 
            {
                is_free = false;
                return 0;
            } 
            else 
            {
                return 1;
            }
        }
    }

    bool is_range_free(x64::linaddr addr, size_t size) 
    {
        if (addr + size > total_size) 
        {
            return false;
        }
        if (addr >= mid_point)
        {
            return children[1].is_range_free(addr - mid_point, size);
        }
        else if (addr + size <= mid_point)
        {
            return children[0].is_range_free(addr, size);
        }
        else 
        {
            return is_free;
        }
    }

    void free_range(x64::linaddr addr, size_t size) 
    {
        if (addr + size > total_size) 
        {
            return;
        }
        if (addr >= mid_point)
        {
            children[1].free_range(addr - mid_point, size);
        }
        else if (addr + size <= mid_point)
        {
            children[0].free_range(addr, size);
        }
        else 
        {
            is_free = true;
        }
    }

    void mark_used(x64::linaddr addr_begin, x64::linaddr addr_end) {
        size_t size = addr_end - addr_begin;
        if (addr_end >= total_size || addr_begin > addr_end) 
        {
            return;
        }
        if (addr_begin >= mid_point)
        {
            children[1].mark_used(addr_begin - mid_point, addr_end - mid_point);
        }
        else if (addr_end < mid_point)
        {
            children[0].mark_used(addr_begin, addr_end);
        }
        else 
        {
            is_free = false;
        }
    }

private:
    bool is_free;
    BuddyAlloc<depth-1, log2_page_size> children[2];
    static const x64::linaddr mid_point = 1 << (depth - 1 + log2_page_size);
    static const x64::linaddr total_size = 1 << (depth + log2_page_size);
};

template <short log2_page_size>
class BuddyAlloc<0, log2_page_size> {
private:
    bool is_free;
    static const x64::linaddr total_size = 1 << log2_page_size;
public:
    BuddyAlloc() : is_free(true) {}
    x64::linaddr get_range(size_t size) 
    {
        if (size > total_size)
        {
            return 1;
        }
        if (is_free) 
        {
            is_free = false;
            return 0;
        } 
        else 
        {
            return 1;
        }
    }

    bool is_range_free(x64::linaddr addr, size_t size) 
    {
        if (addr + size > total_size) 
        {
            return false;
        }
        return is_free;
    }

    void free_range(x64::linaddr addr, size_t size) 
    {
        if (addr + size > total_size) 
        {
            return;
        }
        is_free = true;
    }

    void mark_used(x64::linaddr addr_begin, x64::linaddr addr_end)
    {   
        if (addr_end >= total_size || addr_begin > addr_end) 
        {
            return;
        }
        is_free = false;
    }
};

template <short depth, short log2_page_size = 12>
class VirtualMemoryAllocator 
{
public:
    VirtualMemoryAllocator(phmem_manager *phmem) : phmem(phmem) {}
    x64::linaddr mmap(x64::linaddr addr, size_t size)
    {
        if(root.is_range_free(addr, size))
        {
            root.mark_used(addr, addr+size-1);
            return addr;
        }
        else
        {
            return root.get_range(size);
        }
    }
    void munmap(x64::linaddr addr, size_t size)
    {
        root.free_range(addr, size);
    }
private:
    BuddyAlloc<depth> root;
    phmem_manager *phmem;
};