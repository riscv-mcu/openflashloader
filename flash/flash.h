#include <stdio.h>

int flash_init(uint32_t *spi_base);
int flash_erase(uint32_t *spi_base, uint32_t start_addr, uint32_t end_addr);
int flash_write(uint32_t *spi_base, uint8_t* buffer, uint32_t offset, uint32_t count);
int flash_read(uint32_t *spi_base, uint8_t* buffer, uint32_t offset, uint32_t count);
