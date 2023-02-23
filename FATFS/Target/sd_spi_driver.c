/*
 * sd_spi_driver.c
 *
 *  Created on: Feb 22, 2023
 *      Author: lefucjusz
 */
#include "sd_spi_driver.h"
#include "stm32f4xx_hal.h"

/* MMC/SD command */
#define CMD0	(0)			/* GO_IDLE_STATE */
#define CMD1	(1)			/* SEND_OP_COND (MMC) */
#define	ACMD41	(0x80 | 41)	/* SEND_OP_COND (SDC) */
#define CMD8	(8)			/* SEND_IF_COND */
#define CMD9	(9)			/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define ACMD13	(0x80 | 13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT (MMC) */
#define	ACMD23	(0x80 | 23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD32	(32)		/* ERASE_ER_BLK_START */
#define CMD33	(33)		/* ERASE_ER_BLK_END */
#define CMD38	(38)		/* ERASE */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */

/* MMC card type flags (MMC_GET_TYPE) */
#define TYPE_MMC	0x01					/* MMC ver. 3 */
#define TYPE_SD1	0x02					/* SD ver. 1 */
#define TYPE_SD2	0x04					/* SD ver. 2 */
#define TYPE_SDC	(TYPE_SD1 | TYPE_SD2)	/* SD */
#define TYPE_BLOCK	0x08					/* Block addressing */

/* Block size */
#define MMC_BLOCK_SIZE 512

/* HAL SPI timeout */
#define SPI_TIMEOUT 100 //ms

/* Handle to SPI driver, defined in main.h */
extern SPI_HandleTypeDef SD_SPI_HANDLE;

/* SPI clock speed enum */
typedef enum {
	SPI_CLOCK_SLOW,
	SPI_CLOCK_FAST
} spi_clock_t;

/* Driver context */
typedef struct {
	volatile DSTATUS status;
	uint8_t card_type;
} spi_driver_t;

static spi_driver_t ctx;

/* Timer control functions */
typedef struct {
	uint32_t start_tick;
	uint32_t delay_ms;
} spi_timer_t;

static void timer_start(spi_timer_t *timer, uint32_t delay_ms) {
	timer->start_tick = HAL_GetTick();
	timer->delay_ms = delay_ms;
}

static int timer_timeout(spi_timer_t *timer) {
	if ((HAL_GetTick() - timer->start_tick) >= timer->delay_ms) {
		return 1;
	}
	return 0;
}

/* SPI control functions */
static void spi_set_clock(spi_clock_t clock_speed) {
	switch (clock_speed) {
		case SPI_CLOCK_SLOW:
			MODIFY_REG(SD_SPI_HANDLE.Instance->CR1, SPI_BAUDRATEPRESCALER_256, SPI_BAUDRATEPRESCALER_128); // ~211kHz clock
			break;

		case SPI_CLOCK_FAST:
			MODIFY_REG(SD_SPI_HANDLE.Instance->CR1, SPI_BAUDRATEPRESCALER_256, SPI_BAUDRATEPRESCALER_2); // ~13.5MHz clock
			break;

		default:
			break;
	}
}

static uint8_t spi_exchange_byte(uint8_t byte_to_send) {
	uint8_t byte_received;
	HAL_SPI_TransmitReceive(&SD_SPI_HANDLE, &byte_to_send, &byte_received, sizeof(uint8_t), SPI_TIMEOUT);
	return byte_received;
}

#if _USE_WRITE == 1
static void spi_transmit(const uint8_t *buffer, size_t size) {
	for (size_t i = 0; i < size; ++i) {
		spi_exchange_byte(buffer[i]);
	}
}
#endif

static void spi_receive(uint8_t *buffer, size_t size) {
	for (size_t i = 0; i < size; ++i) {
		buffer[i] = spi_exchange_byte(0xFF);
	}
}

static int spi_ready(uint32_t timeout) {
	spi_timer_t timer;
	uint8_t byte;

	timer_start(&timer, timeout);

	while (timer_timeout(&timer) == 0) {
		byte = spi_exchange_byte(0xFF);
		if (byte == 0xFF) {
			return 1;
		}
	}

	return 0;
}

static void spi_deselect(void) {
	HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
	spi_exchange_byte(0xFF); // Dummy clock (force DO HI-Z)
}


static int spi_select(void) {
	HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);
	spi_exchange_byte(0xFF); // Dummy clock (force DO enabled)

	/* Wait until card ready - 500ms */
	if (spi_ready(500) != 0) {
		return 1;
	}

	/* Timeout */
	spi_deselect();
	return 0;
}

/* MMC control functions */
static int mmc_receive_block(uint8_t *buffer, size_t block_size) {
	spi_timer_t timer;
	uint8_t token;

	timer_start(&timer, 200); // 200ms timeout

	while (timer_timeout(&timer) == 0) {
		token = spi_exchange_byte(0xFF);
		if (token != 0xFF) {
			break;
		}
	}

	/* Invalid DataStart token or timeout */
	if (token != 0xFE) {
		return 0;
	}

	/* Receive data */
	spi_receive(buffer, block_size);

	/* Discard CRC */
	spi_exchange_byte(0xFF);
	spi_exchange_byte(0xFF);

	return 1;
}

