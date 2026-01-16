/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 by Aidan MacDonald
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "sdmmc-stm32h7.h"
#include "kernel.h"
#include "panic.h"
#include "regs/stm32h743/rcc.h"
#include "regs/stm32h743/sdmmc.h"
#include <string.h>

/* cmd_wait flags */
#define WAIT_CMD   0x01
#define WAIT_DATA  0x02

/* Maximum number of bytes for 1 transfer */
#define MAX_DATA_LEN \
    (BM_SDMMC_DLENR_DATALENGTH >> BP_SDMMC_DLENR_DATALENGTH)

/* IRQs for command phase */
#define CMD_SUCCESS_BITS \
    __reg_orm(SDMMC_STAR, CMDSENT, CMDREND)
#define CMD_ERROR_BITS \
    __reg_orm(SDMMC_STAR, CTIMEOUT, CCRCFAIL)
#define CMD_END_BITS \
    (CMD_SUCCESS_BITS | CMD_ERROR_BITS)

/* IRQs for data phase */
#define DATA_SUCCESS_BITS \
    __reg_orm(SDMMC_STAR, DATAEND)
#define DATA_ERROR_BITS \
    __reg_orm(SDMMC_STAR, IDMATE, DTIMEOUT, DABORT, DCRCFAIL, TXUNDERR, RXOVERR)
#define DATA_END_BITS \
    (DATA_SUCCESS_BITS | DATA_ERROR_BITS)

static size_t get_sdmmc_bus_freq(uint32_t clock)
{
    switch (clock)
    {
    case SDMMC_BUS_CLOCK_400KHZ: return 400000;
    case SDMMC_BUS_CLOCK_25MHZ:  return 25000000;
    case SDMMC_BUS_CLOCK_50MHZ:  return 50000000;
    default:                     return 0;
    }
}

static size_t abs_diff(size_t a, size_t b)
{
    return a > b ? a - b : b - a;
}

static bool stm32h7_sdmmc_is_powered_off(struct stm32h7_sdmmc_controller *ctl)
{
    uint32_t pwrctrl = reg_readlf(ctl->regs, SDMMC_POWER, PWRCTRL);

    return pwrctrl == BV_SDMMC_POWER_PWRCTRL_POWER_CYCLE;
}

void stm32h7_reset_sdmmc1(void)
{
    reg_writef(RCC_AHB3RSTR, SDMMC1RST(1));
    reg_writef(RCC_AHB3RSTR, SDMMC1RST(0));
}

void stm32h7_sdmmc_init(struct stm32h7_sdmmc_controller *ctl,
                        uint32_t instance,
                        const struct stm32_clock *clock,
                        void (*reset_sdmmc)(void),
                        void (*vcc_enable)(bool))
{
    memset(ctl, 0, sizeof(ctl));

    ctl->regs = instance;
    ctl->clock = clock;
    ctl->reset_sdmmc = reset_sdmmc;
    ctl->vcc_enable = vcc_enable;

    semaphore_init(&ctl->sem, 1, 0);
}

void stm32h7_sdmmc_set_power_enabled(void *controller, bool enabled)
{
    struct stm32h7_sdmmc_controller *ctl = controller;

    if (enabled)
    {
        /* Enable VCC */
        if (ctl->vcc_enable)
            ctl->vcc_enable(true);

        /* Stay in power-off state for >= 1ms */
        reg_writelf(ctl->regs, SDMMC_POWER, PWRCTRL_V(POWER_OFF));
        sleep(1);

        /* Bus clock is now needed, so enable kernel clock */
        stm32_clock_enable(ctl->clock);

        /* Configure bus parameters */
        stm32h7_sdmmc_set_bus_width(ctl, SDMMC_BUS_WIDTH_1BIT);
        stm32h7_sdmmc_set_bus_clock(ctl, SDMMC_BUS_CLOCK_400KHZ);
        reg_writelf(ctl->regs, SDMMC_CLKCR, PWRSAV(0));

        /* Power on and wait for >= 74 SDMMC clock cycles (= 185us) */
        reg_writelf(ctl->regs, SDMMC_POWER, PWRCTRL_V(POWER_ON));
        udelay(200);

        /* Automatically stop clock when bus is not in use */
        reg_writelf(ctl->regs, SDMMC_CLKCR, PWRSAV(1), HWFC_EN(1));
    }
    else
    {
        /* Controller reset */
        ctl->reset_sdmmc();

        /*
         * Disable kernel clock while powered down. The SDMMC block
         * diagrams from the reference manual show that sdmmc_ker_ck
         * is only used for the clock management subunit. Most likely
         * the kernel clock isn't needed if the clock output is static
         * and the bus is powered down; some quick testing shows this
         * seems to be true.
         */
        stm32_clock_disable(ctl->clock);

        /* Disable VCC */
        if (ctl->vcc_enable)
            ctl->vcc_enable(false);

        /* Stay in power cycle state for >= 1ms */
        reg_writelf(ctl->regs, SDMMC_POWER, PWRCTRL_V(POWER_CYCLE));
        sleep(1);
    }
}

