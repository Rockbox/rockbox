/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by amaury Pouly
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
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
#ifndef __DCP_IMX233_H__
#define __DCP_IMX233_H__

#include "cpu.h"
#include "system.h"
#include "system-target.h"

#define HW_DCP_BASE         0x80028000

/* channels */
#define HW_DCP_CH(x)    (x)
#define HW_DCP_NUM_CHANNELS 4
#define HW_DCP_CSC      8

/* ciphers */
#define HW_DCP_CIPHER_AES128    1

/* hash */
#define HW_DCP_HASH_SHA1    1
#define HW_DCP_HASH_CRC32   2

#define HW_DCP_CTRL         (*(volatile uint32_t *)(HW_DCP_BASE + 0x0))
#define HW_DCP_CTRL__CHANNEL_INTERRUPT_ENABLE_BP    0
#define HW_DCP_CTRL__CHANNEL_INTERRUPT_ENABLE_BM    0xf
#define HW_DCP_CTRL__CHANNEL_INTERRUPT_ENABLE(x)    (1 << (x))
#define HW_DCP_CTRL__CSC_INTERRUPT_ENABLE   (1 << 8)
#define HW_DCP_CTRL__ENABLE_CONTEXT_SWITCHING   (1 << 21)
#define HW_DCP_CTRL__ENABLE_CONTEXT_CACHING (1 << 22)
#define HW_DCP_CTRL__GATHER_RESIDUAL_WRITES (1 << 23)
#define HW_DCP_CTRL__PRESENT_CSC    (1 << 28)
#define HW_DCP_CTRL__PRESENT_CRYPTO (1 << 29)

#define HW_DCP_STAT         (*(volatile uint32_t *)(HW_DCP_BASE + 0x10))
#define HW_DCP_STAT__IRQ_BP 0
#define HW_DCP_STAT__IRQ_BM 0xf
#define HW_DCP_STAT__IRQ(x) (1 << (x))
#define HW_DCP_STAT__CSCIRQ (1 << 8)
#define HW_DCP_STAT__READY_CHANNELS_BP  16
#define HW_DCP_STAT__READY_CHANNELS_BM  (0xff << 16)
#define HW_DCP_STAT__READY_CHANNELS(x)  (1 << (16 + (x)))
#define HW_DCP_STAT__CUR_CHANNEL_BP 24
#define HW_DCP_STAT__CUR_CHANNEL_BM (0xf << 24)
#define HW_DCP_STAT__OTP_KEY_READY  (1 << 28)

#define HW_DCP_CHANNELCTRL  (*(volatile uint32_t *)(HW_DCP_BASE + 0x20))
#define HW_DCP_CHANNELCTRL__ENABLE_CHANNEL_BP   0
#define HW_DCP_CHANNELCTRL__ENABLE_CHANNEL_BM   0xff
#define HW_DCP_CHANNELCTRL__ENABLE_CHANNEL(x)   (1 << (x))
#define HW_DCP_CHANNELCTRL__HIGH_PRIORITY_CHANNEL_BP    8
#define HW_DCP_CHANNELCTRL__HIGH_PRIORITY_CHANNEL_BM    (0xff << 8)
#define HW_DCP_CHANNELCTRL__HIGH_PRIORITY_CHANNEL(x)    (1 << (8 + (x)))
#define HW_DCP_CHANNELCTRL__CH0_IRQ_MERGED  (1 << 16)
#define HW_DCP_CHANNELCTRL__CSC_PRIORITY_BP 17
#define HW_DCP_CHANNELCTRL__CSC_PRIORITY_BM (3 << 17)

#define HW_DCP_CAPABILITY0  (*(volatile uint32_t *)(HW_DCP_BASE + 0x30))
#define HW_DCP_CAPABILITY0__NUM_KEYS_BP 0
#define HW_DCP_CAPABILITY0__NUM_KEYS_BM 0xff
#define HW_DCP_CAPABILITY0__NUM_CHANNELS_BP 8
#define HW_DCP_CAPABILITY0__NUM_CHANNELS_BM 0xf00
#define HW_DCP_CAPABILITY0__ENABLE_TZONE    (1 << 30)
#define HW_DCP_CAPABILITY0__DISABLE_DECRYPT (1 << 31)

