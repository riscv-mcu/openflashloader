/***************************************************************************
 *   Copyright (C) 2010 by Antonio Borneo <borneo.antonio@gmail.com>       *
 *   Modified by Yanwen Wang <wangyanwen@nucleisys.com> based on fespi.c   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

/* The Nuclei SPI controller is a SPI bus controller
 * specifically designed for SPI Flash Memories on Nuclei RISC-V platforms.
 *
 * Two working modes are available:
 * - SW mode: the SPI is controlled by SW. Any custom commands can be sent
 *   on the bus. Writes are only possible in this mode.
 * - HW mode: Memory content is directly
 *   accessible in CPU memory space. CPU can read and execute memory content.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*==== Loader ====*/
#define ERASE_CMD			(1)
#define WRITE_CMD			(2)
#define READ_CMD			(3)
#define PROBE_CMD			(4)

/*==== NUSPI ====*/
/* Register offsets */
#define NUSPI_REG_SCKDIV			(0x00)
#define NUSPI_REG_SCKMODE			(0x04)
#define NUSPI_REG_SCKSAMPLE			(0x08)
#define NUSPI_REG_FORCE				(0x0C)
#define NUSPI_REG_CSID				(0x10)
#define NUSPI_REG_CSDEF				(0x14)
#define NUSPI_REG_CSMODE			(0x18)
#define NUSPI_REG_VERSION			(0x1C)
#define NUSPI_REG_DCSSCK			(0x28)
#define NUSPI_REG_DSCKCS			(0x2a)
#define NUSPI_REG_DINTERCS			(0x2c)
#define NUSPI_REG_DINTERXFR			(0x2e)
#define NUSPI_REG_FMT				(0x40)
#define NUSPI_REG_TXDATA			(0x48)
#define NUSPI_REG_RXDATA			(0x4C)
#define NUSPI_REG_TXMARK			(0x50)
#define NUSPI_REG_RXMARK			(0x54)
#define NUSPI_REG_FCTRL				(0x60)
#define NUSPI_REG_FFMT				(0x64)
#define NUSPI_REG_IE				(0x70)
#define NUSPI_REG_IP				(0x74)
#define NUSPI_REG_FFMT1				(0x78)
#define NUSPI_REG_STATUS			(0x7C)
#define NUSPI_REG_RXEDGE			(0x80)
#define NUSPI_REG_CR				(0x84)
/* Fields */
#define NUSPI_SCK_POL				(0x1)
#define NUSPI_SCK_PHA				(0x2)
/* FMT register */
#define NUSPI_FMT_PROTO(x)			((x) & 0x3)
#define NUSPI_FMT_ENDIAN(x)			(((x) & 0x1) << 2)
#define NUSPI_FMT_DIR(x)			(((x) & 0x1) << 3)
#define NUSPI_FMT_LEN(x)			(((x) & 0xf) << 16)//TODO:
/* TXMARK register */
#define NUSPI_TXWM(x)				((x) & 0xFFFF)//TODO:
/* RXMARK register */
#define NUSPI_RXWM(x)				((x) & 0xFFFF)//TODO:
/* IP register */
#define NUSPI_IP_TXWM				(0x1)
#define NUSPI_IP_RXWM				(0x2)
/* FCTRL register */
#define NUSPI_FCTRL_EN				(0x1)
#define NUSPI_INSN_CMD_EN			(0x1)
#define NUSPI_INSN_ADDR_LEN(x)		(((x) & 0x7) << 1)
#define NUSPI_INSN_PAD_CNT(x)		(((x) & 0xf) << 4)
#define NUSPI_INSN_CMD_PROTO(x)		(((x) & 0x3) << 8)
#define NUSPI_INSN_ADDR_PROTO(x)	(((x) & 0x3) << 10)
#define NUSPI_INSN_DATA_PROTO(x)	(((x) & 0x3) << 12)
#define NUSPI_INSN_CMD_CODE(x)		(((x) & 0xff) << 16)
#define NUSPI_INSN_PAD_CODE(x)		(((x) & 0xff) << 24)
/* STATUS register */
#define NUSPI_STAT_BUSY				(0x1 << 0)
#define NUSPI_STAT_TXFULL			(0x1 << 4)
#define NUSPI_STAT_RXEMPTY			(0x1 << 5)
#define NUSPI_CSMODE_AUTO			(0)
#define NUSPI_CSMODE_HOLD			(2)
#define NUSPI_CSMODE_OFF			(3)
#define NUSPI_DIR_RX				(0)
#define NUSPI_DIR_TX				(1)
#define NUSPI_PROTO_S				(0)
#define NUSPI_PROTO_D				(1)
#define NUSPI_PROTO_Q				(2)
#define NUSPI_ENDIAN_MSB			(0)
#define NUSPI_ENDIAN_LSB			(1)

