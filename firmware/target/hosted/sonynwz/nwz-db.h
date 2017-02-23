/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2016 Amaury Pouly
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
#ifndef __NWZ_DB_H__
#define __NWZ_DB_H__

/** /!\ This file was automatically generated, DO NOT MODIFY IT DIRECTLY /!\ */

/* List of all known NVP nodes */
enum nwz_nvp_node_t
{
    NWZ_NVP_APD, /* application debug mode flag */
    NWZ_NVP_APP, /* application parameter */
    NWZ_NVP_BFD, /* btmw factory scdb */
    NWZ_NVP_BFP, /* btmw factory pair info */
    NWZ_NVP_BLF, /* browser log mode flag */
    NWZ_NVP_BML, /* btmw log mode flag */
    NWZ_NVP_BOK, /* beep ok flag */
    NWZ_NVP_BPR, /* bluetooth address | bluetooth parameter */
    NWZ_NVP_BTC, /* battery calibration */
    NWZ_NVP_BTI, /* boot image */
    NWZ_NVP_CLV, /* color variation */
    NWZ_NVP_CNG, /* aad key | aad/empr key */
    NWZ_NVP_CTR, /*  */
    NWZ_NVP_DBA, /* aad icv */
    NWZ_NVP_DBG, /*  */
    NWZ_NVP_DBI, /* dead battery image */
    NWZ_NVP_DBV, /* empr icv | empr key */
    NWZ_NVP_DCC, /* secure clock */
    NWZ_NVP_DOR, /* key mode (debug/release) */
    NWZ_NVP_E00, /* EMPR  0 */
    NWZ_NVP_E01, /* EMPR  1 */
    NWZ_NVP_E02, /* EMPR  2 */
    NWZ_NVP_E03, /* EMPR  3 */
    NWZ_NVP_E04, /* EMPR  4 */
    NWZ_NVP_E05, /* EMPR  5 */
    NWZ_NVP_E06, /* EMPR  6 */
    NWZ_NVP_E07, /* EMPR  7 */
    NWZ_NVP_E08, /* EMPR  8 */
    NWZ_NVP_E09, /* EMPR  9 */
    NWZ_NVP_E10, /* EMPR 10 */
    NWZ_NVP_E11, /* EMPR 11 */
    NWZ_NVP_E12, /* EMPR 12 */
    NWZ_NVP_E13, /* EMPR 13 */
    NWZ_NVP_E14, /* EMPR 14 */
    NWZ_NVP_E15, /* EMPR 15 */
    NWZ_NVP_E16, /* EMPR 16 */
    NWZ_NVP_E17, /* EMPR 17 */
    NWZ_NVP_E18, /* EMPR 18 */
    NWZ_NVP_E19, /* EMPR 19 */
    NWZ_NVP_E20, /* EMPR 20 */
    NWZ_NVP_E21, /* EMPR 21 */
    NWZ_NVP_E22, /* EMPR 22 */
    NWZ_NVP_E23, /* EMPR 23 */
    NWZ_NVP_E24, /* EMPR 24 */
    NWZ_NVP_E25, /* EMPR 25 */
    NWZ_NVP_E26, /* EMPR 26 */
    NWZ_NVP_E27, /* EMPR 27 */
    NWZ_NVP_E28, /* EMPR 28 */
    NWZ_NVP_E29, /* EMPR 29 */
    NWZ_NVP_E30, /* EMPR 30 */
    NWZ_NVP_E31, /* EMPR 31 */
    NWZ_NVP_EDW, /* quick shutdown flag */
    NWZ_NVP_ERI, /* update error image */
    NWZ_NVP_EXM, /* exception monitor mode */
    NWZ_NVP_FMP, /* fm parameter */
    NWZ_NVP_FNI, /* function information */
    NWZ_NVP_FPI, /*  */
    NWZ_NVP_FUI, /* update image */
    NWZ_NVP_FUP, /* firmware update flag */
    NWZ_NVP_FUR, /*  */
    NWZ_NVP_FVI, /*  */
    NWZ_NVP_GTY, /* getty mode flag */
    NWZ_NVP_HDI, /* hold image */
    NWZ_NVP_HLD, /* hold mode */
    NWZ_NVP_INS, /*  */
    NWZ_NVP_IPT, /* disable iptable flag */
    NWZ_NVP_KAS, /* key and signature */
    NWZ_NVP_LBI, /* low battery image */
    NWZ_NVP_LYR, /*  */
    NWZ_NVP_MAC, /* wifi mac address */
    NWZ_NVP_MCR, /* marlin crl */
    NWZ_NVP_MDK, /* marlin device key */
    NWZ_NVP_MDL, /* middleware parameter */
    NWZ_NVP_MID, /* model id */
    NWZ_NVP_MLK, /* marlin key */
    NWZ_NVP_MSC, /* mass storage class mode */
    NWZ_NVP_MSO, /* MSC only mode flag */
    NWZ_NVP_MTM, /* marlin time */
    NWZ_NVP_MUK, /* marlin user key */
    NWZ_NVP_NCP, /* noise cancel driver parameter */
    NWZ_NVP_NVR, /*  */
    NWZ_NVP_PCD, /* product code */
    NWZ_NVP_PCI, /* precharge image */
    NWZ_NVP_PRK, /*  */
    NWZ_NVP_PSK, /* bluetooth pskey */
    NWZ_NVP_PTS, /* wifi protected setup */
    NWZ_NVP_RBT, /*  */
    NWZ_NVP_RND, /* random data | wmt key */
    NWZ_NVP_RTC, /* rtc alarm */
    NWZ_NVP_SDC, /* SD Card export flag */
    NWZ_NVP_SDP, /* sound driver parameter */
    NWZ_NVP_SER, /* serial number */
    NWZ_NVP_SFI, /* starfish id */
    NWZ_NVP_SHE, /*  */
    NWZ_NVP_SHP, /* ship information */
    NWZ_NVP_SID, /* service id */
    NWZ_NVP_SKD, /* slacker id file */
    NWZ_NVP_SKT, /* slacker time */
    NWZ_NVP_SKU, /*  */
    NWZ_NVP_SLP, /* time out to sleep */
    NWZ_NVP_SPS, /* speaker ship info */
    NWZ_NVP_SYI, /* system information */
    NWZ_NVP_TR0, /* EKB 0 */
    NWZ_NVP_TR1, /* EKB 1 */
    NWZ_NVP_TST, /* test mode flag */
    NWZ_NVP_UBP, /* u-boot password */
    NWZ_NVP_UFN, /* update file name */
    NWZ_NVP_UMS, /*  */
    NWZ_NVP_UPS, /*  */
    NWZ_NVP_VRT, /* europe vol regulation flag */
    NWZ_NVP_COUNT /* Number of nvp nodes */
};