#define HW_DCP_CAPABILITY1  (*(volatile uint32_t *)(HW_DCP_BASE + 0x40))
#define HW_DCP_CAPABILITY1__CIPHER_ALGORITHMS_BP    0
#define HW_DCP_CAPABILITY1__CIPHER_ALGORITHMS_BM    0xffff
#define HW_DCP_CAPABILITY1__HASH_ALGORITHMS_BP      16
#define HW_DCP_CAPABILITY1__HASH_ALGORITHMS_BM      0xffff0000

#define HW_DCP_CONTEXT      (*(volatile uint32_t *)(HW_DCP_BASE + 0x50))

#define HW_DCP_KEY          (*(volatile uint32_t *)(HW_DCP_BASE + 0x60))

#define HW_DCP_KEYDATA      (*(volatile uint32_t *)(HW_DCP_BASE + 0x70))

#define HW_DCP_PACKET0      (*(volatile uint32_t *)(HW_DCP_BASE + 0x80))

#define HW_DCP_PACKET1      (*(volatile uint32_t *)(HW_DCP_BASE + 0x90))

#define HW_DCP_PACKET2      (*(volatile uint32_t *)(HW_DCP_BASE + 0xa0))

#define HW_DCP_PACKET3      (*(volatile uint32_t *)(HW_DCP_BASE + 0xb0))

#define HW_DCP_PACKET4      (*(volatile uint32_t *)(HW_DCP_BASE + 0xc0))

#define HW_DCP_PACKET5      (*(volatile uint32_t *)(HW_DCP_BASE + 0xd0))

#define HW_DCP_PACKET6      (*(volatile uint32_t *)(HW_DCP_BASE + 0xe0))

#define HW_DCP_CHxCMDPTR(x) (*(volatile uint32_t *)(HW_DCP_BASE + 0x100 + (x) * 0x40))

#define HW_DCP_CHxSEMA(x)   (*(volatile uint32_t *)(HW_DCP_BASE + 0x110 + (x) * 0x40))
#define HW_DCP_CHxSEMA__INCREMENT_BP    0
#define HW_DCP_CHxSEMA__INCREMENT_BM    0xff
#define HW_DCP_CHxSEMA__VALUE_BP        16
#define HW_DCP_CHxSEMA__VALUE_BM        0xff0000

#define HW_DCP_CHxSTAT(x)   (*(volatile uint32_t *)(HW_DCP_BASE + 0x120 + (x) * 0x40))

#define HW_DCP_CHxOPTS(x)   (*(volatile uint32_t *)(HW_DCP_BASE + 0x130 + (x) * 0x40))
#define HW_DCP_CHxOPTS__RECOVERY_TIMER_BP   0
#define HW_DCP_CHxOPTS__RECOVERY_TIMER_BM   0xffff

#define HW_DCP_CSCCTRL0     (*(volatile uint32_t *)(HW_DCP_BASE + 0x300))
#define HW_DCP_CSCCTRL0__ENABLE     (1 << 0)
#define HW_DCP_CSCCTRL0__YUV_FORMAT_BP  4
#define HW_DCP_CSCCTRL0__YUV_FORMAT_BM  0xf0
#define HW_DCP_CSCCTRL0__YUV_FORMAT__YUV420 0x0
#define HW_DCP_CSCCTRL0__YUV_FORMAT__YUV422 0x2
#define HW_DCP_CSCCTRL0__RGB_FORMAT_BP  8
#define HW_DCP_CSCCTRL0__RGB_FORMAT_BM  0x300
#define HW_DCP_CSCCTRL0__RGB_FORMAT__RGB16_565  0x0
#define HW_DCP_CSCCTRL0__RGB_FORMAT__YCbCrI     0x1
#define HW_DCP_CSCCTRL0__RGB_FORMAT__RGB24      0x2
#define HW_DCP_CSCCTRL0__RGB_FORMAT__YUV422I    0x3
#define HW_DCP_CSCCTRL0__DELTA      (1 << 10)
#define HW_DCP_CSCCTRL0__SUBSAMPLE  (1 << 11)
#define HW_DCP_CSCCTRL0__ROTATE     (1 << 12)
#define HW_DCP_CSCCTRL0__SCALE      (1 << 13)
#define HW_DCP_CSCCTRL0__UPSAMPLE   (1 << 14)
#define HW_DCP_CSCCTRL0__CLIP       (1 << 15)

