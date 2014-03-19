#ifndef _CPU_H_
#define _CPU_H_

static inline void disable_interrupts(void) {
	asm("cpsid i" : : : "memory");
}
static inline void enable_interrupts(void) {
	asm("cpsie i" : : : "memory");
}

void irq_enable(unsigned n);
void irq_disable(unsigned n);

void irq_set_base(unsigned vector_table_addr);

#endif
