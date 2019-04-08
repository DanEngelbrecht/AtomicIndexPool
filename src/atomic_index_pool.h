#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/////////////// PUBLIC API

typedef struct AtomicIndexPool_private* HAtomicIndexPool;

// Get the memory size required for the index pool
inline size_t AtomicIndexPool_GetSize(uint32_t index_count);

// Create an index pool at memory location mem, mem must be at least AtomicIndexPool_GetSize() big
inline HAtomicIndexPool AtomicIndexPool_Create(void* mem, uint32_t fill_count);

// Push a 1-based index to the pool
inline void AtomicIndexPool_Push(HAtomicIndexPool pool, uint32_t index);

// Pop a 1-based index from the pool, returns zero if the pool is empty
inline uint32_t AtomicIndexPool_Pop(HAtomicIndexPool pool);



/////////////// PRIVATE IMPLEMENTATION

#if !defined(ATOMICINDEXPOOL_ATOMICADD)
    #if defined(_MSC_VER)
        #if !defined(_WINDOWS_)
            #define WIN32_LEAN_AND_MEAN
            #include <Windows.h>
            #undef WIN32_LEAN_AND_MEAN
        #endif

        #define ATOMICINDEXPOOL_ATOMICADD(value, amount) (_InterlockedExchangeAdd(value, amount) + amount)
    #endif
    #if defined(__clang__) || defined(__GNUC__)
        #define ATOMICINDEXPOOL_ATOMICADD(value, amount) (__sync_fetch_and_add(value, amount) + amount)
    #endif
#endif

#if !defined(ATOMICINDEXPOOL_ATOMICCAS)
    #if defined(_MSC_VER)
        #if !defined(_WINDOWS_)
            #define WIN32_LEAN_AND_MEAN
            #include <Windows.h>
            #undef WIN32_LEAN_AND_MEAN
        #endif

        #define ATOMICINDEXPOOL_ATOMICCAS(store, compare, value) _InterlockedCompareExchange(store, value, compare)
    #endif
    #if defined(__clang__) || defined(__GNUC__)
        #define ATOMICINDEXPOOL_ATOMICCAS(store, compare, value) __sync_val_compare_and_swap(store, compare, value)
    #endif
#endif

static const uint32_t ATOMIC_INDEX_POOL_GENERATION_MASK_PRIVATE  = 0xff800000u;
static const uint32_t ATOMIC_INDEX_POOL_GENERATION_SHIFT_PRIVATE = 23u;
static const uint32_t ATOMIC_INDEX_POOL_INDEX_MASK_PRIVATE       = 0x007fffffu;

struct AtomicIndexPool_private
{
    long volatile m_Generation;
    long volatile m_Head[1];
};

inline size_t AtomicIndexPool_GetSize(uint32_t index_count)
{
    return sizeof(struct AtomicIndexPool_private) + sizeof(long volatile) * index_count;
}

inline HAtomicIndexPool AtomicIndexPool_Create(void* mem, uint32_t fill_count)
{
    HAtomicIndexPool result = (HAtomicIndexPool)mem;
    result->m_Generation = 0;
    if (fill_count > 0)
    {
        result->m_Head[0] = 1;
        for (uint32_t i = 1; i < fill_count; ++i)
        {
            result->m_Head[i] = i + 1;
        }
        result->m_Head[fill_count] = 0;
    }
    else
    {
        result->m_Head[0] = 0;
    }
    return result;
}

inline void AtomicIndexPool_Push(HAtomicIndexPool pool, uint32_t index)
{
    uint32_t gen = (((uint32_t)ATOMICINDEXPOOL_ATOMICADD(&pool->m_Generation, 1)) << ATOMIC_INDEX_POOL_GENERATION_SHIFT_PRIVATE) & ATOMIC_INDEX_POOL_GENERATION_MASK_PRIVATE;
    uint32_t new_head = gen | index;

    uint32_t current_head = (uint32_t)pool->m_Head[0];
    pool->m_Head[index] = (long)(current_head & ATOMIC_INDEX_POOL_INDEX_MASK_PRIVATE);

    while (ATOMICINDEXPOOL_ATOMICCAS(&pool->m_Head[0], (long)current_head, (long)new_head) != (long)current_head)
    {
        current_head = (uint32_t)pool->m_Head[0];
        pool->m_Head[index] = (long)(current_head & ATOMIC_INDEX_POOL_INDEX_MASK_PRIVATE);
    }
}

inline uint32_t AtomicIndexPool_Pop(HAtomicIndexPool pool)
{
    do
    {
        uint32_t current_head = (uint32_t)pool->m_Head[0];
        uint32_t head_index = current_head & ATOMIC_INDEX_POOL_INDEX_MASK_PRIVATE;
        if (head_index == 0)
        {
            return 0;
        }

        uint32_t next = (uint32_t)pool->m_Head[head_index];
		uint32_t new_head = (current_head & ATOMIC_INDEX_POOL_GENERATION_MASK_PRIVATE) | next;

        if (ATOMICINDEXPOOL_ATOMICCAS(&pool->m_Head[0], (long)current_head, (long)new_head) == (long)current_head)
        {
            return head_index;
        }
    } while(1);
}

#ifdef __cplusplus
}
#endif // __cplusplus