void stm32h7_sdmmc_set_bus_width(void *controller, uint32_t width)
{
    struct stm32h7_sdmmc_controller *ctl = controller;

    /* Ignore requests from sdmmc_host while powered off */
    if (stm32h7_sdmmc_is_powered_off(ctl))
        return;

    if (width == SDMMC_BUS_WIDTH_1BIT)
        reg_writelf(ctl->regs, SDMMC_CLKCR, WIDBUS_V(1BIT));
    else if (width == SDMMC_BUS_WIDTH_4BIT)
        reg_writelf(ctl->regs, SDMMC_CLKCR, WIDBUS_V(4BIT));
    else if (width == SDMMC_BUS_WIDTH_8BIT)
        reg_writelf(ctl->regs, SDMMC_CLKCR, WIDBUS_V(8BIT));
    else
        panicf("%s", __func__);
}

void stm32h7_sdmmc_set_bus_clock(void *controller, uint32_t clock)
{
    struct stm32h7_sdmmc_controller *ctl = controller;

    /* Ignore requests from sdmmc_host while powered off */
    if (stm32h7_sdmmc_is_powered_off(ctl))
        return;

    size_t ker_freq = stm32_clock_get_frequency(ctl->clock);
    size_t bus_freq = get_sdmmc_bus_freq(clock);
    if (!bus_freq)
        panicf("%s", __func__);

    size_t div[2];
    size_t freq[2];

    /* Divider only supports even values, or can be bypassed (div=1) */
    div[0] = ker_freq / bus_freq;
    if (div[0] <= 1)
    {
        div[0] = 1;
        div[1] = 2;
    }
    else
    {
        div[0] &= ~1u;
        div[1] = div[0] + 2;
    }

    for (int i = 0; i < 2; ++i)
        freq[i] = ker_freq / div[i];

    /* Pick divider with lowest error */
    int idx;
    if (abs_diff(freq[0], bus_freq) < abs_diff(freq[1], bus_freq))
        idx = 0;
    else
        idx = 1;

    /* Set divider */
    ctl->bus_freq = freq[idx];
    reg_writelf(ctl->regs, SDMMC_CLKCR,
                SELCLKRX_V(SDMMC_IO_IN_CK),
                BUSSPEED_V(SLOW),
                DDR(0),
                NEGEDGE(0),
                CLKDIV(div[idx] / 2));
}

int stm32h7_sdmmc_submit_command(void *controller,
                                 const struct sdmmc_host_command *cmd,
                                 struct sdmmc_host_response *resp)
{
    struct stm32h7_sdmmc_controller *ctl = controller;

    uint32_t maskr = CMD_ERROR_BITS;
    uint32_t cmdr = __reg_orf(SDMMC_CMDR, CPSMEN(1), CMDINDEX(cmd->command));
    uint32_t cmd_wait = WAIT_CMD;

    void *buff_addr = cmd->buffer;
    size_t buff_size = cmd->nr_blocks * cmd->block_len;

    /*
     * Handle response length setting
     */
    switch (SDMMC_RESP_LENGTH(cmd->flags))
    {
    case SDMMC_RESP_NONE:
        reg_vwritef(cmdr, SDMMC_CMDR, WAITRESP_V(NONE));
        reg_vwritef(maskr, SDMMC_MASKR, CMDSENT(1));
        break;

    case SDMMC_RESP_SHORT:
        if (cmd->flags & SDMMC_RESP_NOCRC)
        {
            reg_vwritef(cmdr, SDMMC_CMDR, WAITRESP_V(SHORT_NOCRC));
            reg_vwritef(maskr, SDMMC_MASKR, CCRCFAIL(0));
        }
        else
        {
            reg_vwritef(cmdr, SDMMC_CMDR, WAITRESP_V(SHORT));
        }

        reg_vwritef(maskr, SDMMC_MASKR, CMDREND(1));
        break;

    case SDMMC_RESP_LONG:
        reg_vwritef(cmdr, SDMMC_CMDR, WAITRESP_V(LONG));
        reg_vwritef(maskr, SDMMC_MASKR, CMDREND(1));
        break;

    default:
        panicf("%s: bad resp mode", __func__);
        break;
    }

