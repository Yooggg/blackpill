
/*
 * fx_spi_flash.h
 *
 *  Created on: Jun 26, 2024
 *      Author: bobrjc
 */
#ifndef FX_SPI_FLASH_H_
#define FX_SPI_FLASH_H_
#include <stdint.h>
#include <stddef.h>
#define SPI_FLASH_PAGE_SIZE 256
#define SPI_FLASH_SEC_SIZE 4096
#define SPI_FLASH_CMD_RDID	0x90
#define SPI_FLASH_CMD_JEDEC_ID 0x9F
#define SPI_FLASH_CMD_UNIQUE_ID 0x4B
#define SPI_FLASH_CMD_Read	0x03
#define SPI_FLASH_CMD_Fast_Read	0x0B
#define SPI_FLASH_CMD_Write	0x02
#define SPI_FLASH_CMD_Erase_Chip 0x60
#define SPI_FLASH_CMD_Write_Disable  0x04
#define SPI_FLASH_CMD_Write_Enable 0x06
#define SPI_FLASH_CMD_Enable_Reset 0x66
#define SPI_FLASH_CMD_Reset 0x99
#define SPI_FLASH_READ_STATUS_1 0x05
#define SPI_FLASH_READ_STATUS_2 0x35
#define SPI_FLASH_READ_STATUS_3 0x15
#define SPI_FLASH_WRITE_STATUS_1 0x01
#define SPI_FLASH_WRITE_STATUS_2 0x31
#define SPI_FLASH_WRITE_STATUS_3 0x11
#define SPI_FLASH_SECTOR_ERASE 0x20
#define SPI_FLASH_VOLITILE_STATUS 0x50
#define FLASH_CS_ENABLED	HAL_GPIO_WritePin(nCS_GPIO_Port, nCS_Pin, GPIO_PIN_RESET)
#define FLASH_CS_DISABLED	HAL_GPIO_WritePin(nCS_GPIO_Port, nCS_Pin, GPIO_PIN_SET)
enum {
	PUYA,
	GIGADEVICE,
	WINBOND
};
typedef struct flash_id {
	uint8_t dev_id;
	uint8_t mfr_id;
	uint8_t unique_id[8];
	uint8_t mem_type;
	uint8_t capacity;
} flash_info_t;
typedef struct spi_flash_rw
{
    int (*read)(uint8_t*, uint16_t);  
    int (*write)(uint8_t*, uint16_t);
    int (*readwrite)(uint8_t*, uint8_t*, uint16_t);
} spi_flash_rw_t;
typedef struct spi_flash_cs
{
    int (*enable)();  
    int (*disable)();
} spi_flash_cs_t;
typedef struct spi_flash
{
	flash_info_t specs;
	spi_flash_rw_t rw;
	char* buf;
	//fx_mutex_t
} spi_flash_t;

void fx_spi_Erase_Sector(uint32_t blkno);
int fx_spi_chip_erase();
void fx_spi_Set_Block_Protect(uint8_t blkno);
void fx_spi_Wait_Write_End(void);
void fx_spi_write_enable();
void fx_spi_write_disable();
void fx_init_spi_flash(int (*r)(uint8_t*, uint16_t), 
					int (*w)(uint8_t*, uint16_t), 
					int (*rw)(uint8_t*, uint8_t*, uint16_t),
					int (*en)(), int (*dis)());
void * fs_mem_alloc(size_t size);
void fx_spi_flash_Reset (void);
int fx_flash_write(void* buf, uint32_t* nbyte, uint32_t blkno);
int fx_flash_read(void* buf, uint32_t* nbyte, uint32_t blkno);
extern void HAL_Delay(uint32_t delay);

void fx_spi_flash_get_info(flash_info_t* fi);
#endif /* FX_SPI_FLASH_H_ */
