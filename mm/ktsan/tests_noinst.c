// SPDX-License-Identifier: GPL-2.0
#include "ktsan.h"

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/slab.h>

/* Hash table test. */

void kt_test_hash_table(void)
{
	kt_ctx_t *ctx;
	kt_tab_test_t *obj, *obj1, *obj2, *obj3;
	bool created;

	pr_err("ktsan: starting hash table test.\n");

	/* The test table is initialized in ktsan_init_early. */

	ctx = &kt_ctx;

	obj = kt_tab_access(&ctx->test_tab, 10, NULL, false);
	BUG_ON(obj != NULL);

	/* Creating. */

	obj = kt_tab_access(&ctx->test_tab, 7, &created, false);
	BUG_ON(obj == NULL);
	BUG_ON(created != true);
	BUG_ON(!kt_spin_is_locked(&obj->tab.lock));
	kt_spin_unlock(&obj->tab.lock);

	obj1 = kt_tab_access(&ctx->test_tab, 7, &created, false);
	BUG_ON(obj1 != obj);
	BUG_ON(created != false);
	BUG_ON(!kt_spin_is_locked(&obj1->tab.lock));
	kt_spin_unlock(&obj1->tab.lock);

	obj2 = kt_tab_access(&ctx->test_tab, 7 + 13, &created, false);
	BUG_ON(obj2 == NULL);
	BUG_ON(obj2 == obj1);
	BUG_ON(created != true);
	BUG_ON(!kt_spin_is_locked(&obj2->tab.lock));
	kt_spin_unlock(&obj2->tab.lock);

	obj3 = kt_tab_access(&ctx->test_tab, 7 + 13, NULL, false);
	BUG_ON(obj3 != obj2);
	BUG_ON(!kt_spin_is_locked(&obj3->tab.lock));
	kt_spin_unlock(&obj3->tab.lock);

	obj3 = kt_tab_access(&ctx->test_tab, 3, &created, false);
	BUG_ON(obj3 == NULL);
	BUG_ON(obj3 == obj1 || obj3 == obj2);
	BUG_ON(created != true);
	BUG_ON(!kt_spin_is_locked(&obj3->tab.lock));
	kt_spin_unlock(&obj3->tab.lock);

	/* Accessing. */

	obj = kt_tab_access(&ctx->test_tab, 7, NULL, false);
	BUG_ON(obj == NULL);
	BUG_ON(obj != obj1);
	BUG_ON(!kt_spin_is_locked(&obj->tab.lock));
	kt_spin_unlock(&obj->tab.lock);

	obj = kt_tab_access(&ctx->test_tab, 7 + 13, &created, false);
	BUG_ON(obj == NULL);
	BUG_ON(obj != obj2);
	BUG_ON(created != false);
	BUG_ON(!kt_spin_is_locked(&obj->tab.lock));
	kt_spin_unlock(&obj->tab.lock);

	obj = kt_tab_access(&ctx->test_tab, 3, NULL, false);
	BUG_ON(obj == NULL);
	BUG_ON(obj != obj3);
	BUG_ON(!kt_spin_is_locked(&obj->tab.lock));
	kt_spin_unlock(&obj->tab.lock);

	obj = kt_tab_access(&ctx->test_tab, 4, NULL, false);
	BUG_ON(obj != NULL);

	/* Destroying. */

	obj = kt_tab_access(&ctx->test_tab, 3, NULL, true);
	BUG_ON(obj == NULL);
	BUG_ON(obj != obj3);
	BUG_ON(!kt_spin_is_locked(&obj3->tab.lock));
	kt_spin_unlock(&obj3->tab.lock);
	kt_cache_free(&ctx->test_tab.obj_cache, obj3);

	obj = kt_tab_access(&ctx->test_tab, 7 + 13, NULL, true);
	BUG_ON(obj == NULL);
	BUG_ON(obj != obj2);
	BUG_ON(!kt_spin_is_locked(&obj2->tab.lock));
	kt_spin_unlock(&obj2->tab.lock);
	kt_cache_free(&ctx->test_tab.obj_cache, obj2);

	obj = kt_tab_access(&ctx->test_tab, 7, NULL, true);
	BUG_ON(obj == NULL);
	BUG_ON(obj != obj1);
	BUG_ON(!kt_spin_is_locked(&obj1->tab.lock));
	kt_spin_unlock(&obj1->tab.lock);
	kt_cache_free(&ctx->test_tab.obj_cache, obj1);

	pr_err("ktsan: end of test.\n");
}

/* Trace test. */

void kt_test_trace(void)
{
	kt_thr_t *thr;
	kt_time_t clock;
	kt_trace_state_t state;
	int *fake;

	pr_err("ktsan: starting trace test.\n");

	thr = current->ktsan.task->thr;
	clock = kt_clk_get(&thr->clk, thr->id);

	/* Fake mop event. */
	fake = kmalloc(sizeof(*fake), GFP_KERNEL);
	kt_access(thr, (uptr_t)_RET_IP_, (uptr_t)fake, 1, false, false);
	kfree(fake);

	kt_trace_restore_state(thr, clock, &state);

	pr_err("Restored stack trace:\n");
	kt_stack_print(&state.stack, 0);

	pr_err("ktsan: end of test.\n");
}

/* Not instrumented tests, should be called inside ENTER/LEAVE section. */

void kt_tests_run_noinst(void)
{
	pr_err("ktsan: running not instrumented tests, T%d.\n", current->pid);
	pr_err("\n");

	kt_test_hash_table();
	pr_err("\n");
	kt_test_trace();
	pr_err("\n");
}
