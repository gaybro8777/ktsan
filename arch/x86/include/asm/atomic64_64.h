/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_ATOMIC64_64_H
#define _ASM_X86_ATOMIC64_64_H

#include <linux/types.h>
#include <linux/ktsan.h>
#include <asm/alternative.h>
#include <asm/cmpxchg.h>

/* The 64-bit atomic type */

#define ATOMIC64_INIT(i)	{ (i) }

/**
 * arch_atomic64_read - read atomic64 variable
 * @v: pointer of type atomic64_t
 *
 * Atomically reads the value of @v.
 * Doesn't imply a read memory barrier.
 */
static inline s64 arch_atomic64_read(const atomic64_t *v)
{
#ifndef CONFIG_KTSAN
	return READ_ONCE((v)->counter);
#else
	return ktsan_atomic64_load((void *)v, ktsan_memory_order_relaxed);
#endif
}

/**
 * arch_atomic64_set - set atomic64 variable
 * @v: pointer to type atomic64_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.
 */
static inline void arch_atomic64_set(atomic64_t *v, s64 i)
{
#ifndef CONFIG_KTSAN
	WRITE_ONCE(v->counter, i);
#else
	ktsan_atomic64_store((void *)v, i, ktsan_memory_order_relaxed);
#endif
}

/**
 * arch_atomic64_add - add integer to atomic64 variable
 * @i: integer value to add
 * @v: pointer to type atomic64_t
 *
 * Atomically adds @i to @v.
 */
static __always_inline void arch_atomic64_add(s64 i, atomic64_t *v)
{
#ifndef CONFIG_KTSAN
	asm volatile(LOCK_PREFIX "addq %1,%0"
		     : "=m" (v->counter)
		     : "er" (i), "m" (v->counter) : "memory");
#else
	ktsan_atomic64_fetch_add((void *)v, i, ktsan_memory_order_relaxed);
#endif
}

/**
 * arch_atomic64_sub - subtract the atomic64 variable
 * @i: integer value to subtract
 * @v: pointer to type atomic64_t
 *
 * Atomically subtracts @i from @v.
 */
static inline void arch_atomic64_sub(s64 i, atomic64_t *v)
{
#ifndef CONFIG_KTSAN
	asm volatile(LOCK_PREFIX "subq %1,%0"
		     : "=m" (v->counter)
		     : "er" (i), "m" (v->counter) : "memory");
#else
	ktsan_atomic64_fetch_add((void *)v, -i, ktsan_memory_order_relaxed);
#endif
}

/**
 * arch_atomic64_sub_and_test - subtract value from variable and test result
 * @i: integer value to subtract
 * @v: pointer to type atomic64_t
 *
 * Atomically subtracts @i from @v and returns
 * true if the result is zero, or false for all
 * other cases.
 */
static inline bool arch_atomic64_sub_and_test(s64 i, atomic64_t *v)
{
#ifndef CONFIG_KTSAN
	return GEN_BINARY_RMWcc(LOCK_PREFIX "subq", v->counter, e, "er", i);
#else
	return (ktsan_atomic64_fetch_add((void *)v, -i,
			ktsan_memory_order_acq_rel) - i) == 0;
#endif
}
#define arch_atomic64_sub_and_test arch_atomic64_sub_and_test

/**
 * arch_atomic64_inc - increment atomic64 variable
 * @v: pointer to type atomic64_t
 *
 * Atomically increments @v by 1.
 */
static __always_inline void arch_atomic64_inc(atomic64_t *v)
{
#ifndef CONFIG_KTSAN
	asm volatile(LOCK_PREFIX "incq %0"
		     : "=m" (v->counter)
		     : "m" (v->counter) : "memory");
#else
	ktsan_atomic64_fetch_add((void *)v, 1, ktsan_memory_order_relaxed);
#endif
}
#define arch_atomic64_inc arch_atomic64_inc

/**
 * arch_atomic64_dec - decrement atomic64 variable
 * @v: pointer to type atomic64_t
 *
 * Atomically decrements @v by 1.
 */
static __always_inline void arch_atomic64_dec(atomic64_t *v)
{
#ifndef CONFIG_KTSAN
	asm volatile(LOCK_PREFIX "decq %0"
		     : "=m" (v->counter)
		     : "m" (v->counter) : "memory");
#else
	ktsan_atomic64_fetch_add((void *)v, -1, ktsan_memory_order_relaxed);
#endif
}
#define arch_atomic64_dec arch_atomic64_dec

/**
 * arch_atomic64_dec_and_test - decrement and test
 * @v: pointer to type atomic64_t
 *
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.
 */
static inline bool arch_atomic64_dec_and_test(atomic64_t *v)
{
#ifndef CONFIG_KTSAN
	return GEN_UNARY_RMWcc(LOCK_PREFIX "decq", v->counter, e);
#else
	return (ktsan_atomic64_fetch_add((void *)v, -1,
			ktsan_memory_order_acq_rel) - 1) == 0;
#endif
}
#define arch_atomic64_dec_and_test arch_atomic64_dec_and_test