/*==== FLASH ====*/
#define	SPIFLASH_BSY		0
#define SPIFLASH_BSY_BIT	(1 << SPIFLASH_BSY)	/* WIP Bit of SPI SR */
/* SPI Flash Commands */
#define SPIFLASH_ENABLE_RESET	0x66
#define SPIFLASH_RESET_DEVICE	0x99
#define SPIFLASH_WRITE_STATUS1	0x01
#define SPIFLASH_READ_ID		0x9F /* Read Flash Identification */
#define SPIFLASH_READ_STATUS	0x05 /* Read Status Register */
#define SPIFLASH_WRITE_ENABLE	0x06 /* Write Enable */
#define SPIFLASH_PAGE_PROGRAM	0x02 /* Page Program */
#define SPIFLASH_PAGE_SIZE		0x100
#define SPIFLASH_READ			0x03 /* Normal Read */
#define SPIFLASH_SECTOR_ERASE	0x20 /* Sector Erase */
#define SPIFLASH_SECTOR_SIZE	0x1000
#define SPIFLASH_BLOCK_ERASE	0xD8 /* Block Erase */
#define SPIFLASH_BLOCK_SIZE		0x10000

#define NUSPI_OK						(0x0)
#define NUSPI_TXWM_ERR					(0x1 << 0)
#define NUSPI_TX_ERR					(0x1 << 1)
#define NUSPI_RX_ERR					(0x1 << 2)
#define FLASH_TX_ERR					(0x1 << 3)
#define FLASH_RX_ERR					(0x1 << 4)
#define FLASH_WIP_ERR					(0x1 << 5)
#define FLASH_INIT_ERR					(0x1 << 6)
#define FLASH_ERASE_ERR					(0x1 << 7)
#define FLASH_WRITE_ERR					(0x1 << 8)
#define FLASH_READ_ERR					(0x1 << 9)

#define NUSPI_TXWM_TIMEOUT			(5000)
#define NUSPI_TX_TIMEOUT			(5000)
#define NUSPI_RX_TIMEOUT			(5000)

static inline void nuspi_read_reg(uint32_t *spi_base, uint32_t offset, uint32_t *value);
static inline void nuspi_write_reg(uint32_t *spi_base, uint32_t offset, uint32_t value);
static void nuspi_disable_hw_mode(uint32_t *spi_base);
static void nuspi_enable_hw_mode(uint32_t *spi_base);
static void nuspi_set_dir(uint32_t *spi_base, uint32_t dir);
static int nuspi_txwm_wait(uint32_t *spi_base);
static int nuspi_tx(uint32_t *spi_base, uint8_t in);
static int nuspi_rx(uint32_t *spi_base, uint8_t *out);

static int flash_tx(uint32_t *spi_base, uint8_t in);
static int flash_rx(uint32_t *spi_base, uint8_t *out);
static int flash_init(uint32_t *spi_base);
static int flash_wip(uint32_t *spi_base);
static int flash_erase(uint32_t *spi_base, uint32_t first_addr, uint32_t end_addr);
static int flash_write(uint32_t *spi_base, uint8_t* src_address, uint32_t write_offset, uint32_t write_count);
static int flash_read(uint32_t *spi_base, uint8_t* dst_address, uint32_t read_offset, uint32_t read_count);

