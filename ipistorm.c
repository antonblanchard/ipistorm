/*
 * Stress smp_call_function
 *
 * Copyright 2017 Anton Blanchard, IBM Corporation <anton@au1.ibm.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/delay.h>

static long timeout = 30;
module_param(timeout, long, S_IRUGO);
MODULE_PARM_DESC(timeout, "Timeout in seconds (default = 30)");

static bool wait = true;
module_param(wait, bool, S_IRUGO);
MODULE_PARM_DESC(wait, "Wait for IPI to finish? (default true)");

static bool single = false;
module_param(single, bool, S_IRUGO);
MODULE_PARM_DESC(single, "IPI to a single destination? (default false, IPI to all)");

static unsigned long offset = 1;
module_param(offset, ulong, S_IRUGO);
MODULE_PARM_DESC(offset, "Offset from current CPU for single destination IPI (default 1)");

static unsigned long delay = 0;
module_param(delay, ulong, S_IRUGO);
MODULE_PARM_DESC(delay, "Delay between calls in us (default 0)");

static unsigned int num_cpus;
static atomic_t running;

static void do_nothing_ipi(void *dummy)
{
}

static int ipistorm_thread(void *data)
{
	unsigned long mycpu = data;
	unsigned long jiffies_start;

	atomic_inc(&running);

	while (atomic_read(&running) < num_cpus) {
		if (need_resched())
			schedule();
	}

	jiffies_start = jiffies;

	while (jiffies < (jiffies_start + timeout*HZ)) {
		if (single)
			smp_call_function_single((mycpu + offset) % num_cpus,
						 do_nothing_ipi, NULL, wait);
		else
			smp_call_function(do_nothing_ipi, NULL, wait);

		usleep_range(delay, delay+1);
	}

	return 0;
}

static int __init ipistorm_init(void)
{
	unsigned int cpu;

	num_cpus = num_online_cpus();

	for_each_online_cpu(cpu) {
		struct task_struct *p;
		p = kthread_create(ipistorm_thread, NULL,
				   "ipistorm/%u", cpu);
		if (IS_ERR(p)) {
			pr_err("kthread_create on CPU %d failed\n", cpu);
			atomic_inc(&running);
		} else {
			kthread_bind(p, cpu);
			wake_up_process(p);
		}
	}

	return 0;
}

static void __exit ipistorm_exit(void)
{
}

module_init(ipistorm_init)
module_exit(ipistorm_exit)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anton Blanchard");
MODULE_DESCRIPTION("Stress smp_call_function");
