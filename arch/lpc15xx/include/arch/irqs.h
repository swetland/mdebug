#ifdef _IRQ
	_IRQ(wdt)
	_IRQ(bod)
	_IRQ(flash)
	_IRQ(ee)
	_IRQ(dma)
	_IRQ(gint0)
	_IRQ(gint1)
	_IRQ(pin_int0)
	_IRQ(pin_int1)
	_IRQ(pin_int2)
	_IRQ(pin_int3)
	_IRQ(pin_int4)
	_IRQ(pin_int5)
	_IRQ(pin_int6)
	_IRQ(pin_int7)
	_IRQ(rit)
	_IRQ(sct0)
	_IRQ(sct1)
	_IRQ(sct2)
	_IRQ(sct3)
	_IRQ(mrt)
	_IRQ(uart0)
	_IRQ(uart1)
	_IRQ(uart2)
	_IRQ(i2c0)
	_IRQ(spi0)
	_IRQ(spi1)
	_IRQ(c_can0)
	_IRQ(usb)
	_IRQ(usb_fiq)
	_IRQ(usb_wakeup)
	_IRQ(adc0_seqa)
	_IRQ(adc0_seqb)
	_IRQ(adc0_thcmp)
	_IRQ(adc0_ovr)
	_IRQ(adc1_seqa)
	_IRQ(adc1_seqb)
	_IRQ(adc1_tcmp)
	_IRQ(adc1_ovr)
	_IRQ(dac)
	_IRQ(cmp0)
	_IRQ(cmp1)
	_IRQ(cmp2)
	_IRQ(cmp3)
	_IRQ(qei)
	_IRQ(rtc_alarm)
	_IRQ(rtc_wake)
#endif