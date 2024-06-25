#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define RETURN_SPI_TX_ERROR       (0x1 << 4)
#define RETURN_SPI_RX_ERROR       (0x1 << 5)

void spi_init(uint32_t *spi_base);
void spi_hw(uint32_t *spi_base, bool sel);
void spi_cs(uint32_t *spi_base, bool sel);
int spi_tx(uint32_t *spi_base, uint8_t *in, uint32_t len);
int spi_rx(uint32_t *spi_base, uint8_t *out, uint32_t len);
