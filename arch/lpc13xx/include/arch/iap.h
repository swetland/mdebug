#ifndef __ARCH_LPC13XX_IAP_H__
#define __ARCH_LPC13XX_IAP_H__

int iap_eeprom_read(void *buf, unsigned addr, int len);
int iap_eeprom_write(unsigned addr, void *buf, int len);

#endif /* __ARCH_LPC13XX_IAP_H__ */
