#include "spi_flash.h"

static spi_flash_rw_t rw_funcs;
static spi_flash_cs_t cs;

void fx_init_spi_flash(int (*r)(uint8_t*, uint16_t), 
					int (*w)(uint8_t*, uint16_t), 
					int (*rw)(uint8_t*, uint8_t*, uint16_t),
					int (*en)(), int (*dis)())
{
	rw_funcs.read = r;
	rw_funcs.write = w;
	rw_funcs.readwrite = rw;
	cs.enable = en;
	cs.disable = dis;
}

int fx_spi_flash_get_secsize()
{
	return SPI_FLASH_SEC_SIZE;
}

void fx_spi_flash_GetRDID(uint8_t* buf)
{
	uint8_t data[4] = {SPI_FLASH_CMD_RDID, 0, 0, 0};
	
	cs.enable();
	rw_funcs.write(data, sizeof(data));
	rw_funcs.read(buf, 2);
	cs.disable();

}

void fx_spi_flash_Reset (void)
{
	uint8_t cmd1[1] = {SPI_FLASH_CMD_Enable_Reset};
	uint8_t cmd2[1] = {SPI_FLASH_CMD_Reset};

	cs.enable();
	rw_funcs.write(cmd1, 1);
	cs.disable();
	cs.enable();
	rw_funcs.write(cmd2, 1);
	cs.disable(); 
}

void fx_spi_flash_GetUniqueId(uint8_t* buf)
{
	uint8_t data[5] = {SPI_FLASH_CMD_UNIQUE_ID, 0, 0, 0, 0};

	cs.enable();
	rw_funcs.write(data, sizeof(data));
	rw_funcs.read(buf, 8);

	cs.disable();
}

void fx_spi_flash_GetJEDECId(uint8_t* buf)
{
	cs.enable();

	uint8_t cmd = SPI_FLASH_CMD_JEDEC_ID;
	rw_funcs.write(&cmd, 1);
	rw_funcs.read(buf, 3);

	cs.disable();
}

void fx_spi_flash_get_info(flash_info_t* fi)
{
	uint8_t data[8] = {};
	fx_spi_flash_GetRDID(data);
	fi->mfr_id = data[0];
	fi->dev_id = data[1];
	//fx_spi_flash_GetUniqueId(fi->unique_id);
	fx_spi_flash_GetJEDECId(data);
	fi->capacity = data[1];
	fi->mem_type = data[2];
}

void fx_spi_write_enable()
{
	cs.enable();
	uint8_t cmd = SPI_FLASH_CMD_Write_Enable;
	rw_funcs.write(&cmd, 1);
	cs.disable();
}

void fx_spi_write_disable()
{
	cs.enable();
	uint8_t cmd = SPI_FLASH_CMD_Write_Disable;
	rw_funcs.write(&cmd, 1);
	cs.disable();
}

void fx_spi_Set_Block_Protect(uint8_t blkno)
{
    uint8_t cmd = 0x50;
    cs.enable();
    rw_funcs.write(&cmd, 1);
    cs.disable();
    uint8_t buf[2] = {SPI_FLASH_WRITE_STATUS_1, ((blkno & 0x0F) << 2)};
    cs.enable();
    rw_funcs.write(buf, 2);
    cs.disable();
}

void fx_spi_Wait_Write_End(void)
{
	fx_thread_sleep(10);

 uint8_t buf[1] = {SPI_FLASH_READ_STATUS_1};
 uint8_t status;


 //fx_spi_write_enable();

 //fx_spi_write_disable();
 do{
	 cs.enable();
	 rw_funcs.write(buf, 1);
	  rw_funcs.read(buf,1);
	  status = buf[0];
	  fx_thread_sleep(10);
	  cs.disable();
 }
 while((status & 0x01) == 0x01);

}

int fx_spi_Erase_Sector(fs_media_t* dev, uint32_t blkno)
{
  fx_spi_Wait_Write_End();
  
  uint8_t offset = blkno * 512;
  uint8_t buf[4] = {SPI_FLASH_SECTOR_ERASE, (offset >> 16) & 0xFF, (offset >> 8) & 0xFF, offset & 0xFF};
  
  fx_spi_Set_Block_Protect(0x00);
  fx_spi_write_enable();
  cs.enable();
  rw_funcs.write(buf, 4);
  cs.disable();
  fx_spi_Wait_Write_End();
  fx_spi_write_disable();
  fx_spi_Set_Block_Protect(0x0F);
  return 0;
}

int fx_spi_chip_erase()
{

	fx_spi_Wait_Write_End();
	fx_spi_write_enable();
	uint8_t buf[1] = {SPI_FLASH_CMD_Erase_Chip};
	

	cs.enable();
	rw_funcs.write(buf, 1);
	cs.disable();
	//fx_spi_write_disable();
	fx_spi_Wait_Write_End();

	return 0;
}

int fx_flash_read(fs_media_t* dev,void* buf, size_t* nbyte, uint32_t blkno)
{
    fx_spi_Wait_Write_End();
    
  uint32_t offset = SPI_FLASH_SEC_SIZE * blkno;
    uint8_t data[4] = {SPI_FLASH_CMD_Read, (offset >> 16) & 0xff, (offset >> 8) & 0xff, offset & 0xff};
    
  cs.enable();
    rw_funcs.write(data, 4);
    rw_funcs.read(buf, *nbyte);
    cs.disable();
    
  return 0;
}

int fx_flash_write(fs_media_t* dev, void* buf, size_t* nbyte, uint32_t blkno)
{

	fx_spi_write_enable();

	fx_spi_Wait_Write_End();
	//fx_spi_Erase_Sector(dev, blkno);

	uint32_t offset = SPI_FLASH_SEC_SIZE * blkno;
	  uint8_t data[4] = {SPI_FLASH_CMD_Write, (offset >> 16) & 0xff, (offset >> 8) & 0xff, offset & 0xff};
	  uint16_t data_size;
	  size_t full_size = *nbyte;

	  cs.enable();
	  rw_funcs.write(data, 4);
	  rw_funcs.write(buf, *nbyte);
	  cs.disable();


	  fx_spi_Wait_Write_End();
	  fx_spi_write_disable();
//	  //fx_spi_Set_Block_Protect(0x00);
//	  for (size_t i = 0; i < full_size; i += SPI_FLASH_PAGE_SIZE)
//	  {
//	    data_size = SPI_FLASH_PAGE_SIZE * (*nbyte >= SPI_FLASH_PAGE_SIZE) + *nbyte * (*nbyte < SPI_FLASH_PAGE_SIZE);
//
//
//
//	    cs.enable();
//	    rw_funcs.write(data, 4);
//	    cs.disable();
//	    cs.enable();
//	    rw_funcs.write(buf, data_size);
//	    cs.disable();
//
//
//
//	    offset += SPI_FLASH_PAGE_SIZE;
//	    data[1] = (offset >> 16) & 0xff;
//	    data[2] = (offset >> 8) & 0xff;
//	    data[3] =  offset & 0xff;
//	    buf += SPI_FLASH_PAGE_SIZE;
//	    *nbyte -= SPI_FLASH_PAGE_SIZE;
//
//	    //fx_spi_Wait_Write_End();
//
//	    fx_spi_write_disable();
//	  }
//	  //fx_spi_Set_Block_Protect(0x0F);

    return 0;
}
