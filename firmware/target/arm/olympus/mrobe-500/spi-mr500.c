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

#include "system.h"

#define GIO_TS_ENABLE (1<<2)
#define clr_gio_enable() IO_GIO_BITSET1=GIO_TS_ENABLE
#define set_gio_enable() IO_GIO_BITCLR1=GIO_TS_ENABLE

int spi_block_transfer(const uint8_t *tx_bytes, unsigned int tx_size,
                             uint8_t *rx_bytes, unsigned int rx_size)
{
    /* Activate the slave select pin */
    set_gio_enable();

    while (tx_size--)
    {
        /* Send one byte */
        IO_SERIAL0_TX_DATA = *tx_bytes++;

        /* Wait until transfer finished */
        while (IO_SERIAL0_RX_DATA & 0x100);
    }

    while (rx_size--)
    {
        /* Make the clock tick */
        IO_SERIAL0_TX_DATA = 0;

        /* Wait until transfer finished */
        unsigned short data;
        while ((data = IO_SERIAL0_RX_DATA) & 0x100);
        
        *rx_bytes++ = data & 0xff;
    }

    clr_gio_enable();

    return 0;
}

void spi_init(void)
{
    /* Set SCLK idle level = 0 */
    IO_SERIAL0_MODE |= (1<<10);
    
    /* Enable TX */
    IO_SERIAL0_TX_ENABLE = 0x0001;

    /* Set GIO 18 to output for touch screen slave enable */
    IO_GIO_DIR1&=~GIO_TS_ENABLE;
    clr_gio_enable();
}
