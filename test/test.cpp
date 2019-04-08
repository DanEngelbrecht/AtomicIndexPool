#include "../src/atomic_index_pool.h"
#include "../third-party/nadir/src/nadir.h"

#include <stdlib.h>

#include "../third-party/jctest/src/jc_test.h"

TEST(Nadir, AtomicFilledIndexPool)
{
    HAtomicIndexPool pool = AtomicIndexPool_Create(malloc(AtomicIndexPool_GetSize(15)), 15);

    for(uint32_t i = 1; i <= 15; ++i)
    {
        ASSERT_EQ(i, AtomicIndexPool_Pop(pool));
    }

    ASSERT_EQ(0u, AtomicIndexPool_Pop(pool));

    for(uint32_t i = 15; i >= 1; --i)
    {
        AtomicIndexPool_Push(pool, i);
    }

    for(uint32_t i = 1; i <= 15; ++i)
    {
        ASSERT_EQ(i, AtomicIndexPool_Pop(pool));
    }

    free(pool);
}

TEST(Nadir, AtomicEmptyIndexPool)
{
    HAtomicIndexPool pool = AtomicIndexPool_Create(malloc(AtomicIndexPool_GetSize(16)), 0);

    AtomicIndexPool_Push(pool, 1);
    ASSERT_EQ(1u, AtomicIndexPool_Pop(pool));
    ASSERT_EQ(0u, AtomicIndexPool_Pop(pool));

    AtomicIndexPool_Push(pool, 1);
    AtomicIndexPool_Push(pool, 2);
    AtomicIndexPool_Push(pool, 3);
    ASSERT_EQ(3u, AtomicIndexPool_Pop(pool));
    ASSERT_EQ(2u, AtomicIndexPool_Pop(pool));
    AtomicIndexPool_Push(pool, 2);
    AtomicIndexPool_Push(pool, 3);
    ASSERT_EQ(3u, AtomicIndexPool_Pop(pool));
    ASSERT_EQ(2u, AtomicIndexPool_Pop(pool));
    ASSERT_EQ(1u, AtomicIndexPool_Pop(pool));
    ASSERT_EQ(0u, AtomicIndexPool_Pop(pool));

    free(pool);
}

TEST(Nadir, TestAtomicFiloThreads)
{
    #define ENTRY_BREAK_COUNT 311
    static const uint32_t ENTRY_COUNT = 391;

	for (uint32_t t = 0; t < 5; ++t)
	{
        HAtomicIndexPool pool = AtomicIndexPool_Create(malloc(AtomicIndexPool_GetSize(ENTRY_COUNT)), 0);
		struct Data
		{
			Data()
				: m_Busy(0)
				, m_Counter(0)
			{
			}
			nadir::TAtomic32 m_Busy;
			nadir::TAtomic32 m_Counter;
		};
		Data data_array[ENTRY_COUNT];
		nadir::TAtomic32 insert_count = 1;

		struct FiloThread
		{
			static int32_t Execute(void* context)
			{
				uint32_t fail_get_count = 0;
				FiloThread* t = (FiloThread*)context;
				while ((*t->m_InsertCount) > 0)
				{
					uint32_t index = AtomicIndexPool_Pop(t->m_Pool);
					assert(index <= t->m_EntryCount);
					if (index != 0)
					{
						fail_get_count = 0;
						long busy_counter = nadir::AtomicAdd32(&t->m_DataArray[index - 1].m_Busy, 1);
						assert(1 == busy_counter);
						int32_t new_value = nadir::AtomicAdd32(&t->m_DataArray[index - 1].m_Counter, 1);
						if (new_value < ENTRY_BREAK_COUNT)
						{
							busy_counter = nadir::AtomicAdd32(&t->m_DataArray[index - 1].m_Busy, -1);
							assert(0 == busy_counter);
							AtomicIndexPool_Push(t->m_Pool, index);
						}
						else
						{
							assert(new_value == ENTRY_BREAK_COUNT);
						}
					}
					else if (++fail_get_count > 50)
					{
						nadir::Sleep(1000);
						fail_get_count = 0;
					}
				}
				return 0;
			}
			uint32_t m_EntryCount;
			HAtomicIndexPool m_Pool;
			Data* m_DataArray;
			nadir::TAtomic32* m_InsertCount;
			nadir::HThread m_Thread;
		};

		static const uint32_t THREAD_COUNT = 128;
		FiloThread threads[THREAD_COUNT];
		for (uint32_t i = 0; i < THREAD_COUNT; ++i)
		{
			threads[i].m_EntryCount = ENTRY_COUNT;
			threads[i].m_Pool = pool;
			threads[i].m_DataArray = data_array;
			threads[i].m_InsertCount = &insert_count;
			threads[i].m_Thread = nadir::CreateThread(malloc(nadir::GetThreadSize()), FiloThread::Execute, 0, &threads[i]);
		}

		for (uint32_t i = 1; i <= ENTRY_COUNT; ++i)
		{
			AtomicIndexPool_Push(pool, i);
		}

		uint32_t untouched_count = 0;
		uint32_t touched_count = 0;
		for (uint32_t times = 0; times < (uint32_t)ENTRY_COUNT * 100u; ++times)
		{
			touched_count = 0;
			untouched_count = 0;
			for (uint32_t i = 0; i < ENTRY_COUNT; ++i)
			{
				if (data_array[i].m_Counter == ENTRY_BREAK_COUNT)
				{
					++touched_count;
				}
				else
				{
					++untouched_count;
				}
			}
			if (touched_count == ENTRY_COUNT)
			{
				nadir::AtomicAdd32(&insert_count, -1);
				break;
			}
			nadir::Sleep(1000);
		}
		ASSERT_EQ(touched_count, ENTRY_COUNT);
		ASSERT_EQ(untouched_count, 0u);

		for (uint32_t i = 0; i < THREAD_COUNT; ++i)
		{
			nadir::JoinThread(threads[i].m_Thread, nadir::TIMEOUT_INFINITE);
		}

		for (uint32_t i = 0; i < THREAD_COUNT; ++i)
		{
			nadir::DeleteThread(threads[i].m_Thread);
		}

		for (uint32_t i = 0; i < THREAD_COUNT; ++i)
		{
			free(threads[i].m_Thread);
		}

        free(pool);
	}
    #undef ENTRY_BREAK_COUNT
}
