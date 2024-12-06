#include <stdio.h>
#include <stdint.h>

/* NOTE: THIS TEST CODE IS USED WITH NUCLEI SDK */

#define DEBUG_INFO          (1)

/*==== Loader ====*/
#define ERASE_CMD           (1)
#define WRITE_CMD           (2)
#define READ_CMD            (3)
#define PROBE_CMD           (4)

// TODO Modify the following macro as your test requirements
#define TEST_ERASE_SIZE     (0x10000)
#define TEST_OFFSET         (0x00)
#define TEST_COUNT          (512)

uint8_t write_buf[TEST_COUNT] = {0};
uint8_t read_buf[TEST_COUNT] = {0};

void data_init(uint8_t* buf, uint32_t len)
{
    for (int i = 0;i < len;i++) {
        // TODO you can random it
        buf[i] = i + 2;
    }
}

int memory_compare(uint8_t* src, uint8_t* dst, uint32_t len)
{
    while (len--) {
        if(*src++ != *dst++) {
            return -1;
        }
    }
    return 0;
}


// TODO Change SPI_BASE to your real base address

#define SPI_BASE 0x10014000

extern int loader_main(uint32_t cs, uint32_t *spi_base, uint32_t params1, uint32_t params2, uint32_t params3);

int main(void)
{
    int flash_id = 0;
    int retval = 0;
    int ret = 0;

    /* flash loader in/out params */
    uint32_t cs = 0x00;
    uint32_t *spi_base = (uint32_t*)SPI_BASE;
    uint32_t params1 = 0x00;
    uint32_t params2 = 0x00;
    uint32_t params3 = 0x00;

    #if DEBUG_INFO
    printf("Start to do flash loader test at SPI base address 0x%x\r\n", spi_base);
    #endif

    /* data init */
    data_init(write_buf, TEST_COUNT);

    /* flash init */
    cs = PROBE_CMD;
    flash_id = loader_main(cs, spi_base, params1, params2, params3);
    #if DEBUG_INFO
    printf("The current Flash ID is %#x\r\n", flash_id);
    #endif

    /* flash erase */
    cs = ERASE_CMD;
    params1 = TEST_OFFSET;//start_addr
    params2 = TEST_OFFSET + TEST_ERASE_SIZE;//end_addr
    retval = loader_main(cs, spi_base, params1, params2, params3);
    if (retval) {
        #if DEBUG_INFO
        printf("The erase error code\r\n");
        #endif
        ret |= 0x1;
    } else {
        #if DEBUG_INFO
        printf("Erase success\r\n");
        #endif
    }

    /* flash write */
    cs = WRITE_CMD;
    params1 = (uint32_t)write_buf;//buffer
    params2 = TEST_OFFSET;//offset
    params3 = TEST_COUNT;//count
    retval = loader_main(cs, spi_base, params1, params2, params3);
    if (retval) {
        #if DEBUG_INFO
        printf("The write error code\r\n");
        #endif
        ret |= 0x2;
    } else {
        #if DEBUG_INFO
        printf("Write success\r\n");
        #endif
    }

    /* flash read */
    cs = READ_CMD;
    params1 = (uint32_t)read_buf;//buffer
    params2 = TEST_OFFSET;//offset
    params3 = TEST_COUNT;//count
    retval = loader_main(cs, spi_base, params1, params2, params3);
    if (retval) {
        #if DEBUG_INFO
        printf("The read error code\r\n");
        #endif
        ret |= 0x4;
    } else {
        #if DEBUG_INFO
        printf("read success\r\n");
        #endif
    }

    /* verify */
    if (memory_compare(write_buf, read_buf, TEST_COUNT)) {
        #if DEBUG_INFO
        printf("Test fail\r\n");
        #endif
        ret |= 0x5;
    } else {
        #if DEBUG_INFO
        printf("Test pass\r\n");
        #endif
    }

    return ret;
}
