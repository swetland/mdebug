
#define ROM_API_TABLE           ((u32*) 0x10000180)
#define ROM_API_FLASH_TABLE     ((u32*) (ROM_API_TABLE[10]))

#define ROM_FlashPowerModeGet \
    ((u32 (*)(void)) \
    ROM_API_FLASH_TABLE[1])

#define ROM_FlashProtectionSet \
    ((void (*)(u32 ui32SectorAddress, u32 ui32ProtectMode)) \
    ROM_API_FLASH_TABLE[2])

#define ROM_FlashProtectionGet \
    ((u32 (*)(u32 ui32SectorAddress)) \
    ROM_API_FLASH_TABLE[3])

#define ROM_FlashProtectionSave \
    ((u32 (*)(u32 ui32SectorAddress)) \
    ROM_API_FLASH_TABLE[4])

#define ROM_FlashSectorErase \
    ((u32 (*)(u32 ui32SectorAddress)) \
    ROM_API_FLASH_TABLE[5])

#define ROM_FlashProgram \
    ((u32 (*)(const void *pui8DataBuffer, u32 ui32Address, u32 ui32Count)) \
    ROM_API_FLASH_TABLE[6])

#define ROM_FlashEfuseReadRow \
    ((u32 (*)(u32* pui32EfuseData, u32 ui32RowAddress)) \
    ROM_API_FLASH_TABLE[8])

#define ROM_FlashDisableSectorsForWrite \
    ((void (*)(void)) \
    ROM_API_FLASH_TABLE[9])
