#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/sys_config.h>

static u32 pio_hdle = 0;
static int nr_gpio_cnt = 0;

script_item_u   *list = NULL;

void gpio_set_all()
{
	int i;
	for (i=0; i<nr_gpio_cnt; i++) {
		if (gpio_request(list[i].gpio.gpio, NULL)) {
			pr_err("request gpio%d failed\n", list[i].gpio.gpio);
		} else {
			pr_warning("set dir out, v=1%d\n", gpio_direction_output(list[i].gpio.gpio, 1));
			gpio_free(list[i].gpio.gpio);
		}
	}
}

void gpio_put_all()
{
	int i;
	for (i=0; i<nr_gpio_cnt; i++) {
		if (gpio_request(list[i].gpio.gpio, NULL)) {
			pr_err("request gpio%d failed\n", list[i].gpio.gpio);
		} else {
			pr_warning("set dir out, v=0%d\n", gpio_direction_output(list[i].gpio.gpio, 1));
			gpio_free(list[i].gpio.gpio);
		}
	}
}


static int __init gpio_test_init(void)
{
	pr_info("gpio test init\n");

	nr_gpio_cnt = script_get_pio_list("gpio_para",&list);
	printk("gpio count: %d\n", nr_gpio_cnt);

	if (!nr_gpio_cnt) {
		pr_err("err to get pio list\n");
		return -1;
	}

	gpio_set_all();

	return 0;
}
module_init(gpio_test_init);

static void __exit gpio_test_exit(void)
{
	pr_info("gpio_test exit\n");
	gpio_put_all();
}
module_exit(gpio_test_exit);

MODULE_LICENSE("GPL");

