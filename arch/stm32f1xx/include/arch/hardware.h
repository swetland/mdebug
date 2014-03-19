/* hardware.h
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

#ifndef __STM32_HW_H_
#define __STM32_HW_H_

#define RCC_BASE	0x40021000
#define DMA2_BASE	0x40020000
#define DMA1_BASE	0x40020400

#define USART1_BASE	0x40013800
#define SPI1_BASE	0x40013000
#define TIM1_BASE	0x40012C00
#define ADC1_BASE	0x40012400

#define GPIOD_BASE	0x40011400
#define GPIOC_BASE	0x40011000
#define GPIOB_BASE	0x40010C00
#define GPIOA_BASE	0x40010800

#define PWR_BASE	0x40007000
#define BKP_BASE	0x40006C00
#define USB_SRAM_BASE	0x40006000
#define USB_BASE	0x40005C00
#define I2C2_BASE	0x40005800
#define I2C1_BASE	0x40005400


#define RCC_CR		(RCC_BASE + 0x00)
#define RCC_CFGR	(RCC_BASE + 0x04)
#define RCC_CIR		(RCC_BASE + 0x08)
#define RCC_APB2RSTR	(RCC_BASE + 0x0C)
#define RCC_APB1RSTR	(RCC_BASE + 0x10)
#define RCC_AHBENR	(RCC_BASE + 0x14)
#define RCC_APB2ENR	(RCC_BASE + 0x18)
#define RCC_APB1ENR	(RCC_BASE + 0x1C)
#define RCC_BDCR	(RCC_BASE + 0x20)
#define RCC_CSR		(RCC_BASE + 0x24)

/* for RCC_APB2_{RSTR,ENR} */
#define RCC_APB2_AFIO	(1 << 0)
#define RCC_APB2_GPIOA	(1 << 2)
#define RCC_APB2_GPIOB	(1 << 3)
#define RCC_APB2_GPIOC	(1 << 4)
#define RCC_APB2_TIM1	(1 << 11)
#define RCC_APB2_SPI1	(1 << 12)
#define RCC_APB2_USART1	(1 << 14)

#define RCC_APB1_USB	(1 << 23)


#define GPIO_CRL	0x00
#define GPIO_CRH	0x04
#define GPIO_IDR	0x08
#define GPIO_ODR	0x0C
#define GPIO_BSRR	0x10
#define GPIO_BSR	0x10
#define GPIO_BRR	0x14
#define GPIO_LCKR	0x18

/* base mode */
#define GPIO_INPUT		0x00
#define GPIO_OUTPUT_10MHZ	0x01
#define GPIO_OUTPUT_2MHZ	0x02
#define GPIO_OUTPUT_50MHZ	0x50
/* input submodes */
#define GPIO_ANALOG		0x00
#define GPIO_FLOATING		0x04
#define GPIO_PU_PD		0x08
/* output submodes */
#define GPIO_OUT_PUSH_PULL	0x00
#define GPIO_OUT_OPEN_DRAIN	0x04
#define GPIO_ALT_PUSH_PULL	0x08
#define GPIO_ALT_OPEN_DRAIN	0x0C

#define USART_SR		0x00
#define USART_DR		0x04
#define USART_BRR		0x08
#define USART_CR1		0x0C
#define USART_CR2		0x10
#define USART_CR3		0x14
#define USART_GTPR		0x18

#define USART_SR_TXE		(1 << 7)
#define USART_SR_RXNE		(1 << 5)

#define USART_CR1_ENABLE	(1 << 13) // enable
#define USART_CR1_9BIT		(1 << 12)
#define USART_CR1_PARITY	(1 << 10)
#define USART_CR1_ODD		(1 << 9)
#define USART_CR1_TX_ENABLE	(1 << 3)
#define USART_CR1_RX_ENABLE	(1 << 2)


#define SPI_CR1			0x00
#define SPI_CR2			0x04
#define SPI_SR			0x08
#define SPI_DR			0x0C

#define SPI_CR1_BIDI_MODE	(1 << 15)
#define SPI_CR1_BIDI_OE		(1 << 14)
#define SPI_CR1_8BIT		(0 << 11)
#define SPI_CR1_16BIT		(1 << 11)
#define SPI_CR1_RX_ONLY		(1 << 10)
#define SPI_CR1_SSM		(1 << 9) /* sw control of NSS */
#define SPI_CR1_SSI		(1 << 8) 
#define SPI_CR1_MSB_FIRST	(0 << 7)
#define SPI_CR1_LSB_FIRST	(1 << 7)
#define SPI_CR1_ENABLE		(1 << 6)
#define SPI_CR1_CLKDIV_2	(0 << 3)
#define SPI_CR1_CLKDIV_4	(1 << 3)
#define SPI_CR1_CLKDIV_8	(2 << 3)
#define SPI_CR1_CLKDIV_16	(3 << 3)
#define SPI_CR1_CLKDIV_32	(4 << 3)
#define SPI_CR1_CLKDIV_64	(5 << 3)
#define SPI_CR1_CLKDIV_128	(6 << 3)
#define SPI_CR1_CLKDIV_256	(7 << 3)
#define SPI_CR1_MASTER		(1 << 2)
#define SPI_CR1_CK_NEG		(0 << 1)
#define SPI_CR1_CK_POS		(1 << 1)
#define SPI_CR1_CK_PHASE0	(0 << 0)
#define SPI_CR1_CK_PHASE1	(1 << 0)

