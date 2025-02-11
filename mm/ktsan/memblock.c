// SPDX-License-Identifier: GPL-2.0
#include "ktsan.h"

#include <linux/list.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/slab_def.h>
#include <linux/spinlock.h>

uptr_t kt_memblock_addr(uptr_t addr)
{
	struct page *page;
	struct kmem_cache *cache;
	u32 offset;
	u32 idx;
	uptr_t obj_addr;

	if (!virt_addr_valid(addr))
		return 0;
	page = virt_to_head_page((void *)addr);

	/* If the page is a slab page, we want to delete
	   sync objects when a slab object is freed. */
	if (PageSlab(page)) {
		cache = page->slab_cache;
		BUG_ON(addr < (uptr_t)page->s_mem);
		offset = addr - (uptr_t)page->s_mem;
		idx = reciprocal_divide(offset, cache->reciprocal_buffer_size);
		obj_addr = (uptr_t)(page->s_mem + cache->size * idx);
		return obj_addr;
	}

	return (uptr_t)page_address(page);
}

static kt_tab_memblock_t *kt_memblock_ensure_created(kt_thr_t *thr, uptr_t addr)
{
	kt_tab_memblock_t *memblock;
	bool created;

	memblock = kt_tab_access(&kt_ctx.memblock_tab, addr, &created, false);
	BUG_ON(memblock == NULL); /* Ran out of memory. */

	if (created) {
		INIT_LIST_HEAD(&memblock->sync_list);
		INIT_LIST_HEAD(&memblock->lock_list);

		kt_stat_inc(kt_stat_memblock_objects);
		kt_stat_inc(kt_stat_memblock_alloc);
	}

	return memblock;
}

void kt_memblock_add_sync(kt_thr_t *thr, uptr_t addr, kt_tab_sync_t *sync)
{
	kt_tab_memblock_t *memblock;

	memblock = kt_memblock_ensure_created(thr, addr);
	list_add(&sync->list, &memblock->sync_list);
	kt_spin_unlock(&memblock->tab.lock);
}

void kt_memblock_remove_sync(kt_thr_t *thr, uptr_t addr, kt_tab_sync_t *sync)
{
	kt_tab_memblock_t *memblock;
	struct list_head *entry, *tmp;
	bool deleted = false;

	memblock = kt_tab_access(&kt_ctx.memblock_tab, addr, NULL, false);
	BUG_ON(memblock == NULL);

	list_for_each_safe (entry, tmp, &memblock->sync_list) {
		if (list_entry(entry, kt_tab_sync_t, list) == sync) {
			list_del_init(entry);
			deleted = true;
			break;
		}
	}

	BUG_ON(!deleted);
	kt_spin_unlock(&memblock->tab.lock);
}

void kt_memblock_alloc(kt_thr_t *thr, uptr_t pc, uptr_t addr, size_t size,
		       bool write_to_shadow)
{
	/* Memory block size is multiple of KT_GRAIN, so round the size up.
	 * In the "worst" case we will catch OOB accesses due to this. */
	size = round_up(size, KT_GRAIN);

	if (write_to_shadow)
		kt_access_range_imitate(thr, pc, addr, size, false);
}

void kt_memblock_free(kt_thr_t *thr, uptr_t pc, uptr_t addr, size_t size,
		      bool write_to_shadow)
{
	kt_tab_memblock_t *memblock;
	struct list_head *entry, *tmp;
	kt_tab_sync_t *sync;

	if (write_to_shadow)
		kt_access_range(thr, pc, addr, size, false);

	memblock = kt_tab_access(&kt_ctx.memblock_tab, addr, NULL, true);

	if (memblock == NULL)
		return;

	list_for_each_safe (entry, tmp, &memblock->sync_list) {
		sync = list_entry(entry, kt_tab_sync_t, list);
		list_del_init(entry);
		kt_sync_free(thr, sync->tab.key);
	}

	kt_spin_unlock(&memblock->tab.lock);
	kt_cache_free(&kt_ctx.memblock_tab.obj_cache, memblock);

	kt_stat_dec(kt_stat_memblock_objects);
	kt_stat_inc(kt_stat_memblock_free);
}
