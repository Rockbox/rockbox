/*
 * SPI interface driver for the DM320 SoC
 *
 * Copyright (C) 2007 shirour <mrobefan@gmail.com>
 * Copyright (C) 2007 Catalin Patulea <cat@vv.carleton.ca>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS IS'' AND   ANY EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO EVENT  SHALL   THE AUTHOR  BE  LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS OR SERVICES; LOSS OF
 *  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "kernel.h"
#include "system.h"
#include "spi.h"

#define GIO_TS_ENABLE  (1<<2)
#define GIO_RTC_ENABLE (1<<12)
#define GIO_BL_ENABLE  (1<<13)
#define GIO_LCD_ENABLE (1<<5)

struct mutex spi_mtx;

struct SPI_info {
    volatile unsigned short *setreg;
    volatile unsigned short *clrreg;
    int bit;
};

struct SPI_info spi_targets[] =
{
#ifndef CREATIVE_ZVx
    [SPI_target_TSC2100]   = { &IO_GIO_BITCLR1, &IO_GIO_BITSET1, GIO_TS_ENABLE },
    [SPI_target_RX5X348AB] = { &IO_GIO_BITSET0, &IO_GIO_BITCLR0, GIO_RTC_ENABLE},
    [SPI_target_BACKLIGHT] = { &IO_GIO_BITCLR1, &IO_GIO_BITSET1, GIO_BL_ENABLE },
#else
    [SPI_target_LTV250QV] =  { &IO_GIO_BITCLR2, &IO_GIO_BITSET2, GIO_LCD_ENABLE},
#endif
};

#define IO_SERIAL0_XMIT         (0x100)
#define IO_SERIAL0_MODE_SCLK    (1 << 10)

static void spi_disable_all_targets(void)
{
    int i;
    for(i=0;i<SPI_MAX_TARGETS;i++)
    {
        *spi_targets[i].clrreg = spi_targets[i].bit;
    }
}

int spi_block_transfer(enum SPI_target target,
                       const uint8_t *tx_bytes, unsigned int tx_size,
                             uint8_t *rx_bytes, unsigned int rx_size)
{
    mutex_lock(&spi_mtx);
    /* Activate the slave select pin */
    *spi_targets[target].setreg = spi_targets[target].bit;

    while (tx_size--)
    {
        /* Send one byte */
        IO_SERIAL0_TX_DATA = *tx_bytes++;

        /* Wait until transfer finished */
        while (IO_SERIAL0_RX_DATA & IO_SERIAL0_XMIT);
    }

    while (rx_size--)
    {
        /* Make the clock tick */
        IO_SERIAL0_TX_DATA = 0;

        /* Wait until transfer finished */
        unsigned short data;
        while ((data = IO_SERIAL0_RX_DATA) & IO_SERIAL0_XMIT);
        
        *rx_bytes++ = data & 0xff;
    }

    *spi_targets[target].clrreg = spi_targets[target].bit;
    
    mutex_unlock(&spi_mtx);
    return 0;
}

void spi_init(void)
{
    mutex_init(&spi_mtx);
    /* Set SCLK idle level = 0 */
    IO_SERIAL0_MODE |= IO_SERIAL0_MODE_SCLK;
    /* Enable TX */
    IO_SERIAL0_TX_ENABLE = 0x0001;
#ifndef CREATIVE_ZVx
    /* Set GIO 18 to output for touch screen slave enable */
    IO_GIO_DIR1 &= ~GIO_TS_ENABLE;
    /* Set GIO 12 to output for rtc slave enable */
    IO_GIO_DIR0 &= ~GIO_RTC_ENABLE;
#endif  
    spi_disable_all_targets(); /* make sure only one is ever enabled at a time */

}
