
#include <fw/types.h>
#include <fw/lib.h>
#include <arch/hardware.h>

const u32 gpio_led0 = MKGPIO(0, 2);
const u32 gpio_led1 = MKGPIO(2, 7);
const u32 gpio_led2 = MKGPIO(2, 8);
const u32 gpio_led3 = MKGPIO(2, 1);

const u32 gpio_reset_n = MKGPIO(2, 10);

const u8 board_name[] = "M3DEBUG";

void board_init(void) {
	core_48mhz_init();
	gpio_cfg_dir(gpio_led0, GPIO_CFG_OUT);
	gpio_cfg_dir(gpio_led1, GPIO_CFG_OUT);
	gpio_cfg_dir(gpio_led2, GPIO_CFG_OUT);
	gpio_cfg_dir(gpio_led3, GPIO_CFG_OUT);
}

void board_debug_led(int on) {
	gpio_wr(gpio_led0, on);
	gpio_wr(gpio_led1, on);
	gpio_wr(gpio_led2, on);
	gpio_wr(gpio_led3, on);
}