/* Invalid NVP index */
#define NWZ_NVP_INVALID     -1 /* Non-existent entry */
/* Number of models */
#define NWZ_MODEL_COUNT     184
/* Number of series */
#define NWZ_SERIES_COUNT    36

/* NVP node info */
struct nwz_nvp_info_t
{
    const char *name; /* Sony's name: "bti" */
    unsigned long size; /* Size in bytes */
    const char *desc; /* Description: "bootloader image" */
};

/* NVP index map (nwz_nvp_node_t -> index) */
typedef int nwz_nvp_index_t[NWZ_NVP_COUNT];

/* Model info */
struct nwz_model_info_t
{
    unsigned long mid; /* Model ID: first 4 bytes of the NVP mid entry */
    const char *name; /* Human name: "NWZ-E463" */
};

/* Series info */
struct nwz_series_info_t
{
    const char *codename; /* Rockbox codename: nwz-e460 */
    const char *name; /* Human name: "NWZ-E460 Series" */
    int mid_count; /* number of entries in mid_list */
    unsigned long *mid; /* List of model IDs */
    /* Pointer to a name -> index map, nonexistent entries map to NWZ_NVP_INVALID */
    nwz_nvp_index_t *nvp_index;
};

/* List of all NVP entries, indexed by nwz_nvp_node_t */
extern struct nwz_nvp_info_t nwz_nvp[NWZ_NVP_COUNT];
/* List of all models, sorted by increasing values of model ID */
extern struct nwz_model_info_t nwz_model[NWZ_MODEL_COUNT];
/* List of all series */
extern struct nwz_series_info_t nwz_series[NWZ_SERIES_COUNT];

#endif /* __NWZ_DB_H__ */