#define HW_DCP_CSCSTAT      (*(volatile uint32_t *)(HW_DCP_BASE + 0x310))
#define HW_DCP_CSCSTAT__COMPLETE    (1 << 0)
#define HW_DCP_CSCSTAT__ERROR_SETUP (1 << 2)
#define HW_DCP_CSCSTAT__ERROR_SRC   (1 << 4)
#define HW_DCP_CSCSTAT__ERROR_DST   (1 << 5)
#define HW_DCP_CSCSTAT__ERROR_PAGEFAULT (1 << 6)
#define HW_DCP_CSCSTAT__ERROR_CODE_BP   16
#define HW_DCP_CSCSTAT__ERROR_CODE_BM   (0xff << 16)

#define HW_DCP_CSCOUTBUFPARAM   (*(volatile uint32_t *)(HW_DCP_BASE + 0x320))
#define HW_DCP_CSCOUTBUFPARAM__LINE_SIZE_BP 0
#define HW_DCP_CSCOUTBUFPARAM__LINE_SIZE_BM 0xfff
#define HW_DCP_CSCOUTBUFPARAM__FIELD_SIZE_BP    12
#define HW_DCP_CSCOUTBUFPARAM__FIELD_SIZE_BM    0xfff000

#define HW_DCP_CSCINBUFPARAM    (*(volatile uint32_t *)(HW_DCP_BASE + 0x330))
#define HW_DCP_CSCINBUFPARAM__LINE_SIZE_BP  0
#define HW_DCP_CSCINBUFPARAM__LINE_SIZE_BM  0xfff

#define HW_DCP_CSCRGB       (*(volatile uint32_t *)(HW_DCP_BASE + 0x340))

#define HW_DCP_CSCLUMA      (*(volatile uint32_t *)(HW_DCP_BASE + 0x350))

#define HW_DCP_CSCCHROMAU   (*(volatile uint32_t *)(HW_DCP_BASE + 0x360))

#define HW_DCP_CSCCHROMAV   (*(volatile uint32_t *)(HW_DCP_BASE + 0x370))

#define HW_DCP_CSCCOEFF0    (*(volatile uint32_t *)(HW_DCP_BASE + 0x380))
#define HW_DCP_CSCCOEFF0__Y_OFFSET_BP   0
#define HW_DCP_CSCCOEFF0__Y_OFFSET_BM   0xff
#define HW_DCP_CSCCOEFF0__UV_OFFSET_BP  8
#define HW_DCP_CSCCOEFF0__UV_OFFSET_BM  0xff00
#define HW_DCP_CSCCOEFF0__C0_BP 16
#define HW_DCP_CSCCOEFF0__C0_BM 0x3ff0000

#define HW_DCP_CSCCOEFF1    (*(volatile uint32_t *)(HW_DCP_BASE + 0x390))
#define HW_DCP_CSCCOEFF1__C4_BP 0
#define HW_DCP_CSCCOEFF1__C4_BM 0x3ff
#define HW_DCP_CSCCOEFF1__C1_BP 16
#define HW_DCP_CSCCOEFF1__C1_BM 0x3ff0000

#define HW_DCP_CSCCOEFF2    (*(volatile uint32_t *)(HW_DCP_BASE + 0x3a0))
#define HW_DCP_CSCCOEFF2__C3_BP 0
#define HW_DCP_CSCCOEFF2__C3_BM 0x3ff
#define HW_DCP_CSCCOEFF2__C2_BP 16
#define HW_DCP_CSCCOEFF2__C2_BM 0x3ff0000

#define HW_DCP_CSCCLIP      (*(volatile uint32_t *)(HW_DCP_BASE + 0x3b0))
#define HW_DCP_CSCCLIP__WIDTH_BP    0
#define HW_DCP_CSCCLIP__WIDTH_BM    0xfff
#define HW_DCP_CSCCLIP__HEIGHT_BP   12
#define HW_DCP_CSCCLIP__HEIGHT_BM   0xfff000

#define HW_DCP_CSCXSCALE    (*(volatile uint32_t *)(HW_DCP_BASE + 0x3c0))
#define HW_DCP_CSCXSCALE__WIDTH_BP  0
#define HW_DCP_CSCXSCALE__WIDTH_BM  0xfff
#define HW_DCP_CSCXSCALE__FRAC_BP   12
#define HW_DCP_CSCXSCALE__FRAC_BM   0xfff000
#define HW_DCP_CSCXSCALE__INT_BP    24
#define HW_DCP_CSCXSCALE__INT_BM    0x3000000

