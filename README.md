# Openocd-Flashloader

## Introduction

With the increasing number of customers, the standard OpenOCD can no longer met the market demand. In order to enable customers to customize the Flash driver and Loader algorithm of OpenOCD without recompiling the OpenOCD tool, OpenOCD-FlashLoader perfectly solves this problem. Please refer to the following description for details.

## Directory Structure

The directory structure of openocd-flashloader.

```console
openocd-flashloader
├── flash                      // FLASH Drivers Directory
│   ├── flash.h                // FLASH Driver's API
│   ├── w25q256fv.c            // W25Q256FV Driver Source Code
├── img                        // Store README.md Needs image
│   ├── procedure.png          // Flash Loader Execute Procedure
├── loader                     // Flash Loader Directory
│   ├── loader.c               // Loader Source Code
│   ├── riscv.lds              // Loader Link Script
│   ├── startup.S              // Loader startup file
├── spi                        // SPI Driver Directory
│   ├── spi.h                  // SPI Driver's API
│   ├── nuspi.c                // Nuclei SPI Driver
│   ├── fespi.c                // Sfive SPI Driver
├── test                       // Loader Tests Directory
│   ├── main.c                 // Loader Self-test Code
├── .gitignore                 // Git Ignore File
├── Makefile                   // Main Makefile
├── Makefile.loader            // Loader Makefile
├── Makefile.sdk               // Tests Makefile
├── README.md                  // Readme
```

## Execute Procedure

![procedure](img/procedure.png)

## Secondary Development

**The following conditions must be met**

- OpenOCD's Version >= 2022.04
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

## How To Build

```
// build loader spi:nuspi.c flash:w25q256fv.c
make ARCH=rv32 MODE=loader SPI=nuspi FLASH=w25q256fv clean all
make ARCH=rv64 MODE=loader SPI=nuspi FLASH=w25q256fv clean all

// build loader spi:fespi.c flash:w25q256fv.c
make ARCH=rv32 MODE=loader SPI=fespi FLASH=w25q256fv clean all
make ARCH=rv64 MODE=loader SPI=fespi FLASH=w25q256fv clean all

/*===========================================================*/

// build self-test spi:nuspi.c flash:w25q256fv.c
set NUCLEI_SDK_ROOT=D:\Nuclei_Work\Git\nuclei-sdk
make ARCH=rv32 MODE=sdk SPI=nuspi FLASH=w25q256fv clean all
make ARCH=rv64 MODE=sdk SPI=nuspi FLASH=w25q256fv clean all

// build self-test spi:fespi.c flash:w25q256fv.c
set NUCLEI_SDK_ROOT=D:\Nuclei_Work\Git\nuclei-sdk
make ARCH=rv32 MODE=sdk SPI=fespi FLASH=w25q256fv clean all
make ARCH=rv64 MODE=sdk SPI=fespi FLASH=w25q256fv clean all
```

> Note:
>
> <mark>The 'fespi' and 'w25q256fv' should be changed to the driver names of customer's own SPI and FLASH.</mark>
>
> <mark>The **SPI** compile option is used to specify the spi driver source file without the suffix.</mark>
>
> <mark>The **FLASH** compile option is used to specify the flash driver source file without the suffix.</mark>

## How To Use

```
// openocd flash bank configure command(only parameters in parentheses can be modified)
flash bank $FLASHNAME custom <spi_xip> 0 0 0 $TARGETNAME <spi_base> <loader_path> [simulation]

// openocd flash bank configure example
flash bank $FLASHNAME custom 0x20000000 0 0 0 $TARGETNAME 0x10014000 ~/work/riscv.bin
// while [simulation] exist, the loader's timeout=0xFFFFFFFF
flash bank $FLASHNAME custom 0x20000000 0 0 0 $TARGETNAME 0x10014000 ~/work/riscv.bin simulation
```

> Note:
>
> <mark>'custom' is the keyword and should not be changed with others.</mark>
>
> <mark>In the current command **custom**  option can't be modified.</mark>
>
> <mark>**<loader_path>** option advised to write the full path,  because you maybe don't know where you are.</mark>

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
