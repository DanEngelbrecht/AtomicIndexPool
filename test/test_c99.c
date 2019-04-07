#include "../src/atomic_index_pool.h"

#include <stdlib.h>

static long AtomicAdd32(long volatile* value, long amount)
{
    *value += amount;
    return *value;
}

static long AtomicCAS32(long volatile* store, long compare, long value)
{
    long old_value = *store;
    if (old_value == compare)
    {
        *store = value;
    }
    return old_value;
}

static void TestC99()
{
    HAtomicIndexPool pool = AtomicIndexPool_Create(malloc(AtomicIndexPool_GetSize(15)), 15, AtomicAdd32, AtomicCAS32);

    uint32_t i = AtomicIndexPool_Pop(pool);
    AtomicIndexPool_Push(pool, i);

    free(pool);
}
