#include <stdio.h>

void spi_init(uint32_t *spi_base);
void spi_hw(uint32_t *spi_base, bool sel);
void spi_cs(uint32_t *spi_base, bool sel);
int spi_tx(uint32_t *spi_base, uint8_t *in, uint32_t len);
int spi_rx(uint32_t *spi_base, uint8_t *out, uint32_t len);
