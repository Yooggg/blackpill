/*
 * task_flash.h
 *
 *  Created on: Nov 7, 2024
 *      Author: {}
 */

#ifndef INC_TASK_FLASH_H_
#define INC_TASK_FLASH_H_

#include "main.h"
#include "FXRTOS.h"
#include <stdlib.h>
#include "../Middleware/Fat/fx_file.h"
#include "../Middleware/Fat/fatfs.h"
#include "../Middleware/Fat/fs_media.h"
#include "../Middleware/Spi_Flash/spi_flash.h"
#include <stdio.h>

void Task_Flash_Init();

#endif /* INC_TASK_FLASH_H_ */
