/*
 * Stress smp_call_function
 *
 * Copyright 2017-2020 Anton Blanchard, IBM Corporation <anton@linux.ibm.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#define pr_fmt(fmt) "ipistorm: " fmt

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

static unsigned long mask = 0;
module_param(mask, ulong, S_IRUGO);
MODULE_PARM_DESC(mask, "Mask for single destination IPI (default no mask)");

static unsigned long delay = 0;
module_param(delay, ulong, S_IRUGO);
MODULE_PARM_DESC(delay, "Delay between calls in us (default 0)");

static char cpulist_str[256];
module_param_string(cpulist, cpulist_str, sizeof(cpulist_str), 0644);
MODULE_PARM_DESC(cpulist, "List of CPUs (default all)");

static unsigned int num_cpus;
static atomic_t running;

static DECLARE_COMPLETION(ipistorm_done);

static void do_nothing_ipi(void *dummy)
{
}

static int ipistorm_thread(void *data)
{
	unsigned long mycpu = (unsigned long)data;
	unsigned long target_cpu;
	unsigned long jiffies_start;

	atomic_inc(&running);

	while (atomic_read(&running) < num_cpus) {
		if (need_resched())
			schedule();
	}

	if (mask) {
		unsigned long base = mycpu & ~mask;
		unsigned long off = (mycpu + offset) & mask;
		target_cpu = (base + off) % num_cpus;
	} else {
		target_cpu = (mycpu + offset) % num_cpus;
	}

	pr_info("%lu -> %lu\n", mycpu, target_cpu);

	if (single && !cpu_online(target_cpu)) {
		pr_err("CPU %lu not online\n", target_cpu);
		return 0;
	}

	jiffies_start = jiffies;

	while (jiffies < (jiffies_start + timeout*HZ)) {
		if (single)
			smp_call_function_single(target_cpu,
						 do_nothing_ipi, NULL, wait);
		else
			smp_call_function(do_nothing_ipi, NULL, wait);

		if (delay)
			usleep_range(delay, delay+1);
		else
			cond_resched();
	}

	if (atomic_dec_and_test(&running))
		complete(&ipistorm_done);

	return 0;
}

static int __init ipistorm_init(void)
{
	unsigned long cpu;
	cpumask_var_t mask;
	int ret = 0;

	init_completion(&ipistorm_done);

	if (!zalloc_cpumask_var(&mask, GFP_KERNEL))
		return -ENOMEM;

	if (cpulist_str[0]) {
		ret = cpulist_parse(cpulist_str, mask);
		if (ret)
			goto out_free;

		if (!cpumask_subset(mask, cpu_online_mask)) {
			pr_err("Invalid CPU list: %s\n", cpulist_str);
			ret = -EINVAL;
			goto out_free;
		}
	} else {
		cpumask_copy(mask, cpu_online_mask);
	}

	num_cpus = cpumask_weight(mask);

	for_each_cpu(cpu, mask) {
		struct task_struct *p;
		p = kthread_create(ipistorm_thread, (void *)cpu,
				   "ipistorm/%lu", cpu);
		if (IS_ERR(p)) {
			pr_err("kthread_create on CPU %lu failed\n", cpu);
			atomic_inc(&running);
		} else {
			kthread_bind(p, cpu);
			wake_up_process(p);
		}
	}

out_free:
	free_cpumask_var(mask);
	return ret;
}

static void __exit ipistorm_exit(void)
{
	wait_for_completion(&ipistorm_done);
}

module_init(ipistorm_init)
module_exit(ipistorm_exit)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anton Blanchard");
MODULE_DESCRIPTION("Stress smp_call_function");
