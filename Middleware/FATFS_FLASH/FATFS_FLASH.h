/*
 * File: FATFS_FLASH.h
 * Driver Name: [[ FATFS_FLASH SPI ]]
 * SW Layer:   MIDWARE
 * Author:
 * -------------------------------------------
 * FatFS driver for W25Q64FW SPI Flash Memory
 */
#ifndef FATFS_FLASH_H_
#define FATFS_FLASH_H_

#include "ff_gen_drv.h"
#include "main.h"

//-----[ FLASH Card SPI Interface Cfgs ]-----

extern SPI_HandleTypeDef 	hspi1;
#define HSPI_FLASH 			&hspi1
#define FLASH_CS_PORT 		Flash_CS_GPIO_Port
#define FLASH_CS_PIN 		Flash_CS_Pin

// W25Q64FW параметры
#define FLASH_SECTOR_SIZE   512    // Размер сектора flash памяти (4KB)
#define FLASH_SECTOR_COUNT  16384    // Количество секторов (8MB / 4KB = 2048)
#define FLASH_BLOCK_SIZE    8       // Для FatFS erase block size (в секторах)

//-----[ Prototypes For All User External Functions ]-----
DSTATUS FLASH_disk_initialize(BYTE pdrv);
DSTATUS FLASH_disk_status(BYTE pdrv);
DRESULT FLASH_disk_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count);
DRESULT FLASH_disk_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count);
DRESULT FLASH_disk_ioctl(BYTE pdrv, BYTE cmd, void* buff);

#endif /* FATFS_FLASH_H_ */
