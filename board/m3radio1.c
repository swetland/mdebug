
#include <fw/types.h>
#include <arch/hardware.h>

const u32 gpio_led0 = MKGPIO(2, 4);
const u32 gpio_led1 = MKGPIO(2, 5);
const u32 gpio_led2 = MKGPIO(2, 9);
const u32 gpio_led3 = MKGPIO(2, 7);

const u8 board_name[] = "M3RADIO1";

void board_init(void) {
	core_48mhz_init();
}
