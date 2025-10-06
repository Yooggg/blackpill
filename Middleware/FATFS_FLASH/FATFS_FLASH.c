/*
 * File: FATFS_FLASH.c (512-byte sector version)
 * Driver Name: [[ FATFS_FLASH SPI ]]
 * SW Layer:   MIDDLEWARE
 * -------------------------------------------
 * FatFS driver для W25Q64FW с виртуальными секторами 512 байт
 *
 * Физический сектор flash = 4096 байт
 * Виртуальный сектор FatFS = 512 байт
 * Соотношение: 1 физический = 8 виртуальных секторов
 */

#include "FATFS_FLASH.h"
#include "../Spi_Flash/spi_flash.h"
#include <string.h>

// Статус диска
static volatile DSTATUS Stat = STA_NOINIT;

// Флаг инициализации
static uint8_t initialized = 0;

// Буфер для кэширования физического сектора (4096 байт)
static uint8_t sector_cache[4096];
static uint32_t cached_physical_sector = 0xFFFFFFFF;
static uint8_t cache_dirty = 0;

// Вспомогательные макросы
#define PHYSICAL_SECTOR_SIZE  4096
#define VIRTUAL_SECTOR_SIZE   512
#define SECTORS_PER_BLOCK     8    // 4096 / 512 = 8

/*-----------------------------------------------------------------------*/
/* Сброс кэша на flash                                                   */
/*-----------------------------------------------------------------------*/
static DRESULT flush_cache(void)
{
    if (cache_dirty && cached_physical_sector != 0xFFFFFFFF) {
        uint32_t size = PHYSICAL_SECTOR_SIZE;
        int result = fx_flash_write(sector_cache, &size, cached_physical_sector);

        if (result != 0) {
            return RES_ERROR;
        }

        cache_dirty = 0;
    }

    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Загрузка физического сектора в кэш                                    */
/*-----------------------------------------------------------------------*/
static DRESULT load_cache(uint32_t physical_sector)
{
    // Если этот сектор уже в кэше, ничего не делаем
    if (cached_physical_sector == physical_sector) {
        return RES_OK;
    }

    // Сбросить текущий кэш если он грязный
    DRESULT res = flush_cache();
    if (res != RES_OK) {
        return res;
    }

    // Загрузить новый сектор
    uint32_t size = PHYSICAL_SECTOR_SIZE;
    int result = fx_flash_read(sector_cache, &size, physical_sector);

    if (result != 0) {
        return RES_ERROR;
    }

    cached_physical_sector = physical_sector;
    cache_dirty = 0;

    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Инициализация SPI Flash драйвера                                      */
/*-----------------------------------------------------------------------*/
DSTATUS FLASH_disk_initialize(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;

    if (!initialized) {
        // Инвалидировать кэш
        cached_physical_sector = 0xFFFFFFFF;
        cache_dirty = 0;

        initialized = 1;
        Stat &= ~STA_NOINIT;
    }

    return Stat;
}

/*-----------------------------------------------------------------------*/
/* Получение статуса диска                                               */
/*-----------------------------------------------------------------------*/
DSTATUS FLASH_disk_status(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;
    return Stat;
}

/*-----------------------------------------------------------------------*/
/* Чтение виртуальных секторов (512 байт)                                */
/*-----------------------------------------------------------------------*/
DRESULT FLASH_disk_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count)
{
    if (pdrv != 0 || !count) return RES_PARERR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    // Проверка границ
    if (sector + count > FLASH_SECTOR_COUNT) return RES_PARERR;

    DRESULT res;

    // Читаем count виртуальных секторов
    for (UINT i = 0; i < count; i++) {
        DWORD virtual_sector = sector + i;

        // Вычислить физический сектор и смещение внутри него
        uint32_t physical_sector = virtual_sector / SECTORS_PER_BLOCK;
        uint32_t offset = (virtual_sector % SECTORS_PER_BLOCK) * VIRTUAL_SECTOR_SIZE;

        // Загрузить физический сектор в кэш
        res = load_cache(physical_sector);
        if (res != RES_OK) {
            return res;
        }

        // Скопировать данные из кэша
        memcpy(buff, sector_cache + offset, VIRTUAL_SECTOR_SIZE);
        buff += VIRTUAL_SECTOR_SIZE;
    }

    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Запись виртуальных секторов (512 байт)                                */
/*-----------------------------------------------------------------------*/
#if _USE_WRITE == 1
DRESULT FLASH_disk_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count)
{
    if (pdrv != 0 || !count) return RES_PARERR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    // Проверка границ
    if (sector + count > FLASH_SECTOR_COUNT) return RES_PARERR;

    DRESULT res;

    // Записываем count виртуальных секторов
    for (UINT i = 0; i < count; i++) {
        DWORD virtual_sector = sector + i;

        // Вычислить физический сектор и смещение
        uint32_t physical_sector = virtual_sector / SECTORS_PER_BLOCK;
        uint32_t offset = (virtual_sector % SECTORS_PER_BLOCK) * VIRTUAL_SECTOR_SIZE;

        // Загрузить физический сектор в кэш (если это новый сектор)
        res = load_cache(physical_sector);
        if (res != RES_OK) {
            return res;
        }

        // Обновить данные в кэше
        memcpy(sector_cache + offset, buff, VIRTUAL_SECTOR_SIZE);
        cache_dirty = 1;
        buff += VIRTUAL_SECTOR_SIZE;

        // Если это последний виртуальный сектор в физическом блоке,
        // или это последний сектор в записи - сбросить кэш
        uint32_t next_virtual = virtual_sector + 1;
        uint32_t next_physical = next_virtual / SECTORS_PER_BLOCK;

        if (next_physical != physical_sector || i == count - 1) {
            res = flush_cache();
            if (res != RES_OK) {
                return res;
            }
        }
    }

    return RES_OK;
}
#endif

/*-----------------------------------------------------------------------*/
/* Команды управления (IOCTL)                                            */
/*-----------------------------------------------------------------------*/
#if _USE_IOCTL == 1
DRESULT FLASH_disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
    DRESULT res = RES_ERROR;

    if (pdrv != 0) return RES_PARERR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    switch (cmd) {
        case CTRL_SYNC:
            // Сбросить кэш
            res = flush_cache();
            break;

        case GET_SECTOR_COUNT:
            // Возвращаем количество ВИРТУАЛЬНЫХ секторов
            *(DWORD*)buff = FLASH_SECTOR_COUNT;
            res = RES_OK;
            break;

        case GET_SECTOR_SIZE:
            // Возвращаем размер ВИРТУАЛЬНОГО сектора
            *(WORD*)buff = VIRTUAL_SECTOR_SIZE;
            res = RES_OK;
            break;

        case GET_BLOCK_SIZE:
            // Возвращаем размер блока стирания в виртуальных секторах
            *(DWORD*)buff = FLASH_BLOCK_SIZE;
            res = RES_OK;
            break;

        case CTRL_TRIM:
            res = RES_OK;
            break;

        default:
            res = RES_PARERR;
    }

    return res;
}
#endif