/**
 * arch_atomic64_inc_and_test - increment and test
 * @v: pointer to type atomic64_t
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */
static inline bool arch_atomic64_inc_and_test(atomic64_t *v)
{
#ifndef CONFIG_KTSAN
	return GEN_UNARY_RMWcc(LOCK_PREFIX "incq", v->counter, e);
#else
	return (ktsan_atomic64_fetch_add((void *)v, 1,
			ktsan_memory_order_acq_rel) + 1) == 0;
#endif
}
#define arch_atomic64_inc_and_test arch_atomic64_inc_and_test

/**
 * arch_atomic64_add_negative - add and test if negative
 * @i: integer value to add
 * @v: pointer to type atomic64_t
 *
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.
 */
static inline bool arch_atomic64_add_negative(s64 i, atomic64_t *v)
{
#ifndef CONFIG_KTSAN
	return GEN_BINARY_RMWcc(LOCK_PREFIX "addq", v->counter, s, "er", i);
#else
	return ((long)ktsan_atomic64_fetch_add((void *)v, i,
			ktsan_memory_order_acq_rel) + i) < 0;
#endif
}
#define arch_atomic64_add_negative arch_atomic64_add_negative

/**
 * arch_atomic64_add_return - add and return
 * @i: integer value to add
 * @v: pointer to type atomic64_t
 *
 * Atomically adds @i to @v and returns @i + @v
 */
static __always_inline s64 arch_atomic64_add_return(s64 i, atomic64_t *v)
{
#ifndef CONFIG_KTSAN
	return i + xadd(&v->counter, i);
#else
	return (ktsan_atomic64_fetch_add((void *)v, i,
			ktsan_memory_order_acq_rel) + i);
#endif
}

static inline s64 arch_atomic64_sub_return(s64 i, atomic64_t *v)
{
#ifndef CONFIG_KTSAN
	return arch_atomic64_add_return(-i, v);
#else
	return (ktsan_atomic64_fetch_add((void *)v, -i,
			ktsan_memory_order_acq_rel) - i);
#endif
}

static inline s64 arch_atomic64_fetch_add(s64 i, atomic64_t *v)
{
	return xadd(&v->counter, i);
}

static inline s64 arch_atomic64_fetch_sub(s64 i, atomic64_t *v)
{
	return xadd(&v->counter, -i);
}

static inline s64 arch_atomic64_cmpxchg(atomic64_t *v, s64 old, s64 new)
{
#ifndef CONFIG_KTSAN
	return arch_cmpxchg(&v->counter, old, new);
#else
	return ktsan_atomic64_compare_exchange((void *)v, old, new,
			ktsan_memory_order_acq_rel);
#endif
}

#define arch_atomic64_try_cmpxchg arch_atomic64_try_cmpxchg
static __always_inline bool arch_atomic64_try_cmpxchg(atomic64_t *v, s64 *old, s64 new)
{
	return try_cmpxchg(&v->counter, old, new);
}

static inline s64 arch_atomic64_xchg(atomic64_t *v, s64 new)
{
#ifndef CONFIG_KTSAN
	return arch_xchg(&v->counter, new);
#else
	return ktsan_atomic64_exchange((void *)v, new,
			ktsan_memory_order_acq_rel);
#endif
}

static inline void arch_atomic64_and(s64 i, atomic64_t *v)
{
	asm volatile(LOCK_PREFIX "andq %1,%0"
			: "+m" (v->counter)
			: "er" (i)
			: "memory");
}

static inline s64 arch_atomic64_fetch_and(s64 i, atomic64_t *v)
{
	s64 val = arch_atomic64_read(v);

	do {
	} while (!arch_atomic64_try_cmpxchg(v, &val, val & i));
	return val;
}

static inline void arch_atomic64_or(s64 i, atomic64_t *v)
{
	asm volatile(LOCK_PREFIX "orq %1,%0"
			: "+m" (v->counter)
			: "er" (i)
			: "memory");
}

static inline s64 arch_atomic64_fetch_or(s64 i, atomic64_t *v)
{
	s64 val = arch_atomic64_read(v);

	do {
	} while (!arch_atomic64_try_cmpxchg(v, &val, val | i));
	return val;
}

static inline void arch_atomic64_xor(s64 i, atomic64_t *v)
{
	asm volatile(LOCK_PREFIX "xorq %1,%0"
			: "+m" (v->counter)
			: "er" (i)
			: "memory");
}

static inline s64 arch_atomic64_fetch_xor(s64 i, atomic64_t *v)
{
	s64 val = arch_atomic64_read(v);

	do {
	} while (!arch_atomic64_try_cmpxchg(v, &val, val ^ i));
	return val;
}

#endif /* _ASM_X86_ATOMIC64_64_H */
