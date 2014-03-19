/* irqs.h
 * 
 * Copyright 2011 Brian Swetland <swetland@frotz.net>
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef _IRQ
	_IRQ(watchdog)
	_IRQ(pvd)
	_IRQ(tamper)
	_IRQ(rtc)
	_IRQ(flash)
	_IRQ(rcc)
	_IRQ(extio0)
	_IRQ(extio1)
	_IRQ(extio2)
	_IRQ(extio3)
	_IRQ(extio4)
	_IRQ(dma1_ch1)
	_IRQ(dma1_ch2)
	_IRQ(dma1_ch3)
	_IRQ(dma1_ch4)
	_IRQ(dma1_ch5)
	_IRQ(dma1_ch6)
	_IRQ(dma1_ch7)
	_IRQ(adc)
	_IRQ(usb_hp)
	_IRQ(usb_lp)
	_IRQ(can1_rx1)
	_IRQ(can1_sce)
	_IRQ(extio)
	_IRQ(tim1_brk)
	_IRQ(tim1_up)
	_IRQ(tim1_trg_com)
	_IRQ(tim1_cc)
	_IRQ(tim2)
	_IRQ(tim3)
	_IRQ(tim4)
	_IRQ(i2c1_ev)
	_IRQ(i2c1_er)
	_IRQ(i2c2_ev)
	_IRQ(i2c2_er)
	_IRQ(spi1)
	_IRQ(spi2)
	_IRQ(usart1)
	_IRQ(usart2)
	_IRQ(usart3)
	_IRQ(extio_10_15)
	_IRQ(rtc_alarm)
	_IRQ(usb_wakeup)
#endif

