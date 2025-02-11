// SPDX-License-Identifier: GPL-2.0
/* ThreadSanitizer (TSan) is a tool that finds data race bugs. */

#ifndef LINUX_KTSAN_H
#define LINUX_KTSAN_H

/* We can't include linux/types.h, since it includes linux/compiler.h,
   which includes linux/ktsan.h. Redeclare some types from linux/types.h. */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef _Bool bool;
typedef unsigned gfp_t;
struct page;

enum ktsan_memory_order_e {
	ktsan_memory_order_relaxed,
	ktsan_memory_order_acquire,
	ktsan_memory_order_release,
	ktsan_memory_order_acq_rel
};

typedef enum ktsan_memory_order_e ktsan_memory_order_t;

enum ktsan_glob_sync_type_e {
	ktsan_glob_sync_type_rcu_common,
	ktsan_glob_sync_type_rcu_bh,
	ktsan_glob_sync_type_rcu_sched,
	ktsan_glob_sync_type_count,
};

extern int ktsan_glob_sync[ktsan_glob_sync_type_count];

#ifdef CONFIG_KTSAN

struct kt_task_s;

struct ktsan_task_s {
	struct kt_task_s *task;
};

void ktsan_init_early(void);
void ktsan_init(void);

/* Debugging purposes only. */
void ktsan_print_diagnostics(void);

void ktsan_cpu_start(void);

void ktsan_task_create(struct ktsan_task_s *new, int pid);
void ktsan_task_destroy(struct ktsan_task_s *old);
void ktsan_task_start(void);
void ktsan_task_stop(void);

void ktsan_thr_event_disable(void);
void ktsan_thr_event_enable(void);
void ktsan_thr_report_disable(void);
void ktsan_thr_report_enable(void);

void ktsan_slab_alloc(void *addr, unsigned long size, unsigned long flags);
void ktsan_slab_free(void *addr, unsigned long size, unsigned long flags);

void ktsan_sync_acquire(void *addr);
void ktsan_sync_release(void *addr);

void ktsan_mtx_pre_lock(void *addr, bool write, bool try);
void ktsan_mtx_post_lock(void *addr, bool write, bool try, bool success);
void ktsan_mtx_pre_unlock(void *addr, bool write);
void ktsan_mtx_post_unlock(void *addr, bool write);
void ktsan_mtx_downgrade(void *addr);

/*
 * Begin/end of seqcount read critical section.
 * Disables/enabled handling of memory reads, because reads inside of seqcount
 * read critical section are inherently racy.
 */
void ktsan_seqcount_begin(const void *s);
void ktsan_seqcount_end(const void *s);
void ktsan_seqcount_ignore_begin(void);
void ktsan_seqcount_ignore_end(void);

void ktsan_thread_fence(ktsan_memory_order_t mo);

void ktsan_atomic8_store(void *addr, u8 value, ktsan_memory_order_t mo);
void ktsan_atomic16_store(void *addr, u16 value, ktsan_memory_order_t mo);
void ktsan_atomic32_store(void *addr, u32 value, ktsan_memory_order_t mo);
void ktsan_atomic64_store(void *addr, u64 value, ktsan_memory_order_t mo);

u8 ktsan_atomic8_load(const void *addr, ktsan_memory_order_t mo);
u16 ktsan_atomic16_load(const void *addr, ktsan_memory_order_t mo);
u32 ktsan_atomic32_load(const void *addr, ktsan_memory_order_t mo);
u64 ktsan_atomic64_load(const void *addr, ktsan_memory_order_t mo);

u8 ktsan_atomic8_exchange(void *addr, u8 value, ktsan_memory_order_t mo);
u16 ktsan_atomic16_exchange(void *addr, u16 value, ktsan_memory_order_t mo);
u32 ktsan_atomic32_exchange(void *addr, u32 value, ktsan_memory_order_t mo);
u64 ktsan_atomic64_exchange(void *addr, u64 value, ktsan_memory_order_t mo);

u8 ktsan_atomic8_compare_exchange(void *addr, u8 old, u8 new,
				  ktsan_memory_order_t mo);
u16 ktsan_atomic16_compare_exchange(void *addr, u16 old, u16 new,
				    ktsan_memory_order_t mo);
u32 ktsan_atomic32_compare_exchange(void *addr, u32 old, u32 new,
				    ktsan_memory_order_t mo);
u64 ktsan_atomic64_compare_exchange(void *addr, u64 old, u64 new,
				    ktsan_memory_order_t mo);

u8 ktsan_atomic8_fetch_add(void *addr, u8 value, ktsan_memory_order_t mo);
u16 ktsan_atomic16_fetch_add(void *addr, u16 value, ktsan_memory_order_t mo);
u32 ktsan_atomic32_fetch_add(void *addr, u32 value, ktsan_memory_order_t mo);
u64 ktsan_atomic64_fetch_add(void *addr, u64 value, ktsan_memory_order_t mo);

void ktsan_atomic_set_bit(void *addr, long nr, ktsan_memory_order_t mo);
void ktsan_atomic_clear_bit(void *addr, long nr, ktsan_memory_order_t mo);
void ktsan_atomic_change_bit(void *addr, long nr, ktsan_memory_order_t mo);

int ktsan_atomic_fetch_set_bit(void *addr, long nr, ktsan_memory_order_t mo);
int ktsan_atomic_fetch_clear_bit(void *addr, long nr, ktsan_memory_order_t mo);
int ktsan_atomic_fetch_change_bit(void *addr, long nr, ktsan_memory_order_t mo);

void ktsan_preempt_add(int value);
void ktsan_preempt_sub(int value);

void ktsan_irq_disable(void);
void ktsan_irq_enable(void);
void ktsan_irq_save(void);
void ktsan_irq_restore(unsigned long flags);

void ktsan_percpu_acquire(void *addr);

void ktsan_alloc_page(struct page *page, unsigned int order, gfp_t flags,
		      int node);
void ktsan_free_page(struct page *page, unsigned int order);
void ktsan_split_page(struct page *page, unsigned int order);

void ktsan_syscall_enter(void);
void ktsan_syscall_exit(void);

#else /* CONFIG_KTSAN */

/* When disabled ktsan is no-op. */

struct ktsan_task_s {
};

static inline void ktsan_init_early(void)
{
}
static inline void ktsan_init(void)
{
}

static inline void ktsan_print_diagnostics(void)
{
}

static inline void ktsan_cpu_start(void)
{
}

static inline void ktsan_task_create(struct ktsan_task_s *new, int pid)
{
}
static inline void ktsan_task_destroy(struct ktsan_task_s *old)
{
}
static inline void ktsan_task_start(void)
{
}
static inline void ktsan_task_stop(void)
{
}

static inline void ktsan_thr_event_disable(void)
{
}
static inline void ktsan_thr_event_enable(void)
{
}
static inline void ktsan_thr_report_disable(void)
{
}
static inline void ktsan_thr_report_enable(void)
{
}

static inline void ktsan_memblock_alloc(void *addr, unsigned long size)
{
}
static inline void ktsan_memblock_free(void *addr, unsigned long size)
{
}

static inline void ktsan_sync_acquire(void *addr)
{
}
static inline void ktsan_sync_release(void *addr)
{
}

static inline void ktsan_mtx_pre_lock(void *addr, bool write, bool try)
{
}
static inline void ktsan_mtx_post_lock(void *addr, bool write, bool try,
				       bool success)
{
}
static inline void ktsan_mtx_pre_unlock(void *addr, bool write)
{
}
static inline void ktsan_mtx_post_unlock(void *addr, bool write)
{
}
static inline void ktsan_mtx_downgrade(void *addr)
{
}

static inline void ktsan_seqcount_begin(const void *s)
{
}
static inline void ktsan_seqcount_end(const void *s)
{
}
static inline void ktsan_seqcount_ignore_begin(void)
{
}
static inline void ktsan_seqcount_ignore_end(void)
{
}

/* ktsan_atomic* are not called in non-ktsan build. */
/* ktsan_bitop* are not called in non-ktsan build. */

static inline void ktsan_preempt_add(int value)
{
}
static inline void ktsan_preempt_sub(int value)
{
}

static inline void ktsan_irq_disable(void)
{
}
static inline void ktsan_irq_enable(void)
{
}
static inline void ktsan_irq_save(void)
{
}
static inline void ktsan_irq_restore(unsigned long flags)
{
}

static inline void ktsan_percpu_acquire(void *addr)
{
}

static inline void ktsan_alloc_page(struct page *page, unsigned int order,
				    gfp_t flags, int node)
{
}
static inline void ktsan_free_page(struct page *page, unsigned int order)
{
}
static inline void ktsan_split_page(struct page *page, unsigned int order)
{
}

static inline void ktsan_syscall_enter(void)
{
}
static inline void ktsan_syscall_exit(void)
{
}

#endif /* CONFIG_KTSAN */

#endif /* LINUX_KTSAN_H */
