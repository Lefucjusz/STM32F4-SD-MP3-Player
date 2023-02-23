/*
 * sd_spi_driver.h
 *
 *  Created on: Feb 22, 2023
 *      Author: lefucjusz
 */

#ifndef TARGET_SD_SPI_DRIVER_H_
#define TARGET_SD_SPI_DRIVER_H_

#include "integer.h"
#include "diskio.h"
#include "ff_gen_drv.h"

DSTATUS sd_spi_driver_init(BYTE pdrv);
DSTATUS sd_spi_driver_status(BYTE pdrv);
DRESULT sd_spi_driver_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count);

#if _USE_WRITE == 1
DRESULT sd_spi_driver_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif

#if _USE_IOCTL == 1
DRESULT sd_spi_driver_ioctl(BYTE pdrv, BYTE cmd, void *buff);
#endif

#endif /* TARGET_SD_SPI_DRIVER_H_ */