int loader_main(uint32_t cs, uint32_t *spi_base, uint32_t params1,
				uint32_t params2, uint32_t params3)
{
	int retval = 0;
	unsigned long params1_temp = params1;
	nuspi_disable_hw_mode(spi_base);
	switch (cs)
	{
	case ERASE_CMD:
		retval = flash_erase(spi_base, params1, params2);
		break;
	case WRITE_CMD:
		retval = flash_write(spi_base, (uint8_t*)params1_temp, params2, params3);
		break;
	case READ_CMD:
		retval = flash_read(spi_base, (uint8_t*)params1_temp, params2, params3);
		break;
	case PROBE_CMD:
		retval = flash_init(spi_base);
		break;
	default:
		break;
	}
	nuspi_enable_hw_mode(spi_base);
	return retval;
}

static inline void nuspi_read_reg(uint32_t *spi_base, uint32_t offset, uint32_t *value)
{
	*value = spi_base[offset / 4];
}

static inline void nuspi_write_reg(uint32_t *spi_base, uint32_t offset, uint32_t value)
{
	spi_base[offset / 4] = value;
}

static void nuspi_disable_hw_mode(uint32_t *spi_base)
{
	uint32_t fctrl = 0;
	nuspi_read_reg(spi_base, NUSPI_REG_FCTRL, &fctrl);
	nuspi_write_reg(spi_base, NUSPI_REG_FCTRL, fctrl & ~NUSPI_FCTRL_EN);
}

static void nuspi_enable_hw_mode(uint32_t *spi_base)
{
	uint32_t fctrl = 0;
	nuspi_read_reg(spi_base, NUSPI_REG_FCTRL, &fctrl);
	nuspi_write_reg(spi_base, NUSPI_REG_FCTRL, fctrl | NUSPI_FCTRL_EN);
}

static void nuspi_set_dir(uint32_t *spi_base, uint32_t dir)
{
	uint32_t fmt = 0;
	nuspi_read_reg(spi_base, NUSPI_REG_FMT, &fmt);
	if (dir != ((0x8 & fmt) >> 3)) {
		nuspi_write_reg(spi_base, NUSPI_REG_FMT,
					(fmt & ~(NUSPI_FMT_DIR(0xFFFFFFFF))) | NUSPI_FMT_DIR(dir));
	}
}

static int nuspi_txwm_wait(uint32_t *spi_base)
{
	uint32_t timeout = NUSPI_TXWM_TIMEOUT;
	uint32_t version = 0;
	nuspi_read_reg(spi_base, NUSPI_REG_VERSION, &version);
	if(version > 0x10100) {
		uint32_t status = 0;
		while (timeout--) {
			nuspi_read_reg(spi_base, NUSPI_REG_STATUS, &status);
			if (0 == (status & NUSPI_STAT_BUSY)) {
				break;
			}
		}
	} else {
		uint32_t ip = 0;
		while (timeout--) {
			nuspi_read_reg(spi_base, NUSPI_REG_IP, &ip);
			if (ip & NUSPI_IP_TXWM) {
				break;
			}
		}
	}
	if (timeout) {
		return NUSPI_OK;
	}
	return NUSPI_TXWM_ERR;
}

static int nuspi_tx(uint32_t *spi_base, uint8_t in)
{
	uint32_t timeout = NUSPI_TXWM_TIMEOUT;
	uint32_t version = 0;
	nuspi_read_reg(spi_base, NUSPI_REG_VERSION, &version);
	if(version > 0x10100) {
		uint32_t status = 0;
		while (timeout--) {
			nuspi_read_reg(spi_base, NUSPI_REG_STATUS, &status);
			if (!(status & NUSPI_STAT_TXFULL)) {
				break;
			}
		}
	} else {
		uint32_t txfifo = 0;
		while (timeout--) {
			nuspi_read_reg(spi_base, NUSPI_REG_TXDATA, &txfifo);
			if (!(txfifo >> 31)) {
				break;
			}
		}
	}
	if (timeout) {
		nuspi_write_reg(spi_base, NUSPI_REG_TXDATA, in);
		return NUSPI_OK;
	}
	return NUSPI_TX_ERR;
}

