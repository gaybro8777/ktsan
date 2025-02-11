/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_ATOMIC_H
#define _ASM_X86_ATOMIC_H

#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/ktsan.h>
#include <asm/alternative.h>
#include <asm/cmpxchg.h>
#include <asm/rmwcc.h>
#include <asm/barrier.h>

/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc..
 */

#define ATOMIC_INIT(i)	{ (i) }

/**
 * arch_atomic_read - read atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically reads the value of @v.
 */
static __always_inline int arch_atomic_read(const atomic_t *v)
{
#ifndef CONFIG_KTSAN
	/*
	 * Note for KASAN: we deliberately don't use READ_ONCE_NOCHECK() here,
	 * it's non-inlined function that increases binary size and stack usage.
	 */
	return READ_ONCE((v)->counter);
#else
	return ktsan_atomic32_load((void *)v, ktsan_memory_order_relaxed);
#endif
}

/**
 * arch_atomic_set - set atomic variable
 * @v: pointer of type atomic_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.
 */
static __always_inline void arch_atomic_set(atomic_t *v, int i)
{
#ifndef CONFIG_KTSAN
	WRITE_ONCE(v->counter, i);
#else
	ktsan_atomic32_store((void *)v, i, ktsan_memory_order_relaxed);
#endif
}

/**
 * arch_atomic_add - add integer to atomic variable
 * @i: integer value to add
 * @v: pointer of type atomic_t
 *
 * Atomically adds @i to @v.
 */
static __always_inline void arch_atomic_add(int i, atomic_t *v)
{
#ifndef CONFIG_KTSAN
	asm volatile(LOCK_PREFIX "addl %1,%0"
		     : "+m" (v->counter)
		     : "ir" (i) : "memory");
#else
	ktsan_atomic32_fetch_add((void *)v, i, ktsan_memory_order_relaxed);
#endif
}

/**
 * arch_atomic_sub - subtract integer from atomic variable
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 *
 * Atomically subtracts @i from @v.
 */
static __always_inline void arch_atomic_sub(int i, atomic_t *v)
{
#ifndef CONFIG_KTSAN
	asm volatile(LOCK_PREFIX "subl %1,%0"
		     : "+m" (v->counter)
		     : "ir" (i) : "memory");
#else
	ktsan_atomic32_fetch_add((void *)v, -i, ktsan_memory_order_relaxed);
#endif
}

/**
 * arch_atomic_sub_and_test - subtract value from variable and test result
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 *
 * Atomically subtracts @i from @v and returns
 * true if the result is zero, or false for all
 * other cases.
 */
static __always_inline bool arch_atomic_sub_and_test(int i, atomic_t *v)
{
#ifndef CONFIG_KTSAN
	return GEN_BINARY_RMWcc(LOCK_PREFIX "subl", v->counter, e, "er", i);
#else
	return (ktsan_atomic32_fetch_add((void *)v, -i,
			ktsan_memory_order_acq_rel) - i) == 0;
#endif
}
#define arch_atomic_sub_and_test arch_atomic_sub_and_test

/**
 * arch_atomic_inc - increment atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically increments @v by 1.
 */
static __always_inline void arch_atomic_inc(atomic_t *v)
{
#ifndef CONFIG_KTSAN
	asm volatile(LOCK_PREFIX "incl %0"
		     : "+m" (v->counter) :: "memory");
#else
	ktsan_atomic32_fetch_add((void *)v, 1, ktsan_memory_order_relaxed);
#endif
}
#define arch_atomic_inc arch_atomic_inc

/**
 * arch_atomic_dec - decrement atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically decrements @v by 1.
 */
static __always_inline void arch_atomic_dec(atomic_t *v)
{
#ifndef CONFIG_KTSAN
	asm volatile(LOCK_PREFIX "decl %0"
		     : "+m" (v->counter) :: "memory");
#else
	ktsan_atomic32_fetch_add((void *)v, -1, ktsan_memory_order_relaxed);
#endif
}
#define arch_atomic_dec arch_atomic_dec

/**
 * arch_atomic_dec_and_test - decrement and test
 * @v: pointer of type atomic_t
 *
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.
 */
static __always_inline bool arch_atomic_dec_and_test(atomic_t *v)
{
#ifndef CONFIG_KTSAN
	return GEN_UNARY_RMWcc(LOCK_PREFIX "decl", v->counter, e);
#else
	return (ktsan_atomic32_fetch_add((void *)v, -1,
			ktsan_memory_order_acq_rel) - 1) == 0;
#endif
}
#define arch_atomic_dec_and_test arch_atomic_dec_and_test

