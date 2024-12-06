#include "flash.h"

/* WARNING: NEVER USE GLOBAL VARIABLE IN THIS FILE */

/*==== Loader ====*/
#define ERASE_CMD           (1)
#define WRITE_CMD           (2)
#define READ_CMD            (3)
#define PROBE_CMD           (4)

#define RETURN_OK           (0)
#define RETURN_ERROR        (0x1 << 31)

int loader_main(uint32_t cs, uint32_t *spi_base, uint32_t params1, uint32_t params2, uint32_t params3)
{
    int retval = 0;
    unsigned long params1_temp = params1;
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
        retval = RETURN_ERROR;
        break;
    }
    return retval;
}
