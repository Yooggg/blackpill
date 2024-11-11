#include <stdio.h>

#include "main.h"
#include "FXRTOS.h"
#include <stdlib.h>
#include "../Middleware/Fat/fx_file.h"
#include "../Middleware/Fat/fatfs.h"
#include "../Middleware/Fat/fs_media.h"
#include "../Middleware/Spi_Flash/spi_flash.h"

extern SPI_HandleTypeDef hspi1;
fx_mutex_t mutex1;
static fx_thread_t task_flash;
static uint32_t stack_flash[8192 / sizeof(uint32_t)];

static fs_vol_t volume;
static fs_media_t m;


void Task_Flash_Func();

int Callback_Timer_1s();

void Task_Flash_Init()
{



//    trace_set_thread_log_lvl(&(task.trace_handle), THREAD_LOG_LVL_DEFAULT | TRACE_THREAD_PRIO_KEY | TRACE_THREAD_EVENT_KEY /*| TRACE_THREAD_CS_ONLY_KEY*/);

	fx_mutex_init(&mutex1, FX_MUTEX_CEILING_DISABLED, FX_SYNC_POLICY_DEFAULT);

	fx_thread_init(&task_flash, Task_Flash_Func, NULL, 11, (void*)stack_flash, sizeof(stack_flash), false);
}



int cs_en()
{
	HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);
	return 0;
}

int cs_dis()
{
	HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);
	return 0;
}

int read_spi(uint8_t* buf, uint16_t size)
{
	HAL_SPI_Receive(&hspi1, buf, size, HAL_MAX_DELAY);
	return 0;
}

int write_spi(uint8_t* buf, uint16_t size)
{
	HAL_SPI_Transmit(&hspi1, buf, size, HAL_MAX_DELAY);
	return 0;
}

int rw_spi(uint8_t* wbuf, uint8_t* rbuf, uint16_t size)
{
	HAL_SPI_TransmitReceive(&hspi1, wbuf, rbuf, size, HAL_MAX_DELAY);
	return 0;
}


// Команды для работы с флеш-памятью (можно посмотреть в документации к памяти)
#define CMD_WRITE_ENABLE     0x06    // Команда разрешения записи (WREN)
#define CMD_PAGE_PROGRAM     0x02    // Команда записи данных (PP)
#define CMD_READ_STATUS      0x05    // Команда чтения регистра статуса (RDSR)
#define STATUS_BUSY_FLAG     0x01    // Бит занятости в регистре статуса
#define CMD_ENABLE_RESET	0x66
#define CMD_RESET 			0x99

// Функция для передачи команды через SPI
void Flash_SendCommand(uint8_t command) {
    HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET); // CS в LOW (начало передачи)
    HAL_SPI_Transmit(&hspi1, &command, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET); // CS в HIGH (конец передачи)
}

// Функция для разрешения записи (Write Enable)
void Flash_WriteEnable(void) {
    Flash_SendCommand(CMD_WRITE_ENABLE);
}

// Функция для проверки занятости (Polling Busy Flag)
HAL_StatusTypeDef Flash_WaitForWriteEnd(void) {
    uint8_t status;
    do {
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET); // CS в LOW
        uint8_t cmd = CMD_READ_STATUS;
        HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);      // Отправляем команду чтения статуса
        HAL_SPI_Receive(&hspi1, &status, 1, HAL_MAX_DELAY);    // Получаем статус
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);    // CS в HIGH
    } while (status & STATUS_BUSY_FLAG);                      // Пока флаг занятости активен
    return HAL_OK;
}

// Функция записи данных на флеш-память
HAL_StatusTypeDef Flash_WriteData(uint32_t address, uint8_t *data, uint16_t size) {
    // Разрешаем запись
    Flash_WriteEnable();

    // Ожидаем освобождения памяти
    Flash_WaitForWriteEnd();

    uint8_t txBuffer[4]; // Буфер для отправки команды и адреса

    // Формируем команду записи и адрес
    txBuffer[0] = CMD_PAGE_PROGRAM;
    txBuffer[1] = (address >> 16) & 0xFF; // Адрес (старший байт)
    txBuffer[2] = (address >> 8) & 0xFF;  // Адрес (средний байт)
    txBuffer[3] = address & 0xFF;         // Адрес (младший байт)

    // Опускаем CS
    HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);

    // Отправляем команду и адрес
    HAL_SPI_Transmit(&hspi1, txBuffer, 4, HAL_MAX_DELAY);

    // Отправляем данные
    HAL_SPI_Transmit(&hspi1, data, size, HAL_MAX_DELAY);

    // Поднимаем CS
    HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

    // Ожидаем завершения операции записи
    return Flash_WaitForWriteEnd();
}