#define SPI_CR2_SS_OE		(1 << 2) /* enable in single-master mode */

#define SPI_SR_BUSY		(1 << 7)
#define SPI_SR_OVERRUN		(1 << 6)
#define SPI_SR_MODE_FAULT	(1 << 5)
#define SPI_SR_UNDERRUN		(1 << 3)
#define SPI_SR_TX_EMPTY		(1 << 1)
#define SPI_SR_RX_FULL		(1 << 0)


#define USB_EPR(n)		(USB_BASE + (n * 4))
#define USB_CR			(USB_BASE + 0x40)
#define USB_ISR			(USB_BASE + 0x44)
#define USB_FNR			(USB_BASE + 0x48)
#define USB_DADDR		(USB_BASE + 0x4C)
#define USB_BTABLE		(USB_BASE + 0x50)

/* the *M bits apply to both CR (to enable) and ISR (to read) */
#define USB_CTRM		(1 << 15)
#define USB_PMAOVRM		(1 << 14)
#define USB_ERRM		(1 << 13)
#define USB_WKUPM		(1 << 12)
#define USB_SUSPM		(1 << 11)
#define USB_RESETM		(1 << 10)
#define USB_SOFM		(1 << 9)
#define USB_ESOFM		(1 << 8)

#define USB_CR_RESUME		(1 << 4)
#define USB_CR_FSUSP		(1 << 3)
#define USB_CR_LP_MODE		(1 << 2)
#define USB_CR_PDWN		(1 << 1)
#define USB_CR_FRES		(1 << 0)

#define USB_ISR_DIR		(1 << 4)
#define USB_ISR_EP_MASK		0xF

#define USB_DADDR_ENABLE	(1 << 7)

#define USB_EPR_CTR_RX		(1 << 15) // R+W0C
#define USB_EPR_DTOG_RX		(1 << 14) // T
#define USB_EPR_RX_DISABLE	(0 << 12) // T
#define USB_EPR_RX_STALL	(1 << 12) // T
#define USB_EPR_RX_NAK		(2 << 12) // T
#define USB_EPR_RX_VALID	(3 << 12) // T
#define USB_EPR_SETUP		(1 << 11) // RO
#define USB_EPR_TYPE_BULK	(0 << 9)  // RW
#define USB_EPR_TYPE_CONTROL	(1 << 9)  // RW
#define USB_EPR_TYPE_ISO	(2 << 9)  // RW
#define USB_EPR_TYPE_INTERRRUPT	(3 << 9)  // RW
#define USB_EPR_TYPE_MASK	(3 << 9)
#define USB_EPR_DBL_BUF		(1 << 8)  // RW (for BULK)
#define USB_EPR_STATUS_OUT	(1 << 8)  // RW (for CONTROL)
#define USB_EPR_CTR_TX		(1 << 7)  // R+W0C
#define USB_EPR_DTOG_TX		(1 << 6)  // T
#define USB_EPR_TX_DISABLED	(0 << 4)  // T
#define USB_EPR_TX_STALL	(1 << 4)  // T
#define USB_EPR_TX_NAK		(2 << 4)  // T
#define USB_EPR_TX_VALID	(3 << 4)  // T
#define USB_EPR_ADDR_MASK	(0x0F)    // RW

#define USB_ADDR_TX(n)		(USB_SRAM_BASE + ((n) * 16) + 0x00)
#define USB_COUNT_TX(n)		(USB_SRAM_BASE + ((n) * 16) + 0x04)
#define USB_ADDR_RX(n)		(USB_SRAM_BASE + ((n) * 16) + 0x08)
#define USB_COUNT_RX(n)		(USB_SRAM_BASE + ((n) * 16) + 0x0C)

#define USB_RX_SZ_8		((0 << 15) | (4 << 10))
#define USB_RX_SZ_16		((0 << 15) | (8 << 10))
#define USB_RX_SZ_32		((1 << 15) | (0 << 10))
#define USB_RX_SZ_64		((1 << 15) | (1 << 10))
#define USB_RX_SZ_128		((1 << 15) | (3 << 10))
#define USB_RX_SZ_256		((1 << 15) | (7 << 10))

#define _IRQ(name) i_##name ,
enum {
#include "irqs.h"
};
#undef _IRQ

void gpio_config(unsigned n, unsigned cfg);

#endif

