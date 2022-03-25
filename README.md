# openocd-flashloader

## Directory Structure

Here is the directory structure of openocd-flashloader.

~~~console
openocd-flashloader
├── Makefile
├── loader.c
├── README.md
├── riscv.lds
├── startup.S
~~~

* **Makefile**

  A Makefile just for build. You may need to modify this file, When toolchain or build flags is different.

* **loader.c**

  Source code for openocd flashloader. You may need to modify this file, When SPI ip or Flash is different.

* **riscv.lds**

  A link script file just for build.

* **startup.S**

  A startup file.

  > **Disable the use of global variables in flash loader.**

## How to use

* step1

  > Make sure your OpenOCD support custom flash bank.

* step2

  > make clean & make

* step3

  > Modify OpenOCD flash bank cfg like this:

  * flash bank $_FLASHNAME custom 0x20000000 0 0 0 $_TARGETNAME 0xf0040000 *D:/Git/openocd-flashloader/riscv64_loader.bin*

## Secondary Development

### Use Nuclei's SPI

#### Execute Procedure

![procedure](img/procedure.png)

#### Loader Function

  > Read through the Flash data sheet, rewrite the following API.
  >
  > * int loader_main(uint32_t cs, uint32_t *spi_base, uint32_t params1, uint32_t params2, uint32_t params3);
  > * int flash_init(uint32_t *spi_base);
  > * int flash_erase(uint32_t *spi_base, uint32_t first_addr, uint32_t end_addr);
  > * int flash_write(uint32_t *spi_base, uint8_t* src_address, uint32_t write_offset, uint32_t write_count);
  > * int flash_read(uint32_t *spi_base, uint8_t* dst_address, uint32_t read_offset, uint32_t read_count);

  *Warning:"flash_init" need return Flash's ID*

#### Function Overview

##### loader_main

****

**Prototype**

```c
int loader_main(uint32_t cs, uint32_t *spi_base, uint32_t params1, uint32_t params2, uint32_t params3);
```

**Description**

This function is called for any Flash operation. Mainly used to select functions, disable nuspi hardware mode、set default DIR.

**Parameters**

* cs: function select.
* spi_base: SPI base address.
* params1: params.
* params2: params.
* params3: params.

**Return values**

* 0: OK
* Other: ERROR

**Code Example**

```c
int loader_main(uint32_t cs, uint32_t *spi_base, uint32_t params1,
				uint32_t params2, uint32_t params3)
{
	int retval = 0;
	unsigned long params1_temp = params1;
	nuspi_disable_hw_mode(spi_base);
	nuspi_set_dir(spi_base, NUSPI_DIR_TX);
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
		retval = NUSPI_ERR;
		break;
	}
	nuspi_enable_hw_mode(spi_base);
	return retval;
}
```

##### flash_init

****

**Prototype**

```c
int flash_init(uint32_t *spi_base);
```

**Description**

Initialize nuspi、 Read flash ID and return the flash ID.

**Parameters**

* spi_base: SPI base address.

**Return values**

Flash's ID

**Code Example**

```c
int flash_init(uint32_t *spi_base)
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
	/* read flash id */
	nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_HOLD);
	retval |= flash_tx(spi_base, SPIFLASH_READ_ID, true);
	if (retval != NUSPI_OK) {
		goto error;
	}
	nuspi_set_dir(spi_base, NUSPI_DIR_RX);
	for (int i = 0;i < 3;i++) {
		if (NUSPI_OK != flash_rx(spi_base, &rx)) {
			goto error;
		}
		retval |= rx << (i * 8);
	}
	nuspi_set_dir(spi_base, NUSPI_DIR_TX);
	nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_AUTO);
	return retval;
error:
	return retval | FLASH_INIT_ERR;
}
```

##### flash_erase

****

**Prototype**

```c
int flash_erase(uint32_t *spi_base, uint32_t first_addr, uint32_t end_addr);
```

**Description**

Erases Flash space between **first_addr** and **end_addr**.

**Parameters**

* spi_base: SPI base address.
* first_addr: start addr of the flash to be erased.
* end_addr: end addr of the flash to be erased.

**Return values**

* 0: OK
* Other: ERROR

**Code Example**

```c
int flash_erase(uint32_t *spi_base, uint32_t first_addr, uint32_t end_addr)
{
	int retval = 0;
	uint32_t curr_addr = first_addr;
	uint32_t sector_num = (end_addr - first_addr) / SPIFLASH_BLOCK_SIZE;
	for (int i = 0;i < sector_num;i++) {
		curr_addr += i * SPIFLASH_BLOCK_SIZE;
		nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_HOLD);
		retval |= flash_tx(spi_base, SPIFLASH_WRITE_ENABLE, true);
		if (retval != NUSPI_OK) {
			goto error;
		}
		nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_AUTO);
		nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_HOLD);
		retval |= flash_tx(spi_base, SPIFLASH_BLOCK_ERASE, false);
		if (retval != NUSPI_OK) {
			goto error;
		}
		retval |= flash_tx(spi_base, curr_addr >> 16, false);
		if (retval != NUSPI_OK) {
			goto error;
		}
		retval |= flash_tx(spi_base, curr_addr >> 8, false);
		if (retval != NUSPI_OK) {
			goto error;
		}
		retval |= flash_tx(spi_base, curr_addr, true);
		if (retval != NUSPI_OK) {
			goto error;
		}
		nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_AUTO);
		retval |= flash_wip(spi_base);
		if (retval != NUSPI_OK) {
			goto error;
		}
	}
	return retval;
error:
	return retval | FLASH_ERASE_ERR;
}
```


