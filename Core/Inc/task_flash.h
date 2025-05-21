/*
 * task_flash.h
 *
 *  Created on: Nov 7, 2024
 *      Author: {}
 */

#ifndef INC_TASK_FLASH_H_
#define INC_TASK_FLASH_H_

#include <fs_data.h>
#include "main.h"
#include "FXRTOS.h"
#include <stdlib.h>
#include "../Middleware/Fat/fx_file.h"
#include "../Middleware/Fat/fatfs.h"
#include "../Middleware/Fat/fs_media.h"
#include "../Middleware/Spi_Flash/spi_flash.h"
#include <stdio.h>

extern fs_vol_t volume;
extern fs_media_t m;
extern fs_dir_t dir;
extern fs_file_t file;


void Task_Flash_Init();

#endif /* INC_TASK_FLASH_H_ */
