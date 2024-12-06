#include "spi.h"

/* WARNING: NEVER USE GLOBAL VARIABLE IN THIS FILE */

/* Register offsets */
#define NUSPI_REG_SCKMODE           (0x04)
#define NUSPI_REG_FORCE             (0x0C)
#define NUSPI_REG_CSMODE            (0x18)
#define NUSPI_REG_FMT               (0x40)
#define NUSPI_REG_TXDATA            (0x48)
#define NUSPI_REG_RXDATA            (0x4C)
#define NUSPI_REG_FCTRL             (0x60)
#define NUSPI_REG_FFMT              (0x64)
#define NUSPI_REG_STATUS            (0x7C)
#define NUSPI_REG_RXEDGE            (0x80)
/* FCTRL register */
#define NUSPI_FCTRL_EN              (0x1)
#define NUSPI_INSN_CMD_EN           (0x1)
/* FMT register */
#define NUSPI_FMT_DIR(x)            (((x) & 0x1) << 3)
/* STATUS register */
#define NUSPI_STAT_BUSY             (0x1 << 0)
#define NUSPI_STAT_TXFULL           (0x1 << 4)
#define NUSPI_STAT_RXEMPTY          (0x1 << 5)
#define NUSPI_CSMODE_AUTO           (0)
#define NUSPI_CSMODE_HOLD           (2)
#define NUSPI_DIR_RX                (0)
#define NUSPI_DIR_TX                (1)

#define NUSPI_TX_TIMES_OUT          (500)
#define NUSPI_RX_TIMES_OUT          (500)

static inline void nuspi_read_reg(uint32_t volatile *spi_base, uint32_t offset, uint32_t *value)
{
    *value = spi_base[offset / 4];
}

static inline void nuspi_write_reg(uint32_t volatile *spi_base, uint32_t offset, uint32_t value)
{
    spi_base[offset / 4] = value;
}

static inline void nuspi_set_dir(uint32_t *spi_base, uint32_t dir)
{
    uint32_t fmt = 0;
    nuspi_read_reg(spi_base, NUSPI_REG_FMT, &fmt);
    nuspi_write_reg(spi_base, NUSPI_REG_FMT, (fmt & ~(NUSPI_FMT_DIR(0xFFFFFFFF))) | NUSPI_FMT_DIR(dir));
}

void spi_init(uint32_t *spi_base)
{
    /* clear rxfifo */
    uint32_t status = 0;
    while (1) {
        nuspi_read_reg(spi_base, NUSPI_REG_RXDATA, &status);
        nuspi_read_reg(spi_base, NUSPI_REG_STATUS, &status);
        if (status & NUSPI_STAT_RXEMPTY) {
            break;
        }
    }
    /* init register */
    nuspi_write_reg(spi_base, NUSPI_REG_SCKMODE, 0x0);
    nuspi_write_reg(spi_base, NUSPI_REG_FORCE, 0x3);
    nuspi_write_reg(spi_base, NUSPI_REG_FCTRL, 0x0);
    nuspi_write_reg(spi_base, NUSPI_REG_FMT, 0x80008);
    nuspi_write_reg(spi_base, NUSPI_REG_FFMT, 0x30007);
    nuspi_write_reg(spi_base, NUSPI_REG_RXEDGE, 0x0);
}

void spi_hw(uint32_t *spi_base, bool sel)
{
    uint32_t fctrl = 0;
    nuspi_read_reg(spi_base, NUSPI_REG_FCTRL, &fctrl);
    if (sel) {
        nuspi_write_reg(spi_base, NUSPI_REG_FCTRL, fctrl | NUSPI_FCTRL_EN);
    } else {
        nuspi_write_reg(spi_base, NUSPI_REG_FCTRL, fctrl & ~NUSPI_FCTRL_EN);
    }
}

void spi_cs(uint32_t *spi_base, bool sel)
{
    if (sel) {
        nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_HOLD);
    } else {
        nuspi_write_reg(spi_base, NUSPI_REG_CSMODE, NUSPI_CSMODE_AUTO);
    }
}

int spi_tx(uint32_t *spi_base, uint8_t *in, uint32_t len)
{
    uint32_t times_out = 0;
    uint32_t value = 0;
    nuspi_set_dir(spi_base, NUSPI_DIR_TX);
    for (int i = 0;i < len;i++) {
        times_out = NUSPI_TX_TIMES_OUT;
        while (times_out--) {
            nuspi_read_reg(spi_base, NUSPI_REG_STATUS, &value);
            if (!(value & NUSPI_STAT_TXFULL)) {
                break;
            }
        }
        if (0 >= times_out) {
            return RETURN_SPI_TX_ERROR;
        }
        nuspi_write_reg(spi_base, NUSPI_REG_TXDATA, in[i]);
    }
    times_out = NUSPI_TX_TIMES_OUT;
    while (times_out--) {
        nuspi_read_reg(spi_base, NUSPI_REG_STATUS, &value);
        if (0 == (value & NUSPI_STAT_BUSY)) {
            break;
        }
    }
    if (0 >= times_out) {
        return RETURN_SPI_TX_ERROR;
    }
    return 0;
}

int spi_rx(uint32_t *spi_base, uint8_t *out, uint32_t len)
{
    uint32_t times_out = 0;
    uint32_t value = 0;
    nuspi_set_dir(spi_base, NUSPI_DIR_RX);
    for (int i = 0;i < len;i++) {
        times_out = NUSPI_RX_TIMES_OUT;
        nuspi_write_reg(spi_base, NUSPI_REG_TXDATA, 0x00);
        while (times_out--) {
            nuspi_read_reg(spi_base, NUSPI_REG_STATUS, &value);
            if (!(value & NUSPI_STAT_RXEMPTY)) {
                break;
            }
        }
        if (0 >= times_out) {
            return RETURN_SPI_RX_ERROR;
        }
        nuspi_read_reg(spi_base, NUSPI_REG_RXDATA, &value);
        out[i] = value & 0xff;
    }
    return 0;
}
