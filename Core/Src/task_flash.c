#include <fs_data.h>
#include "task_flash.h"
#include "../../FATFS/App/fatfs.h"

extern SPI_HandleTypeDef hspi1;
fx_mutex_t mutex1;

fs_vol_t volume;
fs_media_t m;
fs_dir_t dir;
fs_file_t file;

char TxBuffer[250];

void* fs_mem_alloc(size_t size)
{
	return malloc(size);
}


void Task_Flash_Func();

int Callback_Timer_1s();

void Task_Flash_Init()
{

	static fx_thread_t task_flash;
	static int stack_flash[8192*8 / sizeof(int)];

	fx_mutex_init(&mutex1, FX_MUTEX_CEILING_DISABLED, FX_SYNC_POLICY_DEFAULT);

	fx_thread_init(&task_flash, Task_Flash_Func, NULL, 10, (void*)stack_flash, sizeof(stack_flash), false);
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
int fx_read(struct _fs_media_t* m, void* buf, uint32_t* size, uint32_t blkno)
{
	return fx_flash_read(buf, size, blkno);
}
int fx_write(struct _fs_media_t* m, void* buf, uint32_t* size, uint32_t blkno)
{
	return fx_flash_write(buf, size, blkno);
}
int fx_erase(struct _fs_media_t* m, void* buf, uint32_t* size, uint32_t blkno)
{
	fx_spi_Erase_Sector(blkno);
	return 0;
}

/*old version with eremex fs
 * */
//void Task_Flash_Func()
//{
//	// array size is 299
//	char index_html[]  = {
//	  0x1f, 0x8b, 0x08, 0x08, 0xb1, 0xd4, 0xa5, 0x68, 0x04, 0x00, 0x69, 0x6e, 0x64, 0x65, 0x78, 0x2e,
//	  0x68, 0x74, 0x6d, 0x6c, 0x00, 0x5d, 0x51, 0x4d, 0x4f, 0xc3, 0x30, 0x0c, 0xbd, 0x23, 0xf1, 0x1f,
//	  0x4c, 0xce, 0xb4, 0x1d, 0x02, 0x4d, 0x9b, 0xd4, 0x54, 0x42, 0x7c, 0x5c, 0xe1, 0x30, 0x90, 0x38,
//	  0x86, 0xc4, 0x5d, 0x0d, 0x69, 0x1a, 0x25, 0x66, 0x63, 0xff, 0x1e, 0xb7, 0xfb, 0x90, 0xd8, 0x25,
//	  0xf1, 0x8b, 0xed, 0xf7, 0xfc, 0x9c, 0xfa, 0xea, 0xf1, 0xe5, 0x61, 0xf5, 0xf1, 0xfa, 0x04, 0x1d,
//	  0xf7, 0xbe, 0xb9, 0xbc, 0xa8, 0xc7, 0x1b, 0xbc, 0x09, 0x6b, 0xad, 0x30, 0x28, 0x79, 0x01, 0xa8,
//	  0x3b, 0x34, 0x6e, 0x8a, 0x24, 0xee, 0x91, 0x0d, 0xd8, 0xce, 0xa4, 0x8c, 0xac, 0xd5, 0xdb, 0xea,
//	  0xb9, 0x58, 0xa8, 0x63, 0xce, 0x53, 0xf8, 0x86, 0x84, 0x5e, 0x2b, 0xb2, 0x43, 0x50, 0xd0, 0x25,
//	  0x6c, 0xb5, 0xaa, 0x5a, 0xb3, 0x19, 0x71, 0x29, 0x87, 0xfa, 0x47, 0x13, 0x4c, 0x8f, 0x5a, 0x6d,
//	  0x08, 0xb7, 0x71, 0x48, 0xac, 0x40, 0x8a, 0x18, 0x83, 0xd0, 0x6e, 0xc9, 0x71, 0xa7, 0x1d, 0x4a,
//	  0x1f, 0x16, 0x13, 0xb8, 0x06, 0x0a, 0xc4, 0x64, 0x7c, 0x91, 0xad, 0xf1, 0xa8, 0x6f, 0xca, 0xd9,
//	  0x89, 0x8b, 0x89, 0x3d, 0x36, 0xef, 0xc4, 0x08, 0xf7, 0x31, 0xd6, 0xd5, 0x84, 0x8f, 0xc9, 0x6c,
//	  0x13, 0x45, 0x06, 0xde, 0x45, 0x91, 0xea, 0x07, 0xf7, 0xe3, 0x51, 0x84, 0xd2, 0x90, 0xf3, 0x90,
//	  0x68, 0x4d, 0x01, 0x72, 0xb2, 0x32, 0xa3, 0xc9, 0xe2, 0x27, 0x57, 0x14, 0x1c, 0xfe, 0x16, 0xad,
//	  0x33, 0x8b, 0xf9, 0x12, 0x4d, 0xf9, 0x95, 0x55, 0x53, 0x57, 0x7b, 0x8a, 0xe6, 0xdc, 0x63, 0xe6,
//	  0x9d, 0xc7, 0xdc, 0x21, 0xf2, 0xde, 0xe9, 0x39, 0x8b, 0x9b, 0xcf, 0x6e, 0xdb, 0xbb, 0xa5, 0x2d,
//	  0x6d, 0x16, 0x9a, 0x71, 0x8f, 0xd5, 0x69, 0x91, 0xf5, 0xe7, 0xe0, 0x76, 0xc7, 0x11, 0x1d, 0x6d,
//	  0x80, 0x9c, 0x56, 0x26, 0xc6, 0x51, 0x4e, 0xe0, 0x21, 0x73, 0x39, 0x35, 0x1d, 0x4a, 0xa5, 0x7b,
//	  0xff, 0x45, 0x7f, 0x94, 0xaf, 0xfe, 0xb5, 0xb4, 0x01, 0x00, 0x00
//	};
//
//
//	uint8_t sec_buf[SEC_SIZE];
////	fs_vol_t volume;
////	fs_media_t m;
//	flash_info_t fi;
//    m.read = fx_read;
//    m.write = fx_write;
//    m.sector_erase=fx_erase;
//    volume.media = &m;
////////////////////////////////////////////////////////////////////////////////
////  fx_init_spi_flash(read_spi, write_spi, rw_spi, cs_en, cs_dis);
////
////	fx_spi_flash_Reset();
////	fx_spi_chip_erase();
////
////	fx_spi_flash_get_info(&fi);
////	fatfs_format(&m, 512, sec_buf);
////
////	printf("media: %p, volume: %p\n", (void *)&m, (void *)&volume);
////	fs_volume_open(&m, &volume, 0, fs_mem_alloc);
////
////	fs_dir_create(&volume, "/web");
////	fs_dir_open(&volume, "/web", &dir);
////	fs_dir_close(&dir);
////	add_fs_item("/web", FS_TYPE_FOLDER);
////	char* filename = "/web/index.js.gz";
////	fs_dir_open(&volume, "/web", &dir);
////	fs_file_create(&volume, filename);
////	//fs_file_t file;
////	//char data_w[2] = "ab";
////	char data_r[299];
////	fs_file_open(&volume, &file, filename, 0);
////	size_t size = 0;
////	fs_file_write(&file, index_html, 299, &size);
////	fs_file_open(&volume, &file, filename, 0);
////	fs_file_read(&file, data_r, 299, &size);
////	fs_dir_close(&dir);
////
////
////	fs_dir_create(&volume, "/firmware");
////	printf("Directory created");
////	fs_dir_open(&volume, "/firmware", &dir);
////	add_fs_item("/firmware", FS_TYPE_FOLDER);
////	fs_dir_close(&dir);
/////////////////////////////////////////////////////////////////////////////////
//
////	fs_dir_create(&volume, "/dir2");
////	fs_dir_open(&volume, "/dir2", &dir);
////	fs_dir_close(&dir);
////
////	fs_dir_create(&volume, "/dir1");
////	fs_dir_open(&volume, "/dir1", &dir);
////	fs_dir_close(&dir);
////	char* filename = "/dir1/file.txt";
////	fs_dir_open(&volume, "/dir1", &dir);
////	fs_file_create(&volume, filename);
////	//fs_file_t file;
////	char data_w[2] = "ab";
////	char data_r[2];
////	fs_file_open(&volume, &file, filename, 0);
////	size_t size = 0;
////	fs_file_write(&file, data_w, 2, &size);
////	fs_file_open(&volume, &file, filename, 0);
////	fs_file_read(&file, data_r, 2, &size);
//
/////////////////////////////////////////////////////////////////////////////////
////	fs_dir_close(&dir);
/////////////////////////////////////////////////////////////////////////////////
//
//	SD_Card_Test();
//
//	while (1)
//	{
//		fx_thread_sleep(200);
//	}
//}

//void SD_Card_Test()
//{
//  FATFS FatFs;
//  FIL Fil;
//  FRESULT FR_Status;
//  FATFS *FS_Ptr;
//  UINT RWC, WWC; // Read/Write Word Counter
//  DWORD FreeClusters;
//  uint32_t TotalSize, FreeSpace;
//  char RW_Buffer[200];
//  do
//  {
//    //------------------[ Mount The SD Card ]--------------------
//    FR_Status = f_mount(&FatFs, "", 1);
//    if (FR_Status != FR_OK)
//    {
//      sprintf(TxBuffer, "Error! While Mounting SD Card, Error Code: (%i)\r\n", FR_Status);
//      printf("%s\n",TxBuffer);
//      break;
//    }
//    sprintf(TxBuffer, "SD Card Mounted Successfully! \r\n\n");
//    printf("%s\n",TxBuffer);
//    //------------------[ Get & Print The SD Card Size & Free Space ]--------------------
//    f_getfree("", &FreeClusters, &FS_Ptr);
//    TotalSize = (uint32_t)((FS_Ptr->n_fatent - 2) * FS_Ptr->csize * 0.5);
//    FreeSpace = (uint32_t)(FreeClusters * FS_Ptr->csize * 0.5);
//    sprintf(TxBuffer, "Total SD Card Size: %lu Bytes\r\n", TotalSize);
//    printf("%s\n",TxBuffer);
//    sprintf(TxBuffer, "Free SD Card Space: %lu Bytes\r\n\n", FreeSpace);
//    printf("%s\n",TxBuffer);
//    //------------------[ Open A Text File For Write & Write Data ]--------------------
//    //Open the file
//    FR_Status = f_open(&Fil, "TextFileWrite.txt", FA_WRITE | FA_READ | FA_CREATE_ALWAYS);
//    if(FR_Status != FR_OK)
//    {
//      sprintf(TxBuffer, "Error! While Creating/Opening A New Text File, Error Code: (%i)\r\n", FR_Status);
//      printf("%s\n",TxBuffer);
//      break;
//    }
//    sprintf(TxBuffer, "Text File Created & Opened! Writing Data To The Text File..\r\n\n");
//    printf("%s\n",TxBuffer);
//    // (1) Write Data To The Text File [ Using f_puts() Function ]
//    f_puts("Hello! From STM32 To SD Card Over SPI, Using f_puts()\n", &Fil);
//    // (2) Write Data To The Text File [ Using f_write() Function ]
//    strcpy(RW_Buffer, "Hello! From STM32 To SD Card Over SPI, Using f_write()\r\n");
//    f_write(&Fil, RW_Buffer, strlen(RW_Buffer), &WWC);
//    // Close The File
//    f_close(&Fil);
//    //------------------[ Open A Text File For Read & Read Its Data ]--------------------
//    // Open The File
//    FR_Status = f_open(&Fil, "TextFileWrite.txt", FA_READ);
//    if(FR_Status != FR_OK)
//    {
//      sprintf(TxBuffer, "Error! While Opening (TextFileWrite.txt) File For Read.. \r\n");
//      printf("%s\n",TxBuffer);
//      break;
//    }
//    // (1) Read The Text File's Data [ Using f_gets() Function ]
//    f_gets(RW_Buffer, sizeof(RW_Buffer), &Fil);
//    sprintf(TxBuffer, "Data Read From (TextFileWrite.txt) Using f_gets():%s", RW_Buffer);
//    printf("%s\n",TxBuffer);
//    // (2) Read The Text File's Data [ Using f_read() Function ]
//    f_read(&Fil, RW_Buffer, f_size(&Fil), &RWC);
//    sprintf(TxBuffer, "Data Read From (TextFileWrite.txt) Using f_read():%s", RW_Buffer);
//    printf("%s\n",TxBuffer);
//    // Close The File
//    f_close(&Fil);
//    sprintf(TxBuffer, "File Closed! \r\n\n");
//    printf("%s\n",TxBuffer);
//    //------------------[ Open An Existing Text File, Update Its Content, Read It Back ]--------------------
//    // (1) Open The Existing File For Write (Update)
//    FR_Status = f_open(&Fil, "TextFileWrite.txt", FA_OPEN_EXISTING | FA_WRITE);
//    FR_Status = f_lseek(&Fil, f_size(&Fil)); // Move The File Pointer To The EOF (End-Of-File)
//    if(FR_Status != FR_OK)
//    {
//      sprintf(TxBuffer, "Error! While Opening (TextFileWrite.txt) File For Update.. \r\n");
//      printf("%s\n",TxBuffer);
//      break;
//    }
//    // (2) Write New Line of Text Data To The File
//    FR_Status = f_puts("This New Line Was Added During Update!\r\n", &Fil);
//    f_close(&Fil);
//    memset(RW_Buffer,'\0',sizeof(RW_Buffer)); // Clear The Buffer
//    // (3) Read The Contents of The Text File After The Update
//    FR_Status = f_open(&Fil, "TextFileWrite.txt", FA_READ); // Open The File For Read
//    f_read(&Fil, RW_Buffer, f_size(&Fil), &RWC);
//    sprintf(TxBuffer, "Data Read From (TextFileWrite.txt) After Update:%s", RW_Buffer);
//    printf("%s\n",TxBuffer);
//    f_close(&Fil);
//    //------------------[ Delete The Text File ]--------------------
//    // Delete The File
//    /*
//    FR_Status = f_unlink(TextFileWrite.txt);
//    if (FR_Status != FR_OK){
//        sprintf(TxBuffer, "Error! While Deleting The (TextFileWrite.txt) File.. \r\n");
//        UART_Print(TxBuffer);
//    }
//    */
//  } while(0);
//  //------------------[ Test Complete! Unmount The SD Card ]--------------------
//  FR_Status = f_mount(NULL, "", 0);
//  if (FR_Status != FR_OK)
//  {
//      sprintf(TxBuffer, "Error! While Un-mounting SD Card, Error Code: (%i)\r\n", FR_Status);
//      printf("%s\n",TxBuffer);
//  } else{
//      sprintf(TxBuffer, "SD Card Un-mounted Successfully! \r\n");
//      printf("%s\n",TxBuffer);
//  }
//}

// Замените функцию Task_Flash_Func() в вашем task_flash.c на эту версию:

void Task_Flash_Func()
{
	printf("\r\n=== Flash Memory FatFS Initialization ===\r\n");

	// Инициализация SPI Flash
	fx_init_spi_flash(read_spi, write_spi, rw_spi, cs_en, cs_dis);
	printf("SPI Flash interface initialized\r\n");

	// Сброс flash памяти
	fx_spi_flash_Reset();
	printf("Flash reset complete\r\n");

	// Получение информации о flash
	flash_info_t fi;
	fx_spi_flash_get_info(&fi);
	printf("Flash Info - MFR: 0x%02X, DEV: 0x%02X, MEM: 0x%02X, CAP: 0x%02X\r\n",
	       fi.mfr_id, fi.dev_id, fi.mem_type, fi.capacity);

	// Опционально: форматирование flash (раскомментируйте если нужно отформатировать)
	// ВНИМАНИЕ: это сотрет все данные!
	/*
	printf("Formatting flash memory... (this will take some time)\r\n");
	fx_spi_chip_erase();
	printf("Flash erased\r\n");
	*/

	// Инициализация FatFS
	FATFS FatFs;
	FIL Fil;
	FRESULT FR_Status;
	UINT RWC, WWC;
	char RW_Buffer[256];
//	BYTE work[4096];
//	f_mkfs("0:", FM_FAT, 0, work, sizeof(work));
//	f_mount(&FatFs, "0:", 1);
	printf("\r\n=== Mounting FatFS ===\r\n");

	// Монтирование файловой системы
	FR_Status = f_mount(&FatFs, "", 1);
	if (FR_Status != FR_OK) {
		sprintf(TxBuffer, "Error! Mounting failed, Error Code: (%i)\r\n", FR_Status);
		printf("%s", TxBuffer);

		// Если не получилось смонтировать, попробуем отформатировать
		if (FR_Status == FR_NO_FILESYSTEM) {
			printf("No filesystem found. Formatting flash...\r\n");

			// Форматирование с параметрами для flash
			// FM_FAT32 - тип файловой системы
			// 0 - использовать весь диск
			// work buffer для форматирования
			BYTE work[4096];
			FR_Status = f_mkfs("", FM_FAT, 0, work, sizeof(work));

			if (FR_Status != FR_OK) {
				sprintf(TxBuffer, "Error! Formatting failed, Error Code: (%i)\r\n", FR_Status);
				printf("%s", TxBuffer);
			} else {
				printf("Flash formatted successfully!\r\n");

				// Повторная попытка монтирования
				FR_Status = f_mount(&FatFs, "", 1);
				if (FR_Status != FR_OK) {
					sprintf(TxBuffer, "Error! Mounting after format failed, Error Code: (%i)\r\n", FR_Status);
					printf("%s", TxBuffer);
				} else {
					printf("Flash mounted successfully after format!\r\n");
				}
			}
		}
	} else {
		printf("Flash mounted successfully!\r\n");
	}

	// Если монтирование успешно, протестируем запись/чтение
	if (FR_Status == FR_OK) {
		printf("\r\n=== Testing File Operations ===\r\n");

		// Получение информации о свободном месте
		FATFS *pfs;
		DWORD fre_clust;
		FR_Status = f_getfree("", &fre_clust, &pfs);
		if (FR_Status == FR_OK) {
			DWORD tot_sect = (pfs->n_fatent - 2) * pfs->csize;
			DWORD fre_sect = fre_clust * pfs->csize;

			printf("Total Space: %lu KB, Free Space: %lu KB\r\n",
			       tot_sect * 4, fre_sect * 4);  // *4 потому что сектор 4096 байт
		}

		// Создание/запись тестового файла
		FR_Status = f_open(&Fil, "test.txt", FA_CREATE_ALWAYS | FA_WRITE);
		if (FR_Status != FR_OK) {
			sprintf(TxBuffer, "Error! Failed to create test file, Error Code: (%i)\r\n", FR_Status);
			printf("%s", TxBuffer);
		} else {
			printf("Test file created successfully\r\n");

			// Запись данных
			strcpy(RW_Buffer, "Hello from W25Q64FW Flash Memory!\r\nFatFS is working!\r\n");
			FR_Status = f_write(&Fil, RW_Buffer, strlen(RW_Buffer), &WWC);
			if (FR_Status != FR_OK) {
				sprintf(TxBuffer, "Error! Failed to write, Error Code: (%i)\r\n", FR_Status);
				printf("%s", TxBuffer);
			} else {
				printf("Data written: %d bytes\r\n", WWC);
			}

			// Закрытие файла
			f_close(&Fil);
			printf("File closed\r\n");
		}

		// Чтение тестового файла
		FR_Status = f_open(&Fil, "test.txt", FA_READ);
		if (FR_Status != FR_OK) {
			sprintf(TxBuffer, "Error! Failed to open test file for reading, Error Code: (%i)\r\n", FR_Status);
			printf("%s", TxBuffer);
		} else {
			printf("Test file opened for reading\r\n");

			// Чтение данных
			memset(RW_Buffer, 0, sizeof(RW_Buffer));
			FR_Status = f_read(&Fil, RW_Buffer, sizeof(RW_Buffer) - 1, &RWC);
			if (FR_Status != FR_OK) {
				sprintf(TxBuffer, "Error! Failed to read, Error Code: (%i)\r\n", FR_Status);
				printf("%s", TxBuffer);
			} else {
				printf("Data read: %d bytes\r\n", RWC);
				printf("Content: %s\r\n", RW_Buffer);
			}

			// Закрытие файла
			f_close(&Fil);
			printf("File closed\r\n");
		}

		// Создание директории для веб-файлов
		printf("\r\n=== Creating Web Directory ===\r\n");
		FR_Status = f_mkdir("web");
		if (FR_Status == FR_OK) {
			printf("Directory 'web' created successfully\r\n");
			add_fs_item("/web", FS_TYPE_FOLDER);
		} else if (FR_Status == FR_EXIST) {
			printf("Directory 'web' already exists\r\n");
		} else {
			sprintf(TxBuffer, "Error! Failed to create directory, Error Code: (%i)\r\n", FR_Status);
			printf("%s", TxBuffer);
		}

		// Создание директории для веб-файлов
		printf("\r\n=== Creating Firmware Directory ===\r\n");
		FR_Status = f_mkdir("firmware");
		if (FR_Status == FR_OK) {
			printf("Directory 'firmware' created successfully\r\n");
			add_fs_item("/firmware", FS_TYPE_FOLDER);
		} else if (FR_Status == FR_EXIST) {
			printf("Directory 'firmware' already exists\r\n");
		} else {
			sprintf(TxBuffer, "Error! Failed to create directory, Error Code: (%i)\r\n", FR_Status);
			printf("%s", TxBuffer);
		}

		printf("\r\n=== FatFS Initialization Complete ===\r\n");
	}

	// Основной цикл задачи
	while (1) {
		fx_thread_sleep(1000);

		// Здесь можно добавить периодические операции с файловой системой
		// например, обработку команд от веб-сервера
	}
}

