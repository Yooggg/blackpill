#include "task_flash.h"

extern SPI_HandleTypeDef hspi1;
fx_mutex_t mutex1;

fs_vol_t volume;
fs_media_t m;
fs_dir_t dir;
fs_file_t file;

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
void Task_Flash_Func()
{

	uint8_t sec_buf[SEC_SIZE];
//	fs_vol_t volume;
//	fs_media_t m;
	flash_info_t fi;
    m.read = fx_read;
    m.write = fx_write;
    m.sector_erase=fx_erase;
    volume.media = &m;

    fx_init_spi_flash(read_spi, write_spi, rw_spi, cs_en, cs_dis);

	fx_spi_flash_Reset();
	fx_spi_chip_erase();

	fx_spi_flash_get_info(&fi);
	fatfs_format(&m, 512, sec_buf);

	printf("media: %p, volume: %p\n", (void *)&m, (void *)&volume);
	fs_volume_open(&m, &volume, 0, fs_mem_alloc);
//	fs_dir_t dir;
	fs_dir_create(&volume, "/dir1");
	printf("Directory created");
	fx_thread_sleep(200);
	fs_dir_open(&volume, "/dir1", &dir);
//	char* filename = "/dir1/file.txt";
//	fs_file_create(&volume, filename);
//	//fs_file_t file;
//	char data_w[2] = "ab";
//	char data_r[2];
//	fs_file_open(&volume, &file, filename, 0);
//	size_t size = 0;
//	fs_file_write(&file, data_w, 2, &size);
//	fs_file_open(&volume, &file, filename, 0);
//	fs_file_read(&file, data_r, 2, &size);

	while (1)
	{
		fx_thread_sleep(200);
	}
}