/**
 * arch_atomic_inc_and_test - increment and test
 * @v: pointer of type atomic_t
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */
static __always_inline bool arch_atomic_inc_and_test(atomic_t *v)
{
#ifndef CONFIG_KTSAN
	return GEN_UNARY_RMWcc(LOCK_PREFIX "incl", v->counter, e);
#else
	return (ktsan_atomic32_fetch_add((void *)v, 1,
			ktsan_memory_order_acq_rel) + 1) == 0;
#endif
}
#define arch_atomic_inc_and_test arch_atomic_inc_and_test

/**
 * arch_atomic_add_negative - add and test if negative
 * @i: integer value to add
 * @v: pointer of type atomic_t
 *
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.
 */
static __always_inline bool arch_atomic_add_negative(int i, atomic_t *v)
{
#ifndef CONFIG_KTSAN
	return GEN_BINARY_RMWcc(LOCK_PREFIX "addl", v->counter, s, "er", i);
#else
	return ((int)ktsan_atomic32_fetch_add((void *)v, i,
			ktsan_memory_order_acq_rel) + i) < 0;
#endif
}
#define arch_atomic_add_negative arch_atomic_add_negative

/**
 * arch_atomic_add_return - add integer and return
 * @i: integer value to add
 * @v: pointer of type atomic_t
 *
 * Atomically adds @i to @v and returns @i + @v
 */
static __always_inline int arch_atomic_add_return(int i, atomic_t *v)
{
#ifndef CONFIG_KTSAN
	return i + xadd(&v->counter, i);
#else
	return (ktsan_atomic32_fetch_add((void *)v, i,
			ktsan_memory_order_acq_rel) + i);
#endif
}

/**
 * arch_atomic_sub_return - subtract integer and return
 * @v: pointer of type atomic_t
 * @i: integer value to subtract
 *
 * Atomically subtracts @i from @v and returns @v - @i
 */
static __always_inline int arch_atomic_sub_return(int i, atomic_t *v)
{
#ifndef CONFIG_KTSAN
	return arch_atomic_add_return(-i, v);
#else
	return (ktsan_atomic32_fetch_add((void *)v, -i,
			ktsan_memory_order_acq_rel) - i);
#endif
}

static __always_inline int arch_atomic_fetch_add(int i, atomic_t *v)
{
	return xadd(&v->counter, i);
}

static __always_inline int arch_atomic_fetch_sub(int i, atomic_t *v)
{
	return xadd(&v->counter, -i);
}

static __always_inline int arch_atomic_cmpxchg(atomic_t *v, int old, int new)
{
#ifndef CONFIG_KTSAN
	return arch_cmpxchg(&v->counter, old, new);
#else
	return ktsan_atomic32_compare_exchange((void *)v, old, new,
			ktsan_memory_order_acq_rel);
#endif
}

#define arch_atomic_try_cmpxchg arch_atomic_try_cmpxchg
static __always_inline bool arch_atomic_try_cmpxchg(atomic_t *v, int *old, int new)
{
	return try_cmpxchg(&v->counter, old, new);
}

static inline int arch_atomic_xchg(atomic_t *v, int new)
{
#ifndef CONFIG_KTSAN
	return arch_xchg(&v->counter, new);
#else
	return ktsan_atomic32_exchange((void *)v, new,
			ktsan_memory_order_acq_rel);
#endif
}

static inline void arch_atomic_and(int i, atomic_t *v)
{
	asm volatile(LOCK_PREFIX "andl %1,%0"
			: "+m" (v->counter)
			: "ir" (i)
			: "memory");
}

static inline int arch_atomic_fetch_and(int i, atomic_t *v)
{
	int val = arch_atomic_read(v);

	do { } while (!arch_atomic_try_cmpxchg(v, &val, val & i));

	return val;
}

static inline void arch_atomic_or(int i, atomic_t *v)
{
	asm volatile(LOCK_PREFIX "orl %1,%0"
			: "+m" (v->counter)
			: "ir" (i)
			: "memory");
}

static inline int arch_atomic_fetch_or(int i, atomic_t *v)
{
	int val = arch_atomic_read(v);

	do { } while (!arch_atomic_try_cmpxchg(v, &val, val | i));

	return val;
}

static inline void arch_atomic_xor(int i, atomic_t *v)
{
	asm volatile(LOCK_PREFIX "xorl %1,%0"
			: "+m" (v->counter)
			: "ir" (i)
			: "memory");
}

static inline int arch_atomic_fetch_xor(int i, atomic_t *v)
{
	int val = arch_atomic_read(v);

	do { } while (!arch_atomic_try_cmpxchg(v, &val, val ^ i));

	return val;
}

#ifdef CONFIG_X86_32
# include <asm/atomic64_32.h>
#else
# include <asm/atomic64_64.h>
#endif

#include <asm-generic/atomic-instrumented.h>

#endif /* _ASM_X86_ATOMIC_H */
