# AtomicIndexPool
Single header library for an Atomic (lock-free) index pool

Minimal dependencies - only requires `<stdint.h>`

You provide two functions that are platform specific
 - `AtomicIndexPool_AtomicAdd` Atomically adds a 32-bit signed integer to another 32-bit signed integer and returns the result
 - `AtomicIndexPool_AtomicCAS` Atomically exchange a 32-bit signed integer with another 32-bit signed integer if the value to be swapped matches the provided compare value

 You are responsible for allocating and deallocating required memory, you can ask how much is needed.

 Limited to 8 388 607 entries in the index pool.
 