#define CMD_READ_DATA 0x03  // Команда для чтения данных (обычная скорость)

// Функция для чтения данных из флеш-памяти
HAL_StatusTypeDef Flash_ReadData(uint32_t address, uint8_t *data, uint16_t size) {
    uint8_t txBuffer[4]; // Буфер для команды и адреса

    // Формируем команду чтения и адрес
    txBuffer[0] = CMD_READ_DATA;
    txBuffer[1] = (address >> 16) & 0xFF; // Адрес (старший байт)
    txBuffer[2] = (address >> 8) & 0xFF;  // Адрес (средний байт)
    txBuffer[3] = address & 0xFF;          // Адрес (младший байт)

    // Опускаем CS (начало сеанса передачи данных)
    HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);

    // Отправляем команду и адрес
    HAL_SPI_Transmit(&hspi1, txBuffer, 4, HAL_MAX_DELAY);

    // Получаем данные
    HAL_SPI_Receive(&hspi1, data, size, HAL_MAX_DELAY);

    // Поднимаем CS (конец сеанса передачи данных)
    HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

    return HAL_OK;
}

#define FLASH_MEMORY_ADDRESS  0x000000  // Адрес в памяти для записи данных
#define DATA_SIZE             2       // Количество байт для записи и чтения
#define CMD_CHIP_ERASE    0xC7   // Команда полной очистки памяти

HAL_StatusTypeDef Flash_ChipErase(void) {
    // Разрешаем запись
    Flash_WriteEnable();

    // Отправляем команду полной очистки памяти
    uint8_t cmd = CMD_CHIP_ERASE;
    HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET); // CS в LOW
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);   // CS в HIGH

    // Ожидаем завершения операции стирания
    return Flash_WaitForWriteEnd();
}

