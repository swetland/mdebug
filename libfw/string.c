#include <fw/types.h>

void *memcpy(void *s1, const void *s2, int n)
{
	u8 *dest = (u8 *) s1;
	u8 *src = (u8 *) s2;
	// TODO: arm-cm3 optimized version
	while (n--)
		*dest++ = *src++;

	return s1;
}

void *memset(void *b, int c, int len)
{
	u8 *buff = b;
	while (len--)
		*buff++ = (u8) c;

	return b;
}