#define HW_DCP_CSCYSCALE    (*(volatile uint32_t *)(HW_DCP_BASE + 0x3d0))
#define HW_DCP_CSCYSCALE__WIDTH_BP  0
#define HW_DCP_CSCYSCALE__WIDTH_BM  0xfff
#define HW_DCP_CSCYSCALE__FRAC_BP   12
#define HW_DCP_CSCYSCALE__FRAC_BM   0xfff000
#define HW_DCP_CSCYSCALE__INT_BP    24
#define HW_DCP_CSCYSCALE__INT_BM    0x3000000

#define HW_DCP_PAGETABLE    (*(volatile uint32_t *)(HW_DCP_BASE + 0x420))
#define HW_DCP_PAGETABLE__ENABLE    (1 << 0)
#define HW_DCP_PAGETABLE__FLUSH     (1 << 1)
#define HW_DCP_PAGETABLE__BASE_BP   2
#define HW_DCP_PAGETABLE__BASE_BM   0xfffffffc

struct imx233_dcp_packet_t
{
    uint32_t next;
    uint32_t ctrl0;
    uint32_t ctrl1;
    uint32_t src;
    uint32_t dst;
    uint32_t size;
    uint32_t payload;
    uint32_t status;
} __attribute__((packed));

#define HW_DCP_CTRL0__INTERRUPT_ENABLE  (1 << 0)
#define HW_DCP_CTRL0__DECR_SEMAPHORE    (1 << 1)
#define HW_DCP_CTRL0__CHAIN             (1 << 2)
#define HW_DCP_CTRL0__CHAIN_CONTINUOUS  (1 << 3)
#define HW_DCP_CTRL0__ENABLE_MEMCOPY    (1 << 4)
#define HW_DCP_CTRL0__ENABLE_CIPHER     (1 << 5)
#define HW_DCP_CTRL0__ENABLE_HASH       (1 << 6)
#define HW_DCP_CTRL0__ENABLE_BLIT       (1 << 7)
#define HW_DCP_CTRL0__CIPHER_ENCRYPT    (1 << 8)
#define HW_DCP_CTRL0__CIPHER_INIT       (1 << 9)
#define HW_DCP_CTRL0__OTP_KEY           (1 << 10)
#define HW_DCP_CTRL0__PAYLOAD_KEY       (1 << 11)
#define HW_DCP_CTRL0__HASH_INIT         (1 << 12)
#define HW_DCP_CTRL0__HASH_TERM         (1 << 13)
#define HW_DCP_CTRL0__HASH_CHECK        (1 << 14)
#define HW_DCP_CTRL0__HASH_OUTPUT       (1 << 15)
#define HW_DCP_CTRL0__CONSTANT_FILL     (1 << 16)
#define HW_DCP_CTRL0__TEST_SEMA_IRQ     (1 << 17)
#define HW_DCP_CTRL0__KEY_BYTESWAP      (1 << 18)
#define HW_DCP_CTRL0__KEY_WORDSWAP      (1 << 19)
#define HW_DCP_CTRL0__INPUT_BYTESWAP    (1 << 20)
#define HW_DCP_CTRL0__INPUT_WORDSWAP    (1 << 21)
#define HW_DCP_CTRL0__OUTPUT_BYTESWAP   (1 << 22)
#define HW_DCP_CTRL0__OUTPUT_WORDSWAP   (1 << 23)
#define HW_DCP_CTRL0__TAG_BP            24
#define HW_DCP_CTRL0__TAG_BM            (0xff << 24)

#define HW_DCP_CTRL1__CIPHER_SELECT_BP  0
#define HW_DCP_CTRL1__CIPHER_SELECT_BM  0xf
#define HW_DCP_CTRL1__CIPHER_MODE_BP    4
#define HW_DCP_CTRL1__CIPHER_MODE_BM    0xf0
#define HW_DCP_CTRL1__KEY_SELECT_BP     8
#define HW_DCP_CTRL1__KEY_SELECT_BM     0xff00
#define HW_DCP_CTRL1__FRAMEBUFFER_LENGTH_BP 0
#define HW_DCP_CTRL1__FRAMEBUFFER_LENGTH_BM 0xffff
#define HW_DCP_CTRL1__HASH_SELECT_BP    16
#define HW_DCP_CTRL1__HASH_SELECT_BM    0xf0000
#define HW_DCP_CTRL1__CIPHER_CONFIG_BP  24
#define HW_DCP_CTRL1__CIPHER_CONFIG_BM  (0xff << 24)

