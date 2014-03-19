
#include <fw/types.h>
#include <arch/hardware.h>

const u32 gpio_led0 = MKGPIO(3, 3);
const u32 gpio_led1 = MKGPIO(2, 8);
const u32 gpio_led2 = MKGPIO(0, 5);
const u32 gpio_led3 = MKGPIO(1, 9);

const u8 board_name[] = "M3RADIO2";

void board_init(void) {
	core_48mhz_init();
}
