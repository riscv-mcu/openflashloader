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