    /*
     * Handle commands with data transfers
     */
    if (SDMMC_DATA_PRESENT(cmd->flags))
    {
        if ((uintptr_t)buff_addr & (CACHEALIGN_SIZE - 1))
            panicf("%s: unaligned buffer", __func__);

        if (buff_size > MAX_DATA_LEN)
            panicf("%s: buffer too big", __func__);

        /* Set block size */
        uint32_t dctrl = 0;
        uint32_t dblocksize = find_first_set_bit(cmd->block_len);
        if (dblocksize > 14 || (cmd->block_len & (cmd->block_len - 1)))
            panicf("%s: incorrect block size", __func__);

        reg_vwritef(dctrl, SDMMC_DCTRL, DBLOCKSIZE(dblocksize));

        /* Set data direction & perform needed cache operations */
        if (SDMMC_DATA_DIR(cmd->flags) == SDMMC_DATA_WRITE)
        {
            commit_dcache_range(cmd->buffer, buff_size);
            reg_vwritef(dctrl, SDMMC_DCTRL, DTDIR(0));
        }
        else
        {
            discard_dcache_range(cmd->buffer, buff_size);
            reg_vwritef(dctrl, SDMMC_DCTRL, DTDIR(1));
        }

        /* Set buffer address in IDMA */
        reg_varl(ctl->regs, SDMMC_IDMABASE0R) = (uintptr_t)buff_addr;
        reg_assignlf(ctl->regs, SDMMC_IDMACTRLR, IDMAEN(1));

        /* Use a 10 second timeout (DTIMER is in units of bus clocks) */
        reg_varl(ctl->regs, SDMMC_DTIMER) = 10 * ctl->bus_freq;
        reg_varl(ctl->regs, SDMMC_DLENR) = buff_size;

        /* DCTRL must be written last */
        reg_varl(ctl->regs, SDMMC_DCTRL) = dctrl;

        /* Enable data phase */
        reg_vwritef(cmdr, SDMMC_CMDR, CMDTRANS(1));
        maskr |= DATA_END_BITS;
        cmd_wait |= WAIT_DATA;
    }
    else
    {
        /* Disable data transfer */
        reg_assignlf(ctl->regs, SDMMC_IDMACTRLR, IDMAEN(0));
        reg_varl(ctl->regs, SDMMC_DTIMER) = 0;
        reg_varl(ctl->regs, SDMMC_DLENR) = 0;
        reg_varl(ctl->regs, SDMMC_DCTRL) = 0;
    }

    /*
     * Set CMDSTOP bit for CMD12 (stop transmission) command;
     * this is needed to stop the DPSM in case of an error in
     * the previous data transfer command.
     */
    if (cmd->command == SD_STOP_TRANSMISSION)
        reg_vwritef(cmdr, SDMMC_CMDR, CMDSTOP(1));

    /* Update controller state */
    ctl->cmd_resp = resp;
    ctl->cmd_wait = cmd_wait;
    ctl->cmd_error = SDMMC_STATUS_OK;
    ctl->need_cmd12 = false;

    /* Barrier to ensure state is written before IRQ handler can run */
    membarrier();

    /* Unmask IRQs and begin command execution */
    reg_varl(ctl->regs, SDMMC_MASKR) = maskr;
    reg_varl(ctl->regs, SDMMC_ARGR) = cmd->argument;
    reg_varl(ctl->regs, SDMMC_CMDR) = cmdr;

    /* Wait for command completion */
    semaphore_wait(&ctl->sem, TIMEOUT_BLOCK);

    /*
     * Discard data from speculative reads that may have
     * accessed the buffer during the DMA transfer.
     */
    int cmd_error = ctl->cmd_error;
    if (cmd_error == SDMMC_STATUS_OK)
    {
        if (SDMMC_DATA_DIR(cmd->flags) == SDMMC_DATA_READ)
            discard_dcache_range(buff_addr, buff_size);
    }

    /*
     * If a data transfer command fails we need to issue CMD12
     * to stop DMA before returning. There is no other way to
     * abort the DMA transfer.
     */
    if (ctl->need_cmd12)
    {
        static const struct sdmmc_host_command cmd12 = {
            .command = SD_STOP_TRANSMISSION,
            .flags   = SDMMC_RESP_SHORT | SDMMC_RESP_BUSY,
        };

        /*
         * Recursion depth is limited to 1 because this can
         * only happen due to a failed data transfer command,
         * and CMD12 is not a data transfer command.
         */
        stm32h7_sdmmc_submit_command(ctl, &cmd12, NULL);
    }

