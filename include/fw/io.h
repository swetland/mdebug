/* io.h */

#ifndef _IO_H_
#define _IO_H_

#define readb(a) (*((volatile unsigned char *) (a)))
#define writeb(v, a) (*((volatile unsigned char *) (a)) = (v))

#define readw(a) (*((volatile unsigned short *) (a)))
#define writew(v, a) (*((volatile unsigned short *) (a)) = (v))

#define readl(a) (*((volatile unsigned int *) (a)))
#define writel(v, a) (*((volatile unsigned int *) (a)) = (v))

#endif