static int nuspi_rx(uint32_t *spi_base, uint8_t *out)
{
	uint32_t timeout = NUSPI_TXWM_TIMEOUT;
	uint32_t version = 0;
	uint32_t value = 0;
	nuspi_read_reg(spi_base, NUSPI_REG_VERSION, &version);
	if(version > 0x10100) {
		uint32_t status = 0;
		while (timeout--) {
			nuspi_read_reg(spi_base, NUSPI_REG_STATUS, &status);
			if (!(status & NUSPI_STAT_RXEMPTY)) {
				break;
			}
		}
		nuspi_read_reg(spi_base, NUSPI_REG_RXDATA, &value);
	} else {
		while (timeout--) {
			nuspi_read_reg(spi_base, NUSPI_REG_RXDATA, &value);
			if (!(value >> 31)) {
				break;
			}
		}
	}
	if (timeout) {
		*out = value & 0xff;
		return NUSPI_OK;
	}
	return NUSPI_RX_ERR;
}

static int flash_tx(uint32_t *spi_base, uint8_t in) {
	int retval = 0;
	nuspi_set_dir(spi_base, NUSPI_DIR_TX);
	retval |= nuspi_tx(spi_base, in);
	if (retval != NUSPI_OK) {
		retval |= FLASH_TX_ERR;
		goto out;
	}
out:
	return retval;
}

static int flash_rx(uint32_t *spi_base, uint8_t *out) {
	int retval = 0;
	uint8_t value = 0;
	nuspi_set_dir(spi_base, NUSPI_DIR_RX);
	retval |= nuspi_tx(spi_base, 00);
	if (retval != NUSPI_OK) {
		retval |= FLASH_RX_ERR;
		goto out;
	}
	retval |= nuspi_rx(spi_base, &value);
	if (retval != NUSPI_OK) {
		retval |= FLASH_RX_ERR;
		goto out;
	}
	*out = value;
out:
	return retval;
}

static int flash_wip(uint32_t *spi_base)
{
	int retval = 0;
	uint8_t value = 0;
	nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_HOLD);
	retval |= flash_tx(spi_base, SPIFLASH_READ_STATUS);
	if (retval != NUSPI_OK) {
		retval |= FLASH_WIP_ERR;
		goto out;
	}
	retval |= nuspi_txwm_wait(spi_base);
	if (retval != NUSPI_OK) {
		retval |= FLASH_WIP_ERR;
		goto out;
	}
	while (1) {
		retval |= flash_rx(spi_base, &value);
		if (retval != NUSPI_OK) {
			retval |= FLASH_WIP_ERR;
			goto out;
		}
		if ((value & SPIFLASH_BSY_BIT) == 0) {
			break;
		}
	}
out:
	nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_AUTO);
	return retval;
}

