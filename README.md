# Openocd-Flashloader

## Introduction

With the increasing number of customers, the standard OpenOCD can no longer met the market demand. In order to enable customers to customize the Flash driver and Loader algorithm of OpenOCD without recompiling the OpenOCD tool, OpenOCD-FlashLoader perfectly solves this problem. Please refer to the following description for details.

## Directory Structure

The directory structure of openocd-flashloader.

```console
openocd-flashloader
├── flash                      // FLASH Drivers Directory
│   ├── flash.h                    // FLASH Driver's API
│   ├── w25q256fv.c                // W25Q256FV Driver Source Code
├── img                        // Store README.md Needs image
│   ├── procedure.png              // Flash Loader Execute Procedure
├── loader                     // Flash Loader Directory
│   ├── loader.c                   // Loader Source Code
│   ├── riscv.lds                  // Loader Link Script
│   ├── startup.S                  // Loader startup file
├── spi                        // SPI Driver Directory
│   ├── spi.h                      // SPI Driver's API
│   ├── nuspi.c                    // Nuclei SPI Driver
│   ├── fespi.c                    // Sfive SPI Driver
├── test                       // Loader Tests Directory
│   ├── main.c                     // Loader Self-test Code
├── .gitignore                 // Git Ignore File
├── Makefile                   // Main Makefile
├── Makefile.loader            // Loader Makefile
├── Makefile.sdk               // Tests Makefile
├── README.md                  // Readme
```

## Execute Procedure

![procedure](img/procedure.png)

## Secondary Development

Assume that the FLASH model to be added is "custom-flash" and the SPI name to be added is "custom-spi ".

**The following conditions must be met**

- OpenOCD's Version >= 2022.04
- OpenOCD Support "custom-flash" Flash, is equivalent to "custom-flash" in spi.c（https://github.com/riscv-mcu/riscv-openocd/blob/nuclei-develop/src/flash/nor/spi.c）
- Don't use any global variables.(**Warning**)

### Function Overview

#### flash_init

****

**Prototype**

```c
int flash_init(uint32_t *spi_base);
```

**Description**

Initialize nuspi,  Read flash ID and return the flash ID.

**Parameters**

* spi_base: SPI base address.

**Return values**

Flash ID（**Warning**）

#### flash_erase

****

**Prototype**

```c
int flash_erase(uint32_t *spi_base, uint32_t start_addr, uint32_t end_addr);
```

**Description**

Erases Flash space between **start_addr** and **end_addr**.

**Parameters**

* spi_base: SPI base address.
* start_addr: start addr of the flash to be erased.
* end_addr: end addr of the flash to be erased.

**Return values**

* 0: OK
* Other: ERROR


#### flash_write

****

**Prototype**

```c
int flash_write(uint32_t *spi_base, uint8_t* buffer, uint32_t offset, uint32_t count);
```

**Description**

Write the **count** data from **buffer** to flash's **offset**.

**Parameters**

* spi_base: SPI base address.
* buffer: source address.
* offset: flash offset.
* count:  number of bytes to be write.

**Return values**

* 0: OK
* Other: ERROR


#### flash_read

****

**Prototype**

```c
int flash_read(uint32_t *spi_base, uint8_t* buffer, uint32_t offset, uint32_t count);
```

**Description**

Read the **count** data from flash's **offset** to **buffer**.

**Parameters**

* spi_base: SPI base address.
* buffer: destination address.
* offset: flash offset.
* count: number of bytes to be read.

**Return values**

* 0: OK
* Other: ERROR

### Develop Flash Driver

- Create **flash/custom-flash.c** file
- Implement the functions in flash.h

```c
#include <stdio.h>

int flash_init(uint32_t *spi_base);
int flash_erase(uint32_t *spi_base, uint32_t start_addr, uint32_t end_addr);
int flash_write(uint32_t *spi_base, uint8_t* buffer, uint32_t offset, uint32_t count);
int flash_read(uint32_t *spi_base, uint8_t* buffer, uint32_t offset, uint32_t count);
```

**Code Example**(w25q256fv.c)

```c
#include "flash.h"
#include "spi.h"

/*==== FLASH ====*/
#define SPIFLASH_BSY            0
#define SPIFLASH_BSY_BIT        (1 << SPIFLASH_BSY) /* WIP Bit of SPI SR */
/* SPI Flash Commands */
#define SPIFLASH_READ_ID        0x9F /* Read Flash Identification */
#define SPIFLASH_READ_STATUS    0x05 /* Read Status Register */
#define SPIFLASH_WRITE_ENABLE   0x06 /* Write Enable */
#define SPIFLASH_PAGE_PROGRAM   0x02 /* Page Program */
#define SPIFLASH_PAGE_SIZE      0x100
#define SPIFLASH_READ           0x03 /* Normal Read */
#define SPIFLASH_BLOCK_ERASE    0xD8 /* Block Erase */
#define SPIFLASH_BLOCK_SIZE     0x10000

#define RETURN_FLASH_WIP_ERROR        (0x1 << 0)
#define RETURN_FLASH_ERASE_ERROR      (0x1 << 1)
#define RETURN_FLASH_WRITE_ERROR      (0x1 << 2)
#define RETURN_FLASH_READ_ERROR       (0x1 << 3)

static inline int flash_wip(uint32_t *spi_base)
{
    int retval = 0;
    uint8_t value = 0;
    /* read status */
    spi_cs(spi_base, true);
    value = SPIFLASH_READ_STATUS;
    retval |= spi_tx(spi_base, &value, 1);
    while (1) {
        retval |= spi_rx(spi_base, &value, 1);
        if ((value & SPIFLASH_BSY_BIT) == 0) {
            break;
        }
    }
    spi_cs(spi_base, false);
    if (retval) {
        return retval | RETURN_FLASH_WIP_ERROR;
    }
    return retval;
}

int flash_init(uint32_t *spi_base)
{
    int retval = 0;
    uint8_t value[3] = {0};
    spi_init(spi_base);
    /* read flash id */
    spi_hw(spi_base, false);
    spi_cs(spi_base, true);
    value[0] = SPIFLASH_READ_ID;
    retval |= spi_tx(spi_base, value, 1);
    retval |= spi_rx(spi_base, value, 3);
    spi_cs(spi_base, false);
    spi_hw(spi_base, true);
    retval = (value[2] << 16) | (value[1] << 8) | value[0];
    return retval;
}

int flash_erase(uint32_t *spi_base, uint32_t start_addr, uint32_t end_addr)
{
    int retval = 0;
    uint8_t value[4] = {0};
    uint32_t curr_addr = start_addr;
    uint32_t sector_num = (end_addr - start_addr) / SPIFLASH_BLOCK_SIZE;
    /* erase flash */
    spi_hw(spi_base, false);
    for (int i = 0;i < sector_num;i++) {
        curr_addr += i * SPIFLASH_BLOCK_SIZE;
        /* send write enable cmd */
        spi_cs(spi_base, true);
        value[0] = SPIFLASH_WRITE_ENABLE;
        retval |= spi_tx(spi_base, value, 1);
        spi_cs(spi_base, false);
        /* send erase cmd and addr*/
        spi_cs(spi_base, true);
        value[0] = SPIFLASH_BLOCK_ERASE;
        value[1] = curr_addr >> 16;
        value[2] = curr_addr >> 8;
        value[3] = curr_addr >> 0;
        retval |= spi_tx(spi_base, value, 4);
        spi_cs(spi_base, false);
        retval |= flash_wip(spi_base);
    }
    spi_hw(spi_base, true);
    if (retval) {
        return retval | RETURN_FLASH_ERASE_ERROR;
    }
    return retval;
}

int flash_write(uint32_t *spi_base, uint8_t* buffer, uint32_t offset, uint32_t count)
{
    int retval = 0;
    uint8_t value[4] = {0};
    uint32_t cur_offset = offset % SPIFLASH_PAGE_SIZE;
    uint32_t cur_count = 0;
    /* write flash */
    spi_hw(spi_base, false);
    while (count > 0) {
        if ((cur_offset + count) >= SPIFLASH_PAGE_SIZE) {
            cur_count = SPIFLASH_PAGE_SIZE - cur_offset;
        } else {
            cur_count = count;
        }
        cur_offset = 0;
        /* send write enable cmd */
        spi_cs(spi_base, true);
        value[0] = SPIFLASH_WRITE_ENABLE;
        retval |= spi_tx(spi_base, value, 1);
        spi_cs(spi_base, false);
        /* send write cmd and addr*/
        spi_cs(spi_base, true);
        value[0] = SPIFLASH_PAGE_PROGRAM;
        value[1] = offset >> 16;
        value[2] = offset >> 8;
        value[3] = offset >> 0;
        retval |= spi_tx(spi_base, value, 4);
        retval |= spi_tx(spi_base, buffer, cur_count);
        spi_cs(spi_base, false);
        retval |= flash_wip(spi_base);
        buffer += cur_count;
        offset += cur_count;
        count -= cur_count;
    }
    spi_hw(spi_base, true);
    if (retval) {
        return retval | RETURN_FLASH_WRITE_ERROR;
    }
    return retval;
}

int flash_read(uint32_t *spi_base, uint8_t* buffer, uint32_t offset, uint32_t count)
{
    int retval = 0;
    uint8_t value[4] = {0};
    /* read flash */
    spi_hw(spi_base, false);
    /* send read cmd and addr*/
    spi_cs(spi_base, true);
    value[0] = SPIFLASH_READ;
    value[1] = offset >> 16;
    value[2] = offset >> 8;
    value[3] = offset >> 0;
    retval |= spi_tx(spi_base, value, 4);
    retval |= spi_rx(spi_base, buffer, count);
    spi_cs(spi_base, false);
    retval |= flash_wip(spi_base);
    spi_hw(spi_base, true);
    if (retval) {
        return retval | RETURN_FLASH_READ_ERROR;
    }
    return retval;
}
```



