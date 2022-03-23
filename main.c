#include <stdio.h>

/*==== Loader ====*/
#define ERASE_CMD			(1)
#define WRITE_CMD			(2)
#define READ_CMD			(3)
#define PROBE_CMD			(4)

#define TEST_OFFSET (0x00)
#define TEST_COUNT (0x10000)

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

void main(void)
{
    int flash_id = 0;
    int retval = 0;

    /* flash loader in/out params */
    int cs = 0x00;
    int spi_base = 0xF0040000;
    int params1 = 0x00;
    int params2 = 0x00;
    int params3 = 0x00;

    /* data init */
    data_init(write_buf, TEST_COUNT);

    /* flash init */
    cs = PROBE_CMD;
    flash_id = loader_main(cs, spi_base, params1, params2, params3);
    printf("The current Flash ID is %#x\r\n", flash_id);

    /* flash erase */
    cs = ERASE_CMD;
    params1 = TEST_OFFSET;//first_addr
    params2 = TEST_OFFSET + TEST_COUNT;//end_addr
    retval = loader_main(cs, spi_base, params1, params2, params3);
    if (retval) {
        printf("The erase error code is %#x\r\n", flash_id);
    } else {
        printf("Erase success\r\n");
    }

    /* flash write */
    cs = WRITE_CMD;
    params1 = write_buf;//src_address
    params2 = TEST_OFFSET;//write_offset
    params3 = TEST_COUNT;//write_count
    retval = loader_main(cs, spi_base, params1, params2, params3);
    if (retval) {
        printf("The write error code is %#x\r\n", flash_id);
    } else {
        printf("Write success\r\n");
    }

    /* flash write */
    cs = WRITE_CMD;
    params1 = read_buf;//dst_address
    params2 = TEST_OFFSET;//read_offset
    params3 = TEST_COUNT;//read_count
    retval = loader_main(cs, spi_base, params1, params2, params3);
    if (retval) {
        printf("The write error code is %#x\r\n", flash_id);
    } else {
        printf("Write success\r\n");
    }

    /* verify */
    if (memory_compare(write_buf, read_buf, TEST_COUNT)) {
        printf("Test fail\r\n");
    } else {
        printf("Test pass\r\n");
    }

    while (1) {}
}