static int flash_init(uint32_t *spi_base)
{
	int retval = 0;
	uint8_t rx  = 0;

	/* clear rxfifo */
	uint32_t version = 0;
	uint32_t value = 0;
	nuspi_read_reg(spi_base, NUSPI_REG_VERSION, &version);
	if(version > 0x10100) {
		uint32_t status = 0;
		while (1) {
			nuspi_read_reg(spi_base, NUSPI_REG_RXDATA, &value);
			nuspi_read_reg(spi_base, NUSPI_REG_STATUS, &status);
			if (status & NUSPI_STAT_RXEMPTY) {
				break;
			}
		}
	} else {
		while (1) {
			nuspi_read_reg(spi_base, NUSPI_REG_RXDATA, &value);
			if (value >> 31) {
				break;
			}
		}
	}
	/* init register */
	nuspi_write_reg(spi_base, NUSPI_REG_SCKMODE, 0x0);
	nuspi_write_reg(spi_base, NUSPI_REG_FORCE, 0x3);
	nuspi_write_reg(spi_base, NUSPI_REG_FCTRL, 0x0);
	nuspi_write_reg(spi_base, NUSPI_REG_FMT, 0x80008);
	nuspi_write_reg(spi_base, NUSPI_REG_FFMT, 0x30007);
	nuspi_write_reg(spi_base, NUSPI_REG_RXEDGE, 0x0);
	/* flash reset */
	nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_HOLD);
	retval |= flash_tx(spi_base, SPIFLASH_ENABLE_RESET);
	if (retval != NUSPI_OK) {
		retval |= FLASH_INIT_ERR;
		goto out;
	}
	nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_AUTO);
	retval |= nuspi_txwm_wait(spi_base);
	if (retval != NUSPI_OK) {
		retval |= FLASH_INIT_ERR;
		goto out;
	}
	nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_HOLD);
	retval |= flash_tx(spi_base, SPIFLASH_RESET_DEVICE);
	if (retval != NUSPI_OK) {
		retval |= FLASH_INIT_ERR;
		goto out;
	}
	nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_AUTO);
	retval |= nuspi_txwm_wait(spi_base);
	if (retval != NUSPI_OK) {
		retval |= FLASH_INIT_ERR;
		goto out;
	}
	/* unlock */
	nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_HOLD);
	retval |= flash_tx(spi_base, SPIFLASH_WRITE_STATUS1);
	if (retval != NUSPI_OK) {
		retval |= FLASH_INIT_ERR;
		goto out;
	}
	retval |= flash_tx(spi_base, 0x00);
	if (retval != NUSPI_OK) {
		retval |= FLASH_INIT_ERR;
		goto out;
	}
	nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_AUTO);
	retval |= nuspi_txwm_wait(spi_base);
	if (retval != NUSPI_OK) {
		retval |= FLASH_INIT_ERR;
		goto out;
	}
	/* read flash id */
	nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_HOLD);
	retval |= flash_tx(spi_base, SPIFLASH_READ_ID);
	if (retval != NUSPI_OK) {
		retval |= FLASH_INIT_ERR;
		goto out;
	}
	for (int i = 0;i < 3;i++) {
		if (NUSPI_OK != flash_rx(spi_base, &rx)) {
			retval |= FLASH_INIT_ERR;
			goto out;
		}
		retval |= rx << (i * 8);
	}
	nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_AUTO);
out:
	return retval;
}

static int flash_erase(uint32_t *spi_base, uint32_t first_addr, uint32_t end_addr)
{
	int retval = 0;
	uint32_t curr_addr = first_addr;
	uint32_t sector_num = (end_addr - first_addr) / SPIFLASH_BLOCK_SIZE;
	for (int i = 0;i < sector_num;i++) {
		curr_addr += i * SPIFLASH_BLOCK_SIZE;
		nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_HOLD);
		retval |= flash_tx(spi_base, SPIFLASH_WRITE_ENABLE);
		if (retval != NUSPI_OK) {
			retval |= FLASH_ERASE_ERR;
			goto out;
		}
		nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_AUTO);
		retval |= nuspi_txwm_wait(spi_base);
		if (retval != NUSPI_OK) {
			retval |= FLASH_ERASE_ERR;
			goto out;
		}
		nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_HOLD);
		retval |= flash_tx(spi_base, SPIFLASH_BLOCK_ERASE);
		if (retval != NUSPI_OK) {
			retval |= FLASH_ERASE_ERR;
			goto out;
		}
		retval |= flash_tx(spi_base, curr_addr >> 16);
		if (retval != NUSPI_OK) {
			retval |= FLASH_ERASE_ERR;
			goto out;
		}
		retval |= flash_tx(spi_base, curr_addr >> 8);
		if (retval != NUSPI_OK) {
			retval |= FLASH_ERASE_ERR;
			goto out;
		}
		retval |= flash_tx(spi_base, curr_addr);
		if (retval != NUSPI_OK) {
			retval |= FLASH_ERASE_ERR;
			goto out;
		}
		nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_AUTO);
		retval |= flash_wip(spi_base);
		if (retval != NUSPI_OK) {
			retval |= FLASH_ERASE_ERR;
			goto out;
		}
	}