### Develop SPI Driver

- Create **spi/custom-spi.c** file
- Implement the functions in spi.h

```c
#include <stdio.h>
#include <stdbool.h>

void spi_init(uint32_t *spi_base);
void spi_hw(uint32_t *spi_base, bool sel);
void spi_cs(uint32_t *spi_base, bool sel);
int spi_tx(uint32_t *spi_base, uint8_t *in, uint32_t len);
int spi_rx(uint32_t *spi_base, uint8_t *out, uint32_t len);
```



**Code Example**(nuspi.c)

```c
#include "spi.h"

/* Register offsets */
#define NUSPI_REG_SCKMODE           (0x04)
#define NUSPI_REG_FORCE             (0x0C)
#define NUSPI_REG_CSMODE            (0x18)
#define NUSPI_REG_FMT               (0x40)
#define NUSPI_REG_TXDATA            (0x48)
#define NUSPI_REG_RXDATA            (0x4C)
#define NUSPI_REG_FCTRL             (0x60)
#define NUSPI_REG_FFMT              (0x64)
#define NUSPI_REG_STATUS            (0x7C)
#define NUSPI_REG_RXEDGE            (0x80)
/* FCTRL register */
#define NUSPI_FCTRL_EN              (0x1)
#define NUSPI_INSN_CMD_EN           (0x1)
/* FMT register */
#define NUSPI_FMT_DIR(x)            (((x) & 0x1) << 3)
/* STATUS register */
#define NUSPI_STAT_BUSY             (0x1 << 0)
#define NUSPI_STAT_TXFULL           (0x1 << 4)
#define NUSPI_STAT_RXEMPTY          (0x1 << 5)
#define NUSPI_CSMODE_AUTO           (0)
#define NUSPI_CSMODE_HOLD           (2)
#define NUSPI_DIR_RX                (0)
#define NUSPI_DIR_TX                (1)

#define NUSPI_TX_TIMES_OUT          (500)
#define NUSPI_RX_TIMES_OUT          (500)

#define RETURN_NUSPI_TX_ERROR       (0x1 << 4)
#define RETURN_NUSPI_RX_ERROR       (0x1 << 5)

static inline void nuspi_read_reg(uint32_t volatile *spi_base, uint32_t offset, uint32_t *value)
{
    *value = spi_base[offset / 4];
}

static inline void nuspi_write_reg(uint32_t volatile *spi_base, uint32_t offset, uint32_t value)
{
    spi_base[offset / 4] = value;
}

static inline void nuspi_set_dir(uint32_t *spi_base, uint32_t dir)
{
    uint32_t fmt = 0;
    nuspi_read_reg(spi_base, NUSPI_REG_FMT, &fmt);
    nuspi_write_reg(spi_base, NUSPI_REG_FMT, (fmt & ~(NUSPI_FMT_DIR(0xFFFFFFFF))) | NUSPI_FMT_DIR(dir));
}

void spi_init(uint32_t *spi_base)
{
    /* clear rxfifo */
    uint32_t status = 0;
    while (1) {
        nuspi_read_reg(spi_base, NUSPI_REG_RXDATA, &status);
        nuspi_read_reg(spi_base, NUSPI_REG_STATUS, &status);
        if (status & NUSPI_STAT_RXEMPTY) {
            break;
        }
    }
    /* init register */
    nuspi_write_reg(spi_base, NUSPI_REG_SCKMODE, 0x0);
    nuspi_write_reg(spi_base, NUSPI_REG_FORCE, 0x3);
    nuspi_write_reg(spi_base, NUSPI_REG_FCTRL, 0x0);
    nuspi_write_reg(spi_base, NUSPI_REG_FMT, 0x80008);
    nuspi_write_reg(spi_base, NUSPI_REG_FFMT, 0x30007);
    nuspi_write_reg(spi_base, NUSPI_REG_RXEDGE, 0x0);
}

void spi_hw(uint32_t *spi_base, bool sel)
{
    uint32_t fctrl = 0;
    nuspi_read_reg(spi_base, NUSPI_REG_FCTRL, &fctrl);
    if (sel) {
        nuspi_write_reg(spi_base, NUSPI_REG_FCTRL, fctrl | NUSPI_FCTRL_EN);
    } else {
        nuspi_write_reg(spi_base, NUSPI_REG_FCTRL, fctrl & ~NUSPI_FCTRL_EN);
    }
}

void spi_cs(uint32_t *spi_base, bool sel)
{
    if (sel) {
        nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_HOLD);
    } else {
        nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_AUTO);
    }
}

int spi_tx(uint32_t *spi_base, uint8_t *in, uint32_t len)
{
    uint32_t times_out = 0;
    uint32_t value = 0;
    nuspi_set_dir(spi_base, NUSPI_DIR_TX);
    for (int i = 0;i < len;i++) {
        times_out = NUSPI_TX_TIMES_OUT;
        while (times_out--) {
            nuspi_read_reg(spi_base, NUSPI_REG_STATUS, &value);
            if (!(value & NUSPI_STAT_TXFULL)) {
                break;
            }
        }
        if (0 >= times_out) {
            return RETURN_NUSPI_TX_ERROR;
        }
        nuspi_write_reg(spi_base, NUSPI_REG_TXDATA, in[i]);
    }
    times_out = NUSPI_TX_TIMES_OUT;
    while (times_out--) {
        nuspi_read_reg(spi_base, NUSPI_REG_STATUS, &value);
        if (0 == (value & NUSPI_STAT_BUSY)) {
            break;
        }
    }
    if (0 >= times_out) {
        return RETURN_NUSPI_TX_ERROR;
    }
    return 0;
}

int spi_rx(uint32_t *spi_base, uint8_t *out, uint32_t len)
{
    uint32_t times_out = 0;
    uint32_t value = 0;
    nuspi_set_dir(spi_base, NUSPI_DIR_RX);
    for (int i = 0;i < len;i++) {
        times_out = NUSPI_RX_TIMES_OUT;
        nuspi_write_reg(spi_base, NUSPI_REG_TXDATA, 0x00);
        while (times_out--) {
            nuspi_read_reg(spi_base, NUSPI_REG_STATUS, &value);
            if (!(value & NUSPI_STAT_RXEMPTY)) {
                break;
            }
        }
        if (0 >= times_out) {
            return RETURN_NUSPI_RX_ERROR;
        }
        nuspi_read_reg(spi_base, NUSPI_REG_RXDATA, &value);
        out[i] = value & 0xff;
    }
    return 0;
}
```

