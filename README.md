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

## How to use

* step1

  > Make sure your OpenOCD support custom flash bank.

* step2

  > make clean & make

* step3

  > Modify OpenOCD flash bank cfg like this:
  * flash bank $_FLASHNAME custom 0x20000000 0 0 0 $_TARGETNAME 0xf0040000 *D:/Git/openocd-flashloader/riscv64_loader.bin*

## Secondary Development

### Use Nuclie's SPI

  > Read through the Flash data sheet, rewrite the following API.
  * static int flash_init(uint32_t *spi_base);
  * static int flash_wip(uint32_t *spi_base);
  * static int flash_erase(uint32_t *spi_base, uint32_t first_addr, uint32_t end_addr);
  * static int flash_write(uint32_t *spi_base, uint8_t* src_address, uint32_t write_offset, uint32_t write_count);
  * static int flash_read(uint32_t *spi_base, uint8_t* dst_address, uint32_t read_offset, uint32_t read_count);

  *Warning:"flash_init" need return Flash's ID*

### Not use Nuclie's SPI