void Task_Flash_Func()
{
//	  printf("Device ID: 0x%02X\n", fi.dev_id);
//	 	      printf("Manufacturer ID: 0x%02X\n", fi.mfr_id);
//
//	 	      printf("Unique ID: ");
//	 	      for (int i = 0; i < 8; i++) {
//	 	          printf("0x%02X ", fi.unique_id[i]);
//	 	      }
//	 	      printf("\n");
//
//	 	      printf("Memory Type: 0x%02X\n", fi.mem_type);
//	 	      printf("Capacity: 0x%02X\n", fi.capacity);



	flash_info_t fi;
//		fs_vol_t volume;
//		fs_media_t m;
    m.read = fx_flash_read;
    m.write = fx_flash_write;
    m.sector_erase=fx_spi_Erase_Sector;
    volume.media = &m;
    //volume.fmp.dev = &m;
    void* (*mem_alloc)(size_t) = fs_mem_alloc;

    fx_init_spi_flash(read_spi, write_spi, rw_spi, cs_en, cs_dis);

	uint8_t dataToWrite[DATA_SIZE] = {0xAB, 0xCD};
	uint8_t dataRead[DATA_SIZE];

	fx_spi_chip_erase();
	fx_spi_flash_Reset();

//	Flash_ChipErase();
//	  Flash_SendCommand(CMD_ENABLE_RESET);
//	  Flash_SendCommand(CMD_RESET);


//	     Запись данных во флеш-память
	    if (Flash_WriteData(FLASH_MEMORY_ADDRESS, dataToWrite, DATA_SIZE) == HAL_OK) {
	        printf("Data written successfully.\n");
	    } else {
	        printf("Error writing data.\n");
	        return;
	    }

	    // Ожидание завершения записи
	    Flash_WaitForWriteEnd();

	    // Чтение данных из флеш-памяти
	    if (Flash_ReadData(FLASH_MEMORY_ADDRESS, dataRead, DATA_SIZE) == HAL_OK) {
	        printf("Data read successfully.\n");
	    } else {
	        printf("Error reading data.\n");
	        return;
	    }
//
//	    // Проверка прочитанных данных
//	    for (uint16_t i = 0; i < DATA_SIZE; i++) {
//	        if (dataRead[i] != dataToWrite[i]) {
//	            printf("Data mismatch at index %d: wrote %02X, read %02X\n", i, dataToWrite[i], dataRead[i]);
//	            return;
//	        }
//	    }
//	    printf("Data verified successfully.\n");







	    fx_spi_flash_get_info(&fi);
	    //fx_spi_flash_Reset();

	    uint32_t start_block = 0;
//	    uint32_t offset = 4096 * start_block;
//	    uint8_t cmd_e[1] = {SPI_FLASH_CMD_Write_Enable};
//	    uint8_t cmd_d[1] = {SPI_FLASH_CMD_Write_Disable};
//	    uint8_t data_w[1] = {SPI_FLASH_CMD_Write/*, (offset >> 16) & 0xff, (offset >> 8) & 0xff, offset & 0xff*/};
//	    uint8_t data_r[1] = {SPI_FLASH_CMD_Read/*, (offset >> 16) & 0xff, (offset >> 8) & 0xff, offset & 0xff*/};
//	    uint8_t test_data_write[] = {0xAB, 0xCD};
//	    uint8_t test_data_read[2] = {};
////
	    static uint8_t sec_buf[SEC_SIZE];

	    //fx_spi_chip_erase();

//	    cs_en();
//	    write_spi(cmd_e,1);
//	    //cs_dis();
//	    write_spi(data_w,1);
//
//	    write_spi(test_data_write,2);
//	    write_spi(cmd_d,1);
//	    cs_dis();
//	    cs_en();
//	    write_spi(data_r, 1);
//	    read_spi(test_data_read,2);
//
//	    cs_dis();




//		m.write(&m,dataToWrite,DATA_SIZE,0);
//		m.read(&m,dataRead,DATA_SIZE,0);

//		fx_flash_write(&m,dataToWrite,DATA_SIZE,0);
//		fx_flash_read(&m,dataRead,DATA_SIZE,0);

//	    size_t nbytes_write = sizeof(test_data_write);
//	    size_t nbytes_read = sizeof(test_data_read);
//
//
//	    int result = fx_flash_write(&m, test_data_write, &nbytes_write, start_block);
//
//	    if (result == 0) {
//	        printf("Write func is working fine.\n");
//	    } else {
//	        printf("Write func isn't working.\n");
//	    }
//
//	    result = fx_flash_read(&m, test_data_read, &nbytes_read, start_block);
//	    if (result == 0) {
//	    	printf("Data: ");
//	    	for (size_t i = 0; i < nbytes_read; i++) {
//	    		printf("0x%02X ", test_data_read[i]);
//	    	}
//	    	    printf("\n");;
//	    	} else {
//	    		printf("read func isn't working.\n");
//	    	}





	    fatfs_format(&m, 512, sec_buf);

	    printf("media: %p, volume: %p\n", (void *)&m, (void *)&volume);
	    fs_volume_open(&m, &volume, 0, mem_alloc);
	    		fs_dir_t dir;
	    		fs_dir_create(&volume, "/dir1");
	    		printf("Directory created");
	    		fx_thread_sleep(200);
	    		fs_dir_open(&volume, "/dir1", &dir);
	    		printf("Directory opened");
	    		fx_thread_sleep(200);
	    		fs_volume_close(&volume);
	    		printf("Directory closed");

	while (1)
	{
//		fx_spi_flash_get_info(&fi);
//		printf("\r\n----------------------------------\r\n");
//
//		 printf("\n\rDevice ID: 0x%02X\r\n", fi.dev_id);
//		    printf("\n\rManufacturer ID: 0x%02X\r\n", fi.mfr_id);
//
//		    printf("\n\rUnique ID: \r\n");
//		    for (int i = 0; i < 8; i++) {
//		        printf("0x%02X ", fi.unique_id[i]);
//		    }
//		    printf("\n\r");
//
//		    printf("\n\rMemory Type: 0x%02X\r\n", fi.mem_type);
//		    printf("\n\rCapacity: 0x%02X\r\n", fi.capacity);








		//HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
		fx_thread_sleep(200);

		/*
		uint16_t id = fx_spi_flash_GetId();

		printf("RDID: 0x%4X\n", id);
		 */

		//HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
		fx_thread_sleep(300);

		/*
		fx_spi_flash_TestRead();

		uint8_t txdata[3] = {0xA5, 0, 0};
		uint8_t rxdata[3] = {0, 0, 0};

		__HAL_SPI_ENABLE(&hspi4);
		HAL_SPI_TransmitReceive(&hspi4, txdata, rxdata, 3, HAL_MAX_DELAY);
	    __HAL_SPI_DISABLE(&hspi4);

		printf("CMX7364: 0x%2X, 0x%2X, 0x%2X\n", rxdata[0], rxdata[1], rxdata[2]);
		*/
	}
}
