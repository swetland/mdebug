
#include <fw/io.h>
#include <fw/lib.h>
#include <arch/hardware.h>

const u32 gpio_led_red = GPIO_IDX(0, 25);
const u32 gpio_led_grn = GPIO_IDX(0, 3);
const u32 gpio_led_blu = GPIO_IDX(1, 1);

void board_debug_led(int on) {
	gpio_wr(gpio_led_grn, !on);	
}

void board_init(void) {
	/* enable GPIO blocks */
	writel(readl(SYSAHBCLKCTRL0) |
		AHBCC0_GPIO0 | AHBCC0_GPIO1 | AHBCC0_GPIO2,
		SYSAHBCLKCTRL0);

	/* disable LEDs */
	gpio_set(gpio_led_red);
	gpio_set(gpio_led_grn);
	gpio_set(gpio_led_blu);

	/* LED GPIOs to OUT */
	gpio_cfg_dir(gpio_led_red, GPIO_CFG_OUT);
	gpio_cfg_dir(gpio_led_grn, GPIO_CFG_OUT);
	gpio_cfg_dir(gpio_led_blu, GPIO_CFG_OUT);
}
