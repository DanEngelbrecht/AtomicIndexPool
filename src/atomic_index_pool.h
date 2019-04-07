#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/////////////// PUBLIC API

typedef struct AtomicIndexPool_private* HAtomicIndexPool;

typedef long (*AtomicIndexPool_AtomicAdd)(long volatile* value, long amount);
typedef long (*AtomicIndexPool_AtomicCAS)(long volatile* store, long compare, long value);

// Get the memory size required for the index pool
inline size_t AtomicIndexPool_GetSize(uint32_t index_count);

// Create an index pool at memory location mem, mem must be at least AtomicIndexPool_GetSize() big
inline HAtomicIndexPool AtomicIndexPool_Create(void* mem, uint32_t fill_count, AtomicIndexPool_AtomicAdd atomic_add, AtomicIndexPool_AtomicCAS atomic_cas);

// Push a 1-based index to the pool
inline void AtomicIndexPool_Push(HAtomicIndexPool pool, uint32_t index);

// Pop a 1-based index from the pool, returns zero if the pool is empty
inline uint32_t AtomicIndexPool_Pop(HAtomicIndexPool pool);



/////////////// PRIVATE IMPLEMENTATION

static const uint32_t ATOMIC_INDEX_POOL_GENERATION_MASK_PRIVATE  = 0xff800000u;
static const uint32_t ATOMIC_INDEX_POOL_GENERATION_SHIFT_PRIVATE = 23u;
static const uint32_t ATOMIC_INDEX_POOL_INDEX_MASK_PRIVATE       = 0x007fffffu;

struct AtomicIndexPool_private
{
    long volatile m_Generation;
    AtomicIndexPool_AtomicAdd m_AtomicAdd;
    AtomicIndexPool_AtomicCAS m_AtomicCAS;
    long volatile m_Head[1];
};

inline size_t AtomicIndexPool_GetSize(uint32_t index_count)
{
    return sizeof(struct AtomicIndexPool_private) + sizeof(long volatile) * index_count;
}

inline HAtomicIndexPool AtomicIndexPool_Create(void* mem, uint32_t fill_count, AtomicIndexPool_AtomicAdd atomic_add, AtomicIndexPool_AtomicCAS atomic_cas)
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
    result->m_AtomicAdd = atomic_add;
    result->m_AtomicCAS = atomic_cas;
    return result;
}

inline void AtomicIndexPool_Push(HAtomicIndexPool pool, uint32_t index)
{
    uint32_t gen = (((uint32_t)pool->m_AtomicAdd(&pool->m_Generation, 1)) << ATOMIC_INDEX_POOL_GENERATION_SHIFT_PRIVATE) & ATOMIC_INDEX_POOL_GENERATION_MASK_PRIVATE;
    uint32_t new_head = gen | index;

    uint32_t current_head = (uint32_t)pool->m_Head[0];
    pool->m_Head[index] = (long)(current_head & ATOMIC_INDEX_POOL_INDEX_MASK_PRIVATE);

    while (pool->m_AtomicCAS(&pool->m_Head[0], (long)current_head, (long)new_head) != (long)current_head)
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

        if (pool->m_AtomicCAS(&pool->m_Head[0], (long)current_head, (long)new_head) == (long)current_head)
        {
            return head_index;
        }
    } while(1);
}

#ifdef __cplusplus
}
#endif