## How To Build & Use

```
// SPI=custom-spi FLASH=custom-flash
// the custom-spi and custom-flash need to be changed to match the spi and flash driver file name 
// build loader
make ARCH=rv32 MODE=loader SPI=custom-spi FLASH=custom-flash clean all
make ARCH=rv64 MODE=loader SPI=custom-spi FLASH=custom-flash clean all

// build self-test
set NUCLEI_SDK_ROOT=D:\Nuclei_Work\Git\nuclei-sdk
make ARCH=rv32 MODE=sdk SPI=custom-spi FLASH=custom-flash clean all
make ARCH=rv64 MODE=sdk SPI=custom-spi FLASH=custom-flash clean all
```

```
// openocd flash bank configure
// don't change custom below, it is used by openocd to recognize this is custom flash loader
flash bank $FLASHNAME custom 0x20000000 0 0 0 $TARGETNAME 0x10014000 ~/work/riscv.bin [simulation]
```

## Error Table

| Describe                 | Value     |
| ------------------------ | --------- |
| RETURN_OK                | 0x0       |
| RETURN_ERROR             | 0x1 << 31 |
| RETURN_FLASH_WIP_ERROR   | 0x1 << 0  |
| RETURN_FLASH_ERASE_ERROR | 0x1 << 1  |
| RETURN_FLASH_WRITE_ERROR | 0x1 << 2  |
| RETURN_FLASH_READ_ERROR  | 0x1 << 3  |
|                          |           |
| RETURN_SPI_TX_ERROR      | 0x1 << 4  |
| RETURN_SPI_RX_ERROR      | 0x1 << 5  |
