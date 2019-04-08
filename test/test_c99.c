#include "../src/atomic_index_pool.h"

#include <stdlib.h>

static void TestC99()
{
    HAtomicIndexPool pool = AtomicIndexPool_Create(malloc(AtomicIndexPool_GetSize(15)), 15);

    uint32_t i = AtomicIndexPool_Pop(pool);
    AtomicIndexPool_Push(pool, i);

    free(pool);
}
