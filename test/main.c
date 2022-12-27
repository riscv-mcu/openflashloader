#include <stdio.h>

#define DEBUG_INFO          (0)

/*==== Loader ====*/
#define ERASE_CMD           (1)
#define WRITE_CMD           (2)
#define READ_CMD            (3)
#define PROBE_CMD           (4)

#define TEST_ERASE_SIZE     (0x10000)
#define TEST_OFFSET         (0x00)
#define TEST_COUNT          (512)

uint8_t write_buf[TEST_COUNT] = {0};
uint8_t read_buf[TEST_COUNT] = {0};

void data_init(uint8_t* buf, uint32_t len)
{
    for (int i = 0;i < len;i++) {
        buf[i] = i + 1;
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

extern int loader_main(uint32_t cs, uint32_t *spi_base, uint32_t params1, uint32_t params2, uint32_t params3);

void main(void)
{
    int flash_id = 0;
    int retval = 0;

    /* flash loader in/out params */
    uint32_t cs = 0x00;
    uint32_t *spi_base = (uint32_t*)0x10014000;
    uint32_t params1 = 0x00;
    uint32_t params2 = 0x00;
    uint32_t params3 = 0x00;

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
    } else {
        #if DEBUG_INFO
        printf("Test pass\r\n");
        #endif
    }

    while (1) {}
}
