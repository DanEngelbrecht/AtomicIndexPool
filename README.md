|Branch      | OSX / Linux / Windows |
|------------|---------|
|master      | [![Build Status](https://travis-ci.org/DanEngelbrecht/AtomicIndexPool.svg?branch=master)](https://travis-ci.org/DanEngelbrecht/AtomicIndexPool?branch=master) |


# AtomicIndexPool
Single header library for an Atomic (lock-free) index pool

Minimal dependencies - only requires `<stdint.h>` and `<stddef.h>`, the default (optional) MSVC implementation depends on Windows.h.

The default implementations for the atomic functions can be overridden with your own implementation by overriding the macros
 - `ATOMICINDEXPOOL_ATOMICADD` Atomically adds a 32-bit signed integer to another 32-bit signed integer and returns the result
 - `ATOMICINDEXPOOL_ATOMICADD` Atomically exchange a 32-bit signed integer with another 32-bit signed integer if the value to be swapped matches the provided compare value, returns the old value.

You are responsible for allocating and deallocating required memory, you can ask how much is needed.

Limited to 8 388 607 entries in the index pool.

The pool indexes are starts at 1, so to use the result in C arrays, use index - 1.
