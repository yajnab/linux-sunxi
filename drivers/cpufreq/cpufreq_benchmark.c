/*
 *  linux/drivers/cpufreq/cpufreq_benchmark.c
 *
 *  Copyright (C) 2013 James Deng <csjamesdeng@allwinnertech.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <asm/uaccess.h>

#include <mach/clock.h>


#define MBUS_TARGET_VALID

#define CPU_EXTREMITY_FREQ              1008000     /* cpu extremity frequency: 1008M */
#define SYS_VDD_EXTREMITY               1300000     /* sys vdd: 1.3V                  */


#ifdef MBUS_TARGET_VALID
static struct regulator *sys_vdd_regulator = NULL;
static struct clk *mbus_clk = NULL;
static int old_sys_vdd = 0;
#endif

static unsigned int max_cpu_freq_bak = 912000;
static struct work_struct cpu_up_queue;

#ifdef MBUS_TARGET_VALID
static int __mbus_target_400M(void)
{
    int ret = 0;

    if (sys_vdd_regulator) {
        old_sys_vdd = regulator_get_voltage(sys_vdd_regulator);
        if (old_sys_vdd <= 0) {
            pr_err("get sys_vdd 1 failed: %d\n", old_sys_vdd);
            goto failed;
        }

        if ((ret = regulator_set_voltage(sys_vdd_regulator,
                    SYS_VDD_EXTREMITY, SYS_VDD_EXTREMITY)) < 0) {
            pr_err("set sys_vdd 1 failed: %d\n", ret);
            goto failed;
        }
    }

    if (mbus_clk) {
        if (!clk_set_rate(mbus_clk, 200000000) &&
            !clk_set_rate(mbus_clk, 400000000)) {
            return 0;
        } else {
            pr_err("set mbus clock 1 failed\n");
        }
    }

failed:
    return -1;
}

static int __mbus_target_300M(void)
{
    int ret = 0;

    if (mbus_clk) {
        if (!clk_set_rate(mbus_clk, 200000000) &&
            !clk_set_rate(mbus_clk, 300000000)) {
            /* do nothing */
        } else {
            pr_err("set mbus clock 0 failed\n");
        }
    }

    if (sys_vdd_regulator && old_sys_vdd > 0) {
        if ((ret = regulator_set_voltage(sys_vdd_regulator,
                        old_sys_vdd, old_sys_vdd)) < 0) {
            pr_err("set sys_vdd 0 failed: %d\n", ret);
            return -1;
        }
    }

    return 0;
}
#endif

extern int cpu_up(unsigned int cpu);
static void cpu_up_work(struct work_struct *work)
{
    cpu_up(1);
}

static int cpufreq_governor_benchmark(struct cpufreq_policy *policy,
					unsigned int event)
{
    static int cpu1_state = 1;
    static int mbus_state = 0;

	switch (event) {
	case CPUFREQ_GOV_START:
        /* set cpu frequency to extremity */
        max_cpu_freq_bak = policy->max;
        policy->max = CPU_EXTREMITY_FREQ;
		__cpufreq_driver_target(policy, CPU_EXTREMITY_FREQ,
                CPUFREQ_RELATION_H);

#ifdef MBUS_TARGET_VALID
        /* set mbus clock to 400M */
        mbus_state = __mbus_target_400M();
#endif

        /* up cpu1 */
        cpu1_state = cpu_online(1);
        if (cpu1_state) {
            /* do nothing */
        } else {
            schedule_work(&cpu_up_queue);
        }

		break;
    case CPUFREQ_GOV_STOP:
#ifdef MBUS_TARGET_VALID
        /* set mbus clock to 300M */
        if (mbus_state) {
            /* do nothing */
        } else {
            __mbus_target_300M();
        }
#endif

        /* set cpu frequency to highest */
        policy->max = max_cpu_freq_bak;
		__cpufreq_driver_target(policy, policy->max,
                CPUFREQ_RELATION_H);

        break;
	default:
		break;
	}
	return 0;
}

#ifdef CONFIG_CPU_FREQ_GOV_BENCHMARK_MODULE
static
#endif
struct cpufreq_governor cpufreq_gov_benchmark = {
	.name		= "benchmark",
	.governor	= cpufreq_governor_benchmark,
	.owner		= THIS_MODULE,
};


static int __init cpufreq_gov_benchmark_init(void)
{
#ifdef MBUS_TARGET_VALID
    sys_vdd_regulator = regulator_get(NULL, "axp20_ddr");
    if (IS_ERR(sys_vdd_regulator)) {
        pr_err("get axp20_ddr regulator failed\n");
        sys_vdd_regulator = NULL;
        goto init_queue;
    }

    /* mbus clock source is pll6x2 */
    mbus_clk = clk_get(NULL, CLK_MOD_MBUS);
    if (IS_ERR(mbus_clk)) {
        pr_err("get mbus clock failed\n");
        mbus_clk = NULL;
    }
#endif

init_queue:
    INIT_WORK(&cpu_up_queue, cpu_up_work);

	return cpufreq_register_governor(&cpufreq_gov_benchmark);
}


static void __exit cpufreq_gov_benchmark_exit(void)
{
#ifdef MBUS_TARGET_VALID
    if (mbus_clk) {
        clk_put(mbus_clk);
        mbus_clk = NULL;
    }

    if (sys_vdd_regulator) {
        regulator_put(sys_vdd_regulator);
        sys_vdd_regulator = NULL;
    }
#endif
	cpufreq_unregister_governor(&cpufreq_gov_benchmark);
}


MODULE_AUTHOR("James Deng <csjamesdeng@allwinnertech.com>");
MODULE_DESCRIPTION("CPUfreq policy governor 'benchmark'");
MODULE_LICENSE("GPL");

fs_initcall(cpufreq_gov_benchmark_init);
module_exit(cpufreq_gov_benchmark_exit);