    /* Return error from original command */
    return cmd_error;
}

void stm32h7_sdmmc_abort_command(void *controller)
{
    struct stm32h7_sdmmc_controller *ctl = controller;

    /* Prevent the IRQ handler from preempting us */
    reg_varl(ctl->regs, SDMMC_MASKR) = 0;
    arm_dsb();

    /*
     * Cancel any waiting command, including asking for
     * CMD12 if we need to abort DMA.
     */
    if (ctl->cmd_wait)
    {
        if (ctl->cmd_wait & WAIT_DATA)
            ctl->need_cmd12 = true;

        ctl->cmd_wait = 0;
        semaphore_release(&ctl->sem);
    }
}

void stm32h7_sdmmc_irq_handler(struct stm32h7_sdmmc_controller *ctl)
{
    uint32_t star = reg_readl(ctl->regs, SDMMC_STAR);
    uint32_t maskr = reg_readl(ctl->regs, SDMMC_MASKR);
    uint32_t icr = 0;

    if (!ctl->cmd_wait)
        panicf("sdmmc_irq: not waiting: %08lx", star);

    /*
     * Ignore interrupts which we haven't enabled; this is needed
     * to ensure the correct interrupt is used to detect command
     * completion (CMDSENT or CMDREND).
     */
    star &= maskr;

    if (ctl->cmd_wait & WAIT_CMD)
    {
        if (star & CMD_END_BITS)
        {
            if (reg_vreadf(star, SDMMC_STAR, CTIMEOUT))
                ctl->cmd_error = SDMMC_STATUS_TIMEOUT;
            else if (reg_vreadf(star, SDMMC_STAR, CCRCFAIL))
                ctl->cmd_error = SDMMC_STATUS_INVALID_CRC;

            /* Copy the response, if present */
            if (ctl->cmd_resp && reg_vreadf(star, SDMMC_STAR, CMDREND))
            {
                ctl->cmd_resp->data[0] = reg_readl(ctl->regs, SDMMC_RESPR(0));
                ctl->cmd_resp->data[1] = reg_readl(ctl->regs, SDMMC_RESPR(1));
                ctl->cmd_resp->data[2] = reg_readl(ctl->regs, SDMMC_RESPR(2));
                ctl->cmd_resp->data[3] = reg_readl(ctl->regs, SDMMC_RESPR(3));
                ctl->cmd_resp = NULL;
            }

            ctl->cmd_wait &= ~WAIT_CMD;
            icr |= CMD_END_BITS;
        }
    }

    if (ctl->cmd_wait & WAIT_DATA)
    {
        if ((star & DATA_END_BITS) || ctl->cmd_error)
        {
            if (ctl->cmd_error)
            {
                /*
                 * An error in the command phase of a data transfer
                 * command may leave the DPSM active & DMA ongoing.
                 * Set a flag so submit_command() can handle error
                 * recovery.
                 */
                ctl->need_cmd12 = true;
            }
            else
            {
                if (reg_vreadf(star, SDMMC_STAR, DTIMEOUT))
                    ctl->cmd_error = SDMMC_STATUS_TIMEOUT;
                else if (reg_vreadf(star, SDMMC_STAR, DCRCFAIL))
                    ctl->cmd_error = SDMMC_STATUS_INVALID_CRC;
                else if (reg_vreadf(star, SDMMC_STAR, DABORT))
                    ctl->cmd_error = SDMMC_STATUS_ERROR;
                else if (reg_vreadf(star, SDMMC_STAR, IDMATE))
                    panicf("sdmmc dma err: %08lx", reg_readl(ctl->regs, SDMMC_IDMABASE0R));
                else if (star & DATA_ERROR_BITS)
                    panicf("sdmmc data error: %08lx", star);
            }

            ctl->cmd_wait &= ~WAIT_DATA;
            icr |= DATA_END_BITS;
        }
    }

    /*
     * Each interrupt should complete at least one phase
     * but in error cases we can get spurious interrupts.
     */
    if (icr)
    {
        /* Clear handled interrupts */
        reg_varl(ctl->regs, SDMMC_MASKR) &= ~icr;
        reg_varl(ctl->regs, SDMMC_ICR) = icr;
    }

    /* Signal command complete if there are no more phases to wait for */
    if (ctl->cmd_wait == 0)
        semaphore_release(&ctl->sem);
}
