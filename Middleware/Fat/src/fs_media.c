#include "fs_media.h"
#include "spi_flash.h"

void fs_media_lock(fs_media_t* media){}
void fs_media_unlock(fs_media_t* media){}



// int read(_fs_media_t* dev, void* buf, size_t* len, uint32_t sector){
//     // Проверка, что длина буфера соответствует размеру сектора
//     if (*len != SEC_SIZE)
//     {
//         return -1;  // Ошибка: длина данных не соответствует размеру сектора
//     }

//     // Ваша низкоуровневая операция чтения данных из сектора устройства
//     // Например, если это SD-карта, то здесь вызов библиотеки работы с SD-картой
//     int result = fx_flash_read(buf, len, sector);

//     if (result != 0)
//     {
//         return -1;  // Ошибка чтения
//     }

//     return 0;  // Успех

// }

// int write(fs_media_t* dev, void* buf, size_t* len, uint32_t sector){
//         // Проверка, что длина равна размеру сектора
//     if (*len != SEC_SIZE)
//     {
//         return -1;  // Ошибка: длина данных не соответствует размеру сектора
//     }

//     // Ваша низкоуровневая операция записи данных в указанный сектор устройства
//     // Например, если это SD-карта, то здесь вызов библиотеки работы с SD-картой
//     int result = fx_flash_write(buf, len, sector);

//     if (result != 0)
//     {
//         return -1;  // Ошибка записи
//     }

//     return 0;  // Успех
// }

// int sector_erase(_fs_media_t* dev, uint32_t sector){
//     uint8_t buf[SEC_SIZE];   // Буфер для данных сектора
//     size_t len = SEC_SIZE;   // Длина данных (размер сектора)

//     // Заполнить буфер нулями для "стирания"
//     memset(buf, 0, SEC_SIZE);

//     // Проверка указателя на запись
//     // if (dev->write == NULL)
//     // {
//     //     return -1;  // Ошибка: указатель на функцию записи не задан
//     // }

//     // Выполняем запись буфера в указанный сектор
//     int result = dev->write(dev, buf, &len, sector);

//     if (result != 0 || len != SEC_SIZE)
//     {
//         return -1;  // Ошибка при записи сектора
//     }

//     return 0;  // Успешная операция
// }