#if _USE_WRITE == 1
static int mmc_transmit_block(const uint8_t *buffer, uint8_t token) {
	uint8_t result;

	/* Wait until card ready - 500ms */
	if (spi_ready(500) == 0) {
		return 0;
	}

	/* Send token */
	spi_exchange_byte(token);

	/* Send data if token is other than StopTran */
	if (token != 0xFD) {
		spi_transmit(buffer, MMC_BLOCK_SIZE);

		/* Dummy CRC */
		spi_exchange_byte(0xFF);
		spi_exchange_byte(0xFF);

		/* Check if data accepted */
		result = spi_exchange_byte(0xFF);
		if ((result & 0x1F) != 0x05) {
			return 0;
		}
	}

	return 1;
}
#endif

static uint8_t mmc_transmit_command(uint8_t command, uint32_t argument) {
	uint8_t response_bytes_count;
	uint8_t result, crc;

	/* Send CMD55 prior to ACMDn */
	if (command & 0x80) {
		command &= ~0x80;
		result = mmc_transmit_command(CMD55, 0);
		if (result > 1) {
			return result;
		}
	}

	/* If command is not to stop multiple block read - select the card and wait for ready */
	if (command != CMD12) {
		spi_deselect();
		result = spi_select();
		if (result == 0) {
			return 0xFF;
		}
	}

	/* Get command CRC */
	switch (command) {
		case CMD0:
			crc = 0x95; // Valid CRC for CMD0
			break;
		case CMD8:
			crc = 0x87; // Valid CRC for CMD8
			break;
		default:
			crc = 0x01; // Dummy CRC + Stop
			break;
	}

	/* Send command packet */
	spi_exchange_byte(command | 0x40); // Command index + Start
	spi_exchange_byte((argument >> 24) & 0xFF); // Bits 31-24
	spi_exchange_byte((argument >> 16) & 0xFF); // Bits 23-16
	spi_exchange_byte((argument >> 8) & 0xFF); 	// Bits 15-8
	spi_exchange_byte((argument >> 0) & 0xFF); 	// Bits 7-0
	spi_exchange_byte(crc); // CRC

	/* Get response */
	if (command == CMD12) {
		spi_exchange_byte(0xFF); // Discard one byte
	}
	response_bytes_count = 10; // Max. 10 bytes of response

	do {
		result = spi_exchange_byte(0xFF);
	} while ((result & 0x80) && (--response_bytes_count));

	return result;
}

static void mmc_read_ocr(uint8_t *ocr) {
	for (size_t i = 0; i < 4; ++i) {
		ocr[i] = spi_exchange_byte(0xFF);
	}
}

/* API functions */
DSTATUS sd_spi_driver_init(BYTE pdrv) {
	uint8_t ocr[4];
	uint8_t command;

	/* Only drive 0 supported */
	if (pdrv > 0) {
		return STA_NOINIT;
	}

	/* No card in socket */
	if (ctx.status & STA_NODISK) {
		return ctx.status;
	}

	/* Set slow clock for init and send 80 dummy clocks to wake up card */
	spi_set_clock(SPI_CLOCK_SLOW);
	for (size_t i = 0; i < 10; ++i) {
		spi_exchange_byte(0xFF);
	}

	/* Initialize card */
	if (mmc_transmit_command(CMD0, 0) == 1) {
		spi_timer_t timer;
		timer_start(&timer, 1000); // 1 second timeout for initialization

		/* Check if SDv2 */
		if (mmc_transmit_command(CMD8, 0x1AA) == 1) {
			/* SDv2 card */
			mmc_read_ocr(ocr); // Get R7 response

			/* Check if the card supports VCC 2.7-3.6V */
			if ((ocr[2] == 0x01) && (ocr[3] == 0xAA)) {
				/* Wait for end of init with ACMD41(HCS) */
				while ((timer_timeout(&timer) == 0) && (mmc_transmit_command(ACMD41, (1UL << 30)) != 0));

				/* Check CCS bit in OCR */
				if ((timer_timeout(&timer) == 0) && (mmc_transmit_command(CMD58, 0) == 0)) {
					ctx.card_type = TYPE_SD2;
					mmc_read_ocr(ocr);
					if (ocr[0] & 0x40) {
						ctx.card_type |= TYPE_BLOCK;
					}
				}
			}
		}
		/* Not SDv2 card */
		else {
			/* SDv1(ACMD41(0)) */
			if (mmc_transmit_command(ACMD41, 0) <= 1) {
				ctx.card_type = TYPE_SD1;
				command = ACMD41;
			}
			/* MMCv3 (CMD1(0)) */
			else {
				ctx.card_type = TYPE_MMC;
				command = CMD1;
			}

			/* Wait for end of init */
			while ((timer_timeout(&timer) == 0) && (mmc_transmit_command(command, 0) != 0));

			/* Set block length */
			if ((timer_timeout(&timer) != 0) || (mmc_transmit_command(CMD16, MMC_BLOCK_SIZE) != 0)) {
				ctx.card_type = 0;
			}
		}
	}
	spi_deselect();

	if (ctx.card_type > 0) {
		spi_set_clock(SPI_CLOCK_FAST);
		ctx.status &= ~STA_NOINIT;
	}
	else {
		ctx.status = STA_NOINIT;
	}
	return ctx.status;
}