##### flash_write

****

**Prototype**

```c
int flash_write(uint32_t *spi_base, uint8_t* src_address, uint32_t write_offset, uint32_t write_count);
```

**Description**

Write the **write_count** data from **src_address** to flash's **write_offset**.

**Parameters**

* spi_base: SPI base address.
* src_address: source address.
* write_offset: flash offset.
* write_count:  number of bytes to be write.

**Return values**

* 0: OK
* Other: ERROR

**Code Example**

```c
int flash_write(uint32_t *spi_base, uint8_t* src_address, uint32_t write_offset, uint32_t write_count)
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
		retval |= flash_tx(spi_base, SPIFLASH_WRITE_ENABLE, true);
		if (retval != NUSPI_OK) {
			goto error;
		}
		nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_AUTO);
		nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_HOLD);
		retval |= flash_tx(spi_base, SPIFLASH_PAGE_PROGRAM, false);
		if (retval != NUSPI_OK) {
			goto error;
		}
		retval |= flash_tx(spi_base, write_offset >> 16, false);
		if (retval != NUSPI_OK) {
			goto error;
		}
		retval |= flash_tx(spi_base, write_offset >> 8, false);
		if (retval != NUSPI_OK) {
			goto error;
		}
		retval |= flash_tx(spi_base, write_offset, false);
		if (retval != NUSPI_OK) {
			goto error;
		}
		for (int i = 0;i < cur_count;i++) {
			if ((i + 1) == cur_count) {
				retval |= flash_tx(spi_base, *(src_address + i), true);
			} else {
				retval |= flash_tx(spi_base, *(src_address + i), false);
			}
			if (retval != NUSPI_OK) {
				goto error;
			}
		}
		nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_AUTO);
		retval |= flash_wip(spi_base);
		if (retval != NUSPI_OK) {
			goto error;
		}
		src_address += cur_count;
		write_offset += cur_count;
		write_count -= cur_count;
	}
	return retval;
error:
	return retval | FLASH_WRITE_ERR;
}
```


##### flash_read

****

**Prototype**

```c
int flash_read(uint32_t *spi_base, uint8_t* dst_address, uint32_t read_offset, uint32_t read_count);
```

**Description**

Read the **read_count** data from flash's **read_offset** to **dst_address**.

**Parameters**

* spi_base: SPI base address.
* dst_address: destination address.
* read_offset: flash offset.
* read_count: number of bytes to be read.

**Return values**

* 0: OK
* Other: ERROR

**Code Example**

```c
int flash_read(uint32_t *spi_base, uint8_t* dst_address, uint32_t read_offset, uint32_t read_count)
{
	int retval = 0;
	nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_HOLD);
	retval |= flash_tx(spi_base, SPIFLASH_READ, false);
	if (retval != NUSPI_OK) {
		goto error;
	}
	retval |= flash_tx(spi_base, read_offset >> 16, false);
	if (retval != NUSPI_OK) {
		goto error;
	}
	retval |= flash_tx(spi_base, read_offset >> 8, false);
	if (retval != NUSPI_OK) {
		goto error;
	}
	retval |= flash_tx(spi_base, read_offset, true);
	if (retval != NUSPI_OK) {
		goto error;
	}
	nuspi_set_dir(spi_base, NUSPI_DIR_RX);
	for (int i = 0;i < read_count;i++) {
		retval |= flash_rx(spi_base, (dst_address + i));
		if (retval != NUSPI_OK) {
			goto error;
		}
	}
	nuspi_set_dir(spi_base, NUSPI_DIR_TX);
	nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_AUTO);
	retval |= flash_wip(spi_base);
	if (retval != NUSPI_OK) {
		goto error;
	}
	return retval;
error:
	return retval | FLASH_READ_ERR;
}
```

#### Error Table

|                 |           |
| --------------- | --------- |
| NUSPI_OK        | 0x0       |
| NUSPI_ERR       | 0x1 << 31 |
| NUSPI_TXWM_ERR  | 0x1 << 0  |
| NUSPI_TX_ERR    | 0x1 << 1  |
| NUSPI_RX_ERR    | 0x1 << 2  |
| FLASH_TX_ERR    | 0x1 << 3  |
| FLASH_RX_ERR    | 0x1 << 4  |
| FLASH_WIP_ERR   | 0x1 << 5  |
| FLASH_INIT_ERR  | 0x1 << 6  |
| FLASH_ERASE_ERR | 0x1 << 7  |
| FLASH_WRITE_ERR | 0x1 << 8  |
| FLASH_READ_ERR  | 0x1 << 9  |



### Not use Nuclei's SPI
