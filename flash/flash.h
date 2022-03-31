#include <stdio.h>

#define RETURN_FLASH_WIP_ERROR        (0x1 << 0)
#define RETURN_FLASH_ERASE_ERROR      (0x1 << 1)
#define RETURN_FLASH_WRITE_ERROR      (0x1 << 2)
#define RETURN_FLASH_READ_ERROR       (0x1 << 3)

int flash_init(uint32_t *spi_base);
int flash_erase(uint32_t *spi_base, uint32_t start_addr, uint32_t end_addr);
int flash_write(uint32_t *spi_base, uint8_t* buffer, uint32_t offset, uint32_t count);
int flash_read(uint32_t *spi_base, uint8_t* buffer, uint32_t offset, uint32_t count);