DSTATUS sd_spi_driver_status(BYTE pdrv) {
	if (pdrv > 0) {
		return STA_NOINIT;
	}
	return ctx.status;
}

DRESULT sd_spi_driver_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count) {
	/* Sanity check */
	if ((pdrv > 0) || (count == 0)) {
		return RES_PARERR;
	}

	/* Check if initialized */
	if (ctx.status & STA_NOINIT) {
		return RES_NOTRDY;
	}

	/* Convert LBA to BA addressing for byte-addressed cards */
	if (!(ctx.card_type & TYPE_BLOCK)) {
		sector *= MMC_BLOCK_SIZE;
	}

	/* Single sector - READ_SINGLE_BLOCK command */
	if (count == 1) {
		if ((mmc_transmit_command(CMD17, sector) == 0) && (mmc_receive_block(buff, MMC_BLOCK_SIZE) != 0)) {
			count = 0;
		}
	}
	else {
		/* Multiple sectors - READ_MULTIPLE_BLOCK command */
		if (mmc_transmit_command(CMD18, sector) == 0) {
			while (count > 0) {
				if (mmc_receive_block(buff, MMC_BLOCK_SIZE) == 0) {
					break;
				}
				buff += MMC_BLOCK_SIZE;
				--count;
			}
			mmc_transmit_command(CMD12, 0); // STOP_TRANSMISSION
		}
	}
	spi_deselect();

	return (count == 0) ? RES_OK : RES_ERROR;
}

#if _USE_WRITE == 1
DRESULT sd_spi_driver_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count) {
	/* Sanity check */
	if ((pdrv > 0) || (count == 0)) {
		return RES_PARERR;
	}

	/* Check if initialized */
	if (ctx.status & STA_NOINIT) {
		return RES_NOTRDY;
	}

	/* Check if write protected */
	if (ctx.status & STA_PROTECT) {
		return RES_WRPRT;
	}

	/* Convert LBA to BA addressing for byte-addressed cards */
	if (!(ctx.card_type & TYPE_BLOCK)) {
		sector *= MMC_BLOCK_SIZE;
	}

	/* Single sector - WRITE_BLOCK command */
	if (count == 1) {
		if ((mmc_transmit_command(CMD24, sector) == 0) && (mmc_transmit_block(buff, 0xFE) != 0)) {
			count = 0;
		}
	}
	/* Multiple sectors */
	else {
		if (ctx.card_type & TYPE_SDC) {
			mmc_transmit_command(ACMD23, count); // Predefine number of sectors
		}

		/* WRITE_MULTIPLE_BLOCK command */
		if (mmc_transmit_command(CMD25, sector) == 0) {
			while (count > 0) {
				if (mmc_transmit_block(buff, 0xFC) == 0) {
					break;
				}
				buff += MMC_BLOCK_SIZE;
				--count;
			}

			/* STOP_TRAN token */
			if (mmc_transmit_block(NULL, 0xFD) == 0) {
				count = 1;
			}
		}
	}
	spi_deselect();

	return (count == 0) ? RES_OK : RES_ERROR;
}
#endif

#if _USE_IOCTL == 1
DRESULT sd_spi_driver_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
	DRESULT result;
	uint8_t csd[16];
	uint32_t n, csize;

	/* Sanity check */
	if (pdrv > 0) {
		return RES_PARERR;
	}

	/* Check if initialized */
	if (ctx.status & STA_NOINIT) {
		return RES_NOTRDY;
	}

	result = RES_ERROR;

	switch (cmd) {
	 	/* Wait for end of internal write process of the drive */
		case CTRL_SYNC :
			if (spi_select() != 0) {
				result = RES_OK;
			}
			break;

		/* Get drive capacity in unit of sector (DWORD) */
		case GET_SECTOR_COUNT:
			if ((mmc_transmit_command(CMD9, 0) == 0) && (mmc_receive_block(csd, sizeof(csd)) != 0)) {
				/* SDv2 */
				if ((csd[0] >> 6) == 1) {
					csize = csd[9] + ((uint16_t)csd[8] << 8) + ((uint32_t)(csd[7] & 63) << 16) + 1;
					*(uint32_t *)buff = csize << 10;
				}
				/* SDv1 or MMCv3 */
				else {
					n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
					csize = (csd[8] >> 6) + ((uint16_t)csd[7] << 2) + ((uint16_t)(csd[6] & 3) << 10) + 1;
					*(uint32_t *)buff = (uint32_t)csize << (n - 9);
				}
				result = RES_OK;
			}
			break;

		case GET_SECTOR_SIZE:
			*(uint16_t *)buff = MMC_BLOCK_SIZE;
			result = RES_OK;
			break;

		default:
			result = RES_PARERR;
	}

	spi_deselect();
	return result;
}
#endif
