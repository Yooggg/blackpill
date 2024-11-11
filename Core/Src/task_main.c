#include <stdio.h>

#include "main.h"
#include "task_main.h"
#include "FXRTOS.h"

extern SPI_HandleTypeDef hspi1;

void Task_Main_Func();

void Task_Main_Init()
{
	static fx_thread_t task;
	static uint32_t stack[1024 / sizeof(uint32_t)];

//    trace_set_thread_log_lvl(&(task.trace_handle), THREAD_LOG_LVL_DEFAULT | TRACE_THREAD_PRIO_KEY | TRACE_THREAD_EVENT_KEY /*| TRACE_THREAD_CS_ONLY_KEY*/);

	fx_thread_init(&task, Task_Main_Func, NULL, 11, (void*)stack, sizeof(stack), false);
}

///*
// * Отправляет команду 1 байт и адрес 3 байта
// */
//void fx_spi_flash_SendCmdA(uint8_t cmd, uint32_t addr)
//{
//	uint8_t data[5] = {cmd, (uint8_t)((addr >> 16) & 0xFF), (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF)};
//	HAL_SPI_Transmit(&hspi2, data, sizeof(data), HAL_MAX_DELAY);
//}
//
//void fx_spi_flash_Read(uint8_t *pBuf, uint32_t count)
//{
//	HAL_SPI_Receive(&hspi2, pBuf, count, HAL_MAX_DELAY);
//}
//
//uint16_t fx_spi_flash_GetId()
//{
//	uint8_t data[2] = {0, 0};
//
//	HAL_GPIO_WritePin(SPI2_nCS_GPIO_Port, SPI2_nCS_Pin, GPIO_PIN_RESET);
//
//	fx_spi_flash_SendCmdA(SPI_FLASH_CMD_RDID, 0);
//	fx_spi_flash_Read(data, 2);
//
//	HAL_GPIO_WritePin(SPI2_nCS_GPIO_Port, SPI2_nCS_Pin, GPIO_PIN_SET);
//
//	return ((uint16_t)data[0] << 8) | ((uint16_t)data[1]);
//}
//
//void fx_spi_flash_TestRead()
//{
//	uint8_t data[2] = {0, 0};
//
//	HAL_GPIO_WritePin(SPI2_nCS_GPIO_Port, SPI2_nCS_Pin, GPIO_PIN_RESET);
//
//	fx_spi_flash_SendCmdA(SPI_FLASH_CMD_Read, 0);
//	fx_spi_flash_Read(data, 2);
//
//	HAL_GPIO_WritePin(SPI2_nCS_GPIO_Port, SPI2_nCS_Pin, GPIO_PIN_SET);
//}

void Task_Main_Func()
{
	while (1)
	{
		HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
		fx_thread_sleep(200);

		/*
		uint16_t id = fx_spi_flash_GetId();

		printf("RDID: 0x%4X\n", id);
		 */

		HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
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