out:
	return retval;
}

static int flash_write(uint32_t *spi_base, uint8_t* src_address, uint32_t write_offset, uint32_t write_count)
{
	int retval = 0;
	uint32_t cur_offset = write_offset % SPIFLASH_PAGE_SIZE;
	uint32_t cur_count = 0;
	while (write_count > 0) {
		if ((cur_offset + write_count) >= SPIFLASH_PAGE_SIZE) {
			cur_count = SPIFLASH_PAGE_SIZE - cur_offset;
		} else {
			cur_count = write_count;
		}
		cur_offset = 0;
		nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_HOLD);
		retval |= flash_tx(spi_base, SPIFLASH_WRITE_ENABLE);
		if (retval != NUSPI_OK) {
			retval |= FLASH_WRITE_ERR;
			goto out;
		}
		nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_AUTO);
		retval |= nuspi_txwm_wait(spi_base);
		if (retval != NUSPI_OK) {
			retval |= FLASH_WRITE_ERR;
			goto out;
		}
		nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_HOLD);
		retval |= flash_tx(spi_base, SPIFLASH_PAGE_PROGRAM);
		if (retval != NUSPI_OK) {
			retval |= FLASH_WRITE_ERR;
			goto out;
		}
		retval |= flash_tx(spi_base, write_offset >> 16);
		if (retval != NUSPI_OK) {
			retval |= FLASH_WRITE_ERR;
			goto out;
		}
		retval |= flash_tx(spi_base, write_offset >> 8);
		if (retval != NUSPI_OK) {
			retval |= FLASH_WRITE_ERR;
			goto out;
		}
		retval |= flash_tx(spi_base, write_offset);
		if (retval != NUSPI_OK) {
			retval |= FLASH_WRITE_ERR;
			goto out;
		}
		for (int i = 0;i < cur_count;i++) {
			retval |= flash_tx(spi_base, *(src_address + i));
			if (retval != NUSPI_OK) {
				retval |= FLASH_WRITE_ERR;
				goto out;
			}
		}
		nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_AUTO);
		retval |= flash_wip(spi_base);
		if (retval != NUSPI_OK) {
			retval |= FLASH_WRITE_ERR;
			goto out;
		}
		src_address += cur_count;
		write_offset += cur_count;
		write_count -= cur_count;
	}
out:
	return retval;
}

static int flash_read(uint32_t *spi_base, uint8_t* dst_address, uint32_t read_offset, uint32_t read_count)
{
	int retval = 0;
	nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_HOLD);
	retval |= flash_tx(spi_base, SPIFLASH_READ);
	if (retval != NUSPI_OK) {
		retval |= FLASH_READ_ERR;
		goto out;
	}
	retval |= flash_tx(spi_base, read_offset >> 16);
	if (retval != NUSPI_OK) {
		retval |= FLASH_READ_ERR;
		goto out;
	}
	retval |= flash_tx(spi_base, read_offset >> 8);
	if (retval != NUSPI_OK) {
		retval |= FLASH_READ_ERR;
		goto out;
	}
	retval |= flash_tx(spi_base, read_offset);
	if (retval != NUSPI_OK) {
		retval |= FLASH_READ_ERR;
		goto out;
	}
	retval |= nuspi_txwm_wait(spi_base);
	if (retval != NUSPI_OK) {
		retval |= FLASH_READ_ERR;
		goto out;
	}
	for (int i = 0;i < read_count;i++) {
		retval |= flash_rx(spi_base, (dst_address + i));
		if (retval != NUSPI_OK) {
			retval |= FLASH_READ_ERR;
			goto out;
		}
	}
	nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_AUTO);
	retval |= flash_wip(spi_base);
	if (retval != NUSPI_OK) {
		retval |= FLASH_READ_ERR;
		goto out;
	}
out:
	return retval;
}