#define HW_DCP_SIZE__BLIT_WIDTH_BP  0
#define HW_DCP_SIZE__BLIT_WIDTH_BM  0xffff
#define HW_DCP_SIZE__NUMBER_LINES_BP    16
#define HW_DCP_SIZE__NUMBER_LINES_BM    0xffff0000

#define HW_DCP_STATUS__COMPLETE     (1 << 0)
#define HW_DCP_STATUS__HASH_MISMATCH    (1 << 1)
#define HW_DCP_STATUS__ERROR_SETUP  (1 << 2)
#define HW_DCP_STATUS__ERROR_PACKET (1 << 3)
#define HW_DCP_STATUS__ERROR_SRC    (1 << 4)
#define HW_DCP_STATUS__ERROR_DST    (1 << 5)
#define HW_DCP_STATUS__ERROR_CODE_BP    16
#define HW_DCP_STATUS__ERROR_CODE_BM    (0xff << 16)
#define HW_DCP_STATUS__TAG_BP       24
#define HW_DCP_STATUS__TAG_BM       (0xff << 24)

struct imx233_dcp_channel_info_t
{
    bool irq;
    bool irq_en;
    bool enable;
    bool high_priority;
    bool ready;
    int sema;
    uint32_t cmdptr;
    bool acquired;
};

struct imx233_dcp_csc_info_t
{
    bool irq;
    bool irq_en;
    bool enable;
    int priority;
};

struct imx233_dcp_info_t
{
    /* capabilities */
    bool has_crypto;
    bool has_csc;
    int num_keys;
    int num_channels;
    unsigned ciphers;
    unsigned hashs;
    /* global state */
    bool context_switching;
    bool context_caching;
    bool gather_writes;
    bool otp_key_ready;
    bool ch0_merged;
    /* channel state */
    struct imx233_dcp_channel_info_t channel[HW_DCP_NUM_CHANNELS];
    /* csc state */
    struct imx233_dcp_csc_info_t csc;
};

#define DCP_INFO_CAPABILITIES   (1 << 0)
#define DCP_INFO_GLOBAL_STATE   (1 << 1)
#define DCP_INFO_CHANNELS       (1 << 2)
#define DCP_INFO_CSC            (1 << 3)
#define DCP_INFO_ALL            0xf

enum imx233_dcp_error_t
{
    DCP_SUCCESS = 0,
    DCP_TIMEOUT = -1,
    DCP_ERROR_SETUP = -2,
    DCP_ERROR_PACKET = -3,
    DCP_ERROR_SRC = -4,
    DCP_ERROR_DST = -5,
    DCP_ERROR_CHAIN_IS_0  = -6,
    DCP_ERROR_NO_CHAIN = -7,
    DCP_ERROR_CONTEXT = -8,
    DCP_ERROR_PAYLOAD = -9,
    DCP_ERROR_MODE = -10,
    DCP_ERROR = -11
};

void imx233_dcp_init(void);
// return OBJ_WAIT_TIMEOUT on failure
int imx233_dcp_acquire_channel(int timeout);
void imx233_dcp_release_channel(int chan);
// doesn't check that channel is in use!
void imx233_dcp_reserve_channel(int channel);

enum imx233_dcp_error_t imx233_dcp_memcpy_ex(int channel, bool fill, const void *src, void *dst, size_t len);
enum imx233_dcp_error_t imx233_dcp_memcpy(bool fill, const void *src, void *dst, size_t len, int tmo);

enum imx233_dcp_error_t imx233_dcp_blit_ex(int channel, bool fill, const void *src, size_t w, size_t h, void *dst, size_t out_w);
enum imx233_dcp_error_t imx233_dcp_blit(bool fill, const void *src, size_t w, size_t h, void *dst, size_t out_w, int tmo);

struct imx233_dcp_info_t imx233_dcp_get_info(unsigned flags);

#endif // __DMA_IMX233_H__
