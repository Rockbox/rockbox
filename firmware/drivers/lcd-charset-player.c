/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Jens Arnold
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

#include "config.h"
#include "hwcompat.h"

#include "lcd-charcell.h"

int lcd_pattern_count;  /* actual number of user-definable hw patterns */

const struct xchar_info *xchar_info;
int xchar_info_size;  /* number of entries */       

enum {
    /* Standard ascii */
    XF_BACKSLASH = 0, XF_CIRCUMFLEX,    XF_GRAVEACCENT, XF_VERTICALBAR,
    XF_TILDE,
#ifndef BOOTLOADER
    /* Icons and special symbols */
    XF_ICON_UNKNOWN,  XF_ICON_BOOKMARK, XF_ICON_PLUGIN, XF_ICON_FOLDER,
    XF_ICON_FIRMWARE, XF_ICON_LANGUAGE, XF_ICON_AUDIO,  XF_ICON_WPS,
    XF_ICON_PLAYLIST, XF_ICON_TEXTFILE, XF_ICON_CONFIG,

    /* Latin1 */
    XF_INVEXCLAMATION, XF_CENTSIGN, XF_POUNDSIGN, XF_CURRENCY,
    XF_LEFTDBLANGLEQUOT, XF_MACRON, XF_PLUSMINUS, XF_SUPER2,
    XF_SUPER3, XF_MICRO, XF_MIDDLEDOT, XF_RIGHTDBLANGLEQUOT,
    XF_ONEQUARTER, XF_ONEHALF, XF_THREEQUARTERS, XF_INVQUESTION,
    XF_AGRAVE, XF_AACUTE, XF_ACIRCUMFLEX, XF_ATILDE,
    XF_ADIERESIS, XF_ARING, XF_AELIGATURE, XF_CCEDILLA,
    XF_EGRAVE, XF_EACUTE, XF_ECIRCUMFLEX, XF_EDIERESIS,
    XF_IGRAVE, XF_IACUTE, XF_ICIRCUMFLEX, XF_IDIERESIS,
    XF_ETH,    XF_NTILDE, XF_OGRAVE, XF_OACUTE,
    XF_OCIRCUMFLEX, XF_OTILDE, XF_ODIERESIS, XF_OSTROKE,
    XF_UGRAVE, XF_UACUTE, XF_UCIRCUMFLEX, XF_UDIERESIS,
    XF_YACUTE, XF_aGRAVE, XF_aACUTE, XF_aCIRCUMFLEX,
    XF_aTILDE, XF_aDIERESIS, XF_aRING, XF_aeLIGATURE,
    XF_cCEDILLA, XF_eGRAVE, XF_eACUTE, XF_eCIRCUMFLEX,
    XF_eDIERESIS, XF_iGRAVE, XF_iACUTE, XF_iCIRCUMFLEX,
    XF_iDIERESIS, XF_nTILDE, XF_oGRAVE, XF_oACUTE,
    XF_oCIRCUMFLEX, XF_oTILDE, XF_oDIERESIS, XF_DIVISION,
    XF_oSLASH, XF_uGRAVE, XF_uACUTE, XF_uCIRCUMFLEX,
    XF_uDIERESIS, XF_yACUTE, XF_yDIERESIS,

    /* Latin extended A */
    XF_aBREVE, XF_aOGONEK, XF_cACUTE, XF_cCARON,
    XF_dCARON, XF_dSTROKE, XF_eOGONEK, XF_eCARON,
    XF_GBREVE, XF_gBREVE, XF_IDOT, XF_DOTLESSi,
    XF_LSTROKE, XF_lSTROKE, XF_nACUTE, XF_nCARON,
    XF_ODBLACUTE, XF_oDBLACUTE, XF_RCARON, XF_rCARON,
    XF_sACUTE, XF_SCEDILLA, XF_sCEDILLA, XF_sCARON,
    XF_tCEDILLA, XF_tCARON, XF_uRING, XF_UDBLACUTE,
    XF_uDBLACUTE, XF_zACUTE, XF_zDOT, XF_zCARON,
#define XF_DSTROKE XF_ETH
#define XF_SACUTE  XF_sACUTE
#define XF_SCARON  XF_sCARON

    /* Greek */
    XF_GR_DELTA, XF_GR_THETA, XF_GR_LAMBDA, XF_GR_XI,
    XF_GR_PSI, XF_GR_alpha, XF_GR_alphaTONOS, XF_GR_gamma,
    XF_GR_epsilon, XF_GR_epsilonTONOS, XF_GR_zeta, XF_GR_eta,
    XF_GR_etaTONOS, XF_GR_iota, XF_GR_lambda, XF_GR_xi,
    XF_GR_rho, XF_GR_FINALsigma, XF_GR_sigma, XF_GR_upsilon,
    XF_GR_upsilonTONOS, XF_GR_chi, XF_GR_psi, XF_GR_omega,
    XF_GR_omegaTONOS,
#define XF_GR_ANOTELEIA XF_MIDDLEDOT
#define XF_GR_GAMMA     XF_CYR_GHE
#define XF_GR_PI        XF_CYR_PE
#define XF_GR_delta     XF_CYR_be
#define XF_GR_iotaTONOS XF_iACUTE
#define XF_GR_iotaDIA   XF_iDIERESIS
#define XF_GR_kappa     XF_CYR_ka
#define XF_GR_mu        XF_MICRO
#define XF_GR_pi        XF_CYR_pe
#define XF_GR_omicronTONOS XF_oACUTE
#define XF_GR_tau XF_CYR_te

    /* Cyrillic */
    XF_CYR_BE, XF_CYR_GHE, XF_CYR_DE, XF_CYR_ZHE,
    XF_CYR_ZE, XF_CYR_I, XF_CYR_SHORTI, XF_CYR_EL,
    XF_CYR_PE, XF_CYR_TSE, XF_CYR_CHE, XF_CYR_SHA,
    XF_CYR_SHCHA, XF_CYR_HARD, XF_CYR_YERU, XF_CYR_E,
    XF_CYR_YU, XF_CYR_YA, XF_CYR_be, XF_CYR_ve,
    XF_CYR_ghe, XF_CYR_de, XF_CYR_zhe, XF_CYR_ze,
    XF_CYR_i, XF_CYR_SHORTi, XF_CYR_ka, XF_CYR_el,
    XF_CYR_em, XF_CYR_en, XF_CYR_pe, XF_CYR_te,
    XF_CYR_tse, XF_CYR_che, XF_CYR_sha, XF_CYR_shcha,
    XF_CYR_hard, XF_CYR_yeru, XF_CYR_soft, XF_CYR_e,
    XF_CYR_yu, XF_CYR_ya,
#define XF_CYR_IEGRAVE XF_EGRAVE
#define XF_CYR_IO      XF_EDIERESIS
#define XF_CYR_YI      XF_IDIERESIS
#define XF_CYR_ieGRAVE XF_eGRAVE
#define XF_CYR_io      XF_eDIERESIS
#define XF_CYR_yi      XF_iDIERESIS

#endif
};

const struct xchar_info xchar_info_newlcd[] = {
    /* Standard ascii */
    {   0x20, 0, 0, 0x20 }, /*   */
    {   0x21, 0, 0, 0x21 }, /* ! */
    {   0x22, 0, 0, 0x22 }, /* " */
    {   0x23, 0, 0, 0x23 }, /* # */
    {   0x24, 0, 0, 0x24 }, /* $ */
    {   0x25, 0, 0, 0x25 }, /* % */
    {   0x26, 0, 0, 0x26 }, /* & */
    {   0x27, 0, 0, 0x27 }, /* ' */
    {   0x28, 0, 0, 0x28 }, /* ( */
    {   0x29, 0, 0, 0x29 }, /* ) */
    {   0x2a, 0, 0, 0x2a }, /* * */
    {   0x2b, 0, 0, 0x2b }, /* + */
    {   0x2c, 0, 0, 0x2c }, /* , */
    {   0x2d, 0, 0, 0x2d }, /* - */
    {   0x2e, 0, 0, 0x2e }, /* . */
    {   0x2f, 0, 0, 0x2f }, /* / */
    {   0x30, 0, 0, 0x30 }, /* 0 */
    {   0x31, 0, 0, 0x31 }, /* 1 */
    {   0x32, 0, 0, 0x32 }, /* 2 */
    {   0x33, 0, 0, 0x33 }, /* 3 */
    {   0x34, 0, 0, 0x34 }, /* 4 */
    {   0x35, 0, 0, 0x35 }, /* 5 */
    {   0x36, 0, 0, 0x36 }, /* 6 */
    {   0x37, 0, 0, 0x37 }, /* 7 */
    {   0x38, 0, 0, 0x38 }, /* 8 */
    {   0x39, 0, 0, 0x39 }, /* 9 */
    {   0x3a, 0, 0, 0x3a }, /* : */
    {   0x3b, 0, 0, 0x3b }, /* ; */
    {   0x3c, 0, 0, 0x3c }, /* < */
    {   0x3d, 0, 0, 0x3d }, /* = */
    {   0x3e, 0, 0, 0x3e }, /* > */
    {   0x3f, 0, 0, 0x3f }, /* ? */
    {   0x40, 0, 0, 0x40 }, /* @ */
    {   0x41, 0, 0, 0x41 }, /* A */
    {   0x42, 0, 0, 0x42 }, /* B */
    {   0x43, 0, 0, 0x43 }, /* C */
    {   0x44, 0, 0, 0x44 }, /* D */
    {   0x45, 0, 0, 0x45 }, /* E */
    {   0x46, 0, 0, 0x46 }, /* F */
    {   0x47, 0, 0, 0x47 }, /* G */
    {   0x48, 0, 0, 0x48 }, /* H */
    {   0x49, 0, 0, 0x49 }, /* I */
    {   0x4a, 0, 0, 0x4a }, /* J */
    {   0x4b, 0, 0, 0x4b }, /* K */
    {   0x4c, 0, 0, 0x4c }, /* L */
    {   0x4d, 0, 0, 0x4d }, /* M */
    {   0x4e, 0, 0, 0x4e }, /* N */
    {   0x4f, 0, 0, 0x4f }, /* O */
    {   0x50, 0, 0, 0x50 }, /* P */
    {   0x51, 0, 0, 0x51 }, /* Q */
    {   0x52, 0, 0, 0x52 }, /* R */
    {   0x53, 0, 0, 0x53 }, /* S */
    {   0x54, 0, 0, 0x54 }, /* T */
    {   0x55, 0, 0, 0x55 }, /* U */
    {   0x56, 0, 0, 0x56 }, /* V */
    {   0x57, 0, 0, 0x57 }, /* W */
    {   0x58, 0, 0, 0x58 }, /* X */
    {   0x59, 0, 0, 0x59 }, /* Y */
    {   0x5a, 0, 0, 0x5a }, /* Z */
    {   0x5b, 0, 0, 0x5b }, /* [ */
    {   0x5c, 0, 0, 0x12 }, /* \ */
    {   0x5d, 0, 0, 0x5d }, /* ] */
    {   0x5e, 0, 0, 0x5e }, /* ^ */
    {   0x5f, 0, 0, 0x5f }, /* _ */
    {   0x60, 0, 0, 0x60 }, /* ` */
    {   0x61, 0, 0, 0x61 }, /* a */
    {   0x62, 0, 0, 0x62 }, /* b */
    {   0x63, 0, 0, 0x63 }, /* c */
    {   0x64, 0, 0, 0x64 }, /* d */
    {   0x65, 0, 0, 0x65 }, /* e */
    {   0x66, 0, 0, 0x66 }, /* f */
    {   0x67, 0, 0, 0x67 }, /* g */
    {   0x68, 0, 0, 0x68 }, /* h */
    {   0x69, 0, 0, 0x69 }, /* i */
    {   0x6a, 0, 0, 0x6a }, /* j */
    {   0x6b, 0, 0, 0x6b }, /* k */
    {   0x6c, 0, 0, 0x6c }, /* l */
    {   0x6d, 0, 0, 0x6d }, /* m */
    {   0x6e, 0, 0, 0x6e }, /* n */
    {   0x6f, 0, 0, 0x6f }, /* o */
    {   0x70, 0, 0, 0x70 }, /* p */
    {   0x71, 0, 0, 0x71 }, /* q */
    {   0x72, 0, 0, 0x72 }, /* r */
    {   0x73, 0, 0, 0x73 }, /* s */
    {   0x74, 0, 0, 0x74 }, /* t */
    {   0x75, 0, 0, 0x75 }, /* u */
    {   0x76, 0, 0, 0x76 }, /* v */
    {   0x77, 0, 0, 0x77 }, /* w */
    {   0x78, 0, 0, 0x78 }, /* x */
    {   0x79, 0, 0, 0x79 }, /* y */
    {   0x7a, 0, 0, 0x7a }, /* z */
    {   0x7b, 0, 0, 0x7b }, /* { */
    {   0x7c, 0, 0, 0x7c }, /* | */
    {   0x7d, 0, 0, 0x7d }, /* } */
    {   0x7e, 0, 0, 0xf0 }, /* ~ */
    {   0x7f, 0, 0, 0xfe }, /* (full grid) */

#ifndef BOOTLOADER /* bootloader only supports pure ASCII */
    /* Latin 1 */
    {   0xa0, 0, 0, 0x20 }, /*   (non-breaking space) */
    {   0xa1, XF_INVEXCLAMATION, 1, 0x21 }, /* ¡ (inverted !) */
    {   0xa2, XF_CENTSIGN,       1, 0x63 }, /* ¢ (cent sign) */
    {   0xa3, XF_POUNDSIGN,      1, 0x4c }, /* £ (pound sign) */
    {   0xa4, XF_CURRENCY,       1, 0x6f }, /* ¤ (currency sign) */
    {   0xa5, 0, 0, 0x5c }, /* ¥ (yen sign) */

    {   0xa7, 0, 0, 0x15 }, /* § (paragraph sign) */

    {   0xab, 0, 0, 0x9e }, /* « (left double-angle quotation mark) */

    {   0xad, 0, 0, 0x2d }, /* ­ (soft hyphen) */

    {   0xaf, XF_MACRON, 1, 0x2d }, /* ¯ (macron) */

    {   0xb1, 0, 0, 0x95 }, /* ± (plus-minus sign) */
    {   0xb2, 0, 0, 0x99 }, /* ³ (superscript 2) */
    {   0xb3, 0, 0, 0x9a }, /* ³ (superscript 3) */

    {   0xb5, 0, 0, 0xe6 }, /* µ (micro sign) */
    {   0xb6, 0, 0, 0x14 }, /* ¶ (pilcrow sign) */
    {   0xb7, 0, 0, 0xa5 }, /* · (middle dot) */

    {   0xbb, 0, 0, 0x9f }, /* » (right double-angle quotation mark) */
    {   0xbc, 0, 0, 0x9c }, /* ¼ (one quarter) */
    {   0xbd, 0, 0, 0x9b }, /* ½ (one half) */
    {   0xbe, 0, 0, 0x9d }, /* ¾ (three quarters) */
    {   0xbf, XF_INVQUESTION, 1, 0x3f }, /* ¿ (inverted ?) */
    {   0xc0, XF_AGRAVE,      1, 0x41 }, /* À (A grave) */
    {   0xc1, XF_AACUTE,      1, 0x41 }, /* Á (A acute) */
    {   0xc2, XF_ACIRCUMFLEX, 1, 0x41 }, /* Â (A circumflex) */
    {   0xc3, XF_ATILDE,      1, 0x41 }, /* Ã (A tilde) */
    {   0xc4, XF_ADIERESIS,   1, 0x41 }, /* Ä (A dieresis) */
    {   0xc5, XF_ARING,       1, 0x41 }, /* Å (A with ring above) */
    {   0xc6, XF_AELIGATURE,  1, 0x41 }, /* Æ (AE ligature) */
    {   0xc7, XF_CCEDILLA,    1, 0x43 }, /* Ç (C cedilla) */
    {   0xc8, XF_EGRAVE,      1, 0x45 }, /* È (E grave) */
    {   0xc9, XF_EACUTE,      1, 0x45 }, /* É (E acute) */
    {   0xca, XF_ECIRCUMFLEX, 1, 0x45 }, /* Ê (E circumflex) */
    {   0xcb, XF_EDIERESIS,   1, 0x45 }, /* Ë (E dieresis) */
    {   0xcc, XF_IGRAVE,      1, 0x49 }, /* Ì (I grave) */
    {   0xcd, XF_IACUTE,      1, 0x49 }, /* Í (I acute) */
    {   0xce, XF_ICIRCUMFLEX, 1, 0x49 }, /* Î (I circumflex) */
    {   0xcf, XF_IDIERESIS,   1, 0x49 }, /* Ï (I dieresis) */
    {   0xd0, XF_ETH,         1, 0x44 }, /* Ð (ETH) */
    {   0xd1, XF_NTILDE,      1, 0x4e }, /* Ñ (N tilde) */
    {   0xd2, XF_OGRAVE,      1, 0x4f }, /* Ò (O grave) */
    {   0xd3, XF_OACUTE,      1, 0x4f }, /* Ó (O acute) */
    {   0xd4, XF_OCIRCUMFLEX, 1, 0x4f }, /* Ô (O circumflex) */
    {   0xd5, XF_OTILDE,      1, 0x4f }, /* Õ (O tilde) */
    {   0xd6, XF_ODIERESIS,   1, 0x4f }, /* Ö (O dieresis) */
    {   0xd7, 0, 0, 0x96 }, /* × (multiplication sign) */
    {   0xd8, XF_OSTROKE,     1, 0x30 }, /* Ø (O stroke) */
    {   0xd9, XF_UGRAVE,      1, 0x55 }, /* Ù (U grave) */
    {   0xda, XF_UACUTE,      1, 0x55 }, /* Ú (U acute) */
    {   0xdb, XF_UCIRCUMFLEX, 1, 0x55 }, /* Û (U circumflex) */
    {   0xdc, XF_UDIERESIS,   1, 0x55 }, /* Ü (U dieresis) */
    {   0xdd, XF_YACUTE,      1, 0x59 }, /* Ý (Y acute) */

    {   0xdf, 0, 0, 0xe1 }, /* ß (sharp s) */
    {   0xe0, XF_aGRAVE,      1, 0x61 }, /* à (a grave) */
    {   0xe1, XF_aACUTE,      1, 0x61 }, /* á (a acute) */
    {   0xe2, XF_aCIRCUMFLEX, 1, 0x61 }, /* â (a circumflex) */
    {   0xe3, XF_aTILDE,      1, 0x61 }, /* ã (a tilde) */
    {   0xe4, XF_aDIERESIS,   1, 0x61 }, /* ä (a dieresis) */
    {   0xe5, XF_aRING,       1, 0x61 }, /* å (a with ring above) */
    {   0xe6, XF_aeLIGATURE,  1, 0x61 }, /* æ (ae ligature) */
    {   0xe7, XF_cCEDILLA,    1, 0x63 }, /* ç (c cedilla) */
    {   0xe8, XF_eGRAVE,      1, 0x65 }, /* è (e grave) */
    {   0xe9, XF_eACUTE,      1, 0x65 }, /* é (e acute) */
    {   0xea, XF_eCIRCUMFLEX, 1, 0x65 }, /* ê (e circumflex) */
    {   0xeb, XF_eDIERESIS,   1, 0x65 }, /* ë (e dieresis) */
    {   0xec, XF_iGRAVE,      1, 0x69 }, /* ì (i grave) */
    {   0xed, XF_iACUTE,      1, 0x69 }, /* í (i acute) */
    {   0xee, XF_iCIRCUMFLEX, 1, 0x69 }, /* î (i circumflex) */
    {   0xef, XF_iDIERESIS,   1, 0x69 }, /* ï (i dieresis) */

    {   0xf1, XF_nTILDE,      1, 0x6e }, /* ñ (n tilde) */
    {   0xf2, XF_oGRAVE,      1, 0x6f }, /* ò (o grave) */
    {   0xf3, XF_oACUTE,      1, 0x6f }, /* ó (o acute) */
    {   0xf4, XF_oCIRCUMFLEX, 1, 0x6f }, /* ô (o circumflex) */
    {   0xf5, XF_oTILDE,      1, 0x6f }, /* õ (o tilde) */
    {   0xf6, XF_oDIERESIS,   1, 0x6f }, /* ö (o dieresis) */
    {   0xf7, 0, 0, 0x97 }, /* ÷ (division sign) */
    {   0xf8, XF_oSLASH,      1, 0x6f }, /* ø (o slash) */
    {   0xf9, XF_uGRAVE,      1, 0x75 }, /* ù (u grave) */
    {   0xfa, XF_uACUTE,      1, 0x75 }, /* ú (u acute) */
    {   0xfb, XF_uCIRCUMFLEX, 1, 0x75 }, /* û (u circumflex) */
    {   0xfc, XF_uDIERESIS,   1, 0x75 }, /* ü (u dieresis) */
    {   0xfd, XF_yACUTE,      1, 0x79 }, /* ý (y acute) */

    {   0xff, XF_yDIERESIS,   1, 0x79 }, /* ÿ (y dieresis) */
    
    /* Latin extended A */
    { 0x0103, XF_aBREVE,      1, 0x61 }, /* a breve */
    { 0x0105, XF_aOGONEK,     1, 0x61 }, /* a ogonek */
    { 0x0107, XF_cACUTE,      1, 0x63 }, /* c acute */
    { 0x010d, XF_cCARON,      1, 0x63 }, /* c caron */
    { 0x010f, XF_dCARON,      1, 0x64 }, /* d caron */
    { 0x0110, XF_DSTROKE,     1, 0x44 }, /* D stroke */
    { 0x0111, XF_dSTROKE,     1, 0x64 }, /* d stroke */
    { 0x0119, XF_eOGONEK,     1, 0x65 }, /* e ogonek */
    { 0x011b, XF_eCARON,      1, 0x65 }, /* e caron */
    { 0x011e, XF_GBREVE,      1, 0x47 }, /* G breve */
    { 0x011f, XF_gBREVE,      1, 0x67 }, /* g breve */
    { 0x0130, XF_IDOT,        1, 0x49 }, /* I with dot above */
    { 0x0131, XF_DOTLESSi,    1, 0x69 }, /* dotless i */
    { 0x0141, XF_LSTROKE,     1, 0x4c }, /* L stroke */
    { 0x0142, XF_lSTROKE,     1, 0x6c }, /* l stroke */
    { 0x0144, XF_nACUTE,      1, 0x6e }, /* n acute */
    { 0x0148, XF_nCARON,      1, 0x6e }, /* n caron */
    { 0x0150, XF_ODBLACUTE,   1, 0x4f }, /* O double acute */
    { 0x0151, XF_oDBLACUTE,   1, 0x6f }, /* o double acute */
    { 0x0158, XF_RCARON,      1, 0x52 }, /* R caron */
    { 0x0159, XF_rCARON,      1, 0x72 }, /* r caron */
    { 0x015a, XF_SACUTE,      1, 0x53 }, /* S acute */
    { 0x015b, XF_sACUTE,      1, 0x73 }, /* s acute */
    { 0x015e, XF_SCEDILLA,    1, 0x53 }, /* S cedilla */
    { 0x015f, XF_sCEDILLA,    1, 0x73 }, /* s cedilla */
    { 0x0160, XF_SCARON,      1, 0x53 }, /* S caron */
    { 0x0161, XF_sCARON,      1, 0x73 }, /* s caron */
    { 0x0163, XF_tCEDILLA,    1, 0x74 }, /* t cedilla */
    { 0x0165, XF_tCARON,      1, 0x74 }, /* t caron */
    { 0x016f, XF_uRING,       1, 0x75 }, /* u with ring above */
    { 0x0170, XF_UDBLACUTE,   1, 0x55 }, /* U double acute */
    { 0x0171, XF_uDBLACUTE,   1, 0x75 }, /* u double acute */
    { 0x017a, XF_zACUTE,      1, 0x7a }, /* z acute */
    { 0x017c, XF_zDOT,        1, 0x7a }, /* z with dot above */
    { 0x017e, XF_zCARON,      1, 0x7a }, /* z caron */

    /* Greek */
    { 0x037e, 0, 0, 0x3b }, /* greek question mark */
    
    { 0x0386, 0, 0, 0x41 }, /* greek ALPHA with tonos */
    { 0x0387, 0, 0, 0xa5 }, /* greek ano teleia */
    { 0x0388, 0, 0, 0x45 }, /* greek EPSILON with tonos */
    { 0x0389, 0, 0, 0x48 }, /* greek ETA with tonos */
    { 0x038a, 0, 0, 0x49 }, /* greek IOTA with tonos */
    /* reserved */
    { 0x038c, 0, 0, 0x4f }, /* greek OMICRON with tonos */
    /* reserved */
    { 0x038e, 0, 0, 0x59 }, /* greek YPSILON with tonos */
    { 0x038f, 0, 0, 0xea }, /* greek OMEGA with tonos */
    { 0x0390, XF_GR_iotaTONOS, 1, 0x69 }, /* greek iota with dialytica + tonos */
    { 0x0391, 0, 0, 0x41 }, /* greek ALPHA */
    { 0x0392, 0, 0, 0x42 }, /* greek BETA */
    { 0x0393, XF_GR_GAMMA,   2, 0xb2 }, /* greek GAMMA */
    { 0x0394, XF_GR_DELTA,   2, 0x1f }, /* greek DELTA */
    { 0x0395, 0, 0, 0x45 }, /* greek EPSILON */
    { 0x0396, 0, 0, 0x5a }, /* greek ZETA */
    { 0x0397, 0, 0, 0x48 }, /* greek ETA */
    { 0x0398, XF_GR_THETA,   1, 0x30 }, /* greek THETA */
    { 0x0399, 0, 0, 0x49 }, /* greek IOTA */
    { 0x039a, 0, 0, 0x4b }, /* greek KAPPA */
    { 0x039b, XF_GR_LAMBDA,  2, 0x4c }, /* greek LAMBDA */
    { 0x039c, 0, 0, 0x4d }, /* greek MU */
    { 0x039d, 0, 0, 0x4e }, /* greek NU */
    { 0x039e, XF_GR_XI,      2, 0xd0 }, /* greek XI */
    { 0x039f, 0, 0, 0x4f }, /* greek OMICRON */
    { 0x03a0, XF_GR_PI,      1, 0x14 }, /* greek PI */
    { 0x03a1, 0, 0, 0x50 }, /* greek RHO */
    /* reserved */
    { 0x03a3, 0, 0, 0xe4 }, /* greek SIGMA */
    { 0x03a4, 0, 0, 0x54 }, /* greek TAU */
    { 0x03a5, 0, 0, 0x59 }, /* greek UPSILON */
    { 0x03a6, 0, 0, 0xe8 }, /* greek PHI */
    { 0x03a7, 0, 0, 0x58 }, /* greek CHI */
    { 0x03a8, XF_GR_PSI,     2, 0xc2 }, /* greek PSI */
    { 0x03a9, 0, 0, 0xea }, /* greek OMEGA */
    { 0x03aa, 0, 0, 0x49 }, /* greek IOTA with dialytica */
    { 0x03ab, 0, 0, 0x59 }, /* greek UPSILON with dialytica */
    { 0x03ac, XF_GR_alphaTONOS,   1, 0xe0 }, /* greek alpha with tonos */
    { 0x03ad, XF_GR_epsilonTONOS, 1, 0xee }, /* greek epsilon with tonos */
    { 0x03ae, XF_GR_etaTONOS,     1, 0x6e }, /* greek eta with tonos */
    { 0x03af, XF_GR_iotaTONOS,    1, 0x69 }, /* greek iota with tonos */
    { 0x03b0, XF_GR_upsilonTONOS, 1, 0x75 }, /* greek upsilon with dialytica + tonos */
    { 0x03b1, 0, 0, 0xe0 }, /* greek alpha */
    { 0x03b2, 0, 0, 0xe1 }, /* greek beta */
    { 0x03b3, 0, 0, 0xe2 }, /* greek gamma */
    { 0x03b4, 0, 0, 0xeb }, /* greek delta */
    { 0x03b5, XF_GR_epsilon, 1, 0xee }, /* greek epsilon */
    { 0x03b6, XF_GR_zeta,    1, 0x7a }, /* greek zeta */
    { 0x03b7, XF_GR_eta,     1, 0x6e }, /* greek eta */
    { 0x03b8, 0, 0, 0xe9 }, /* greek theta */
    { 0x03b9, XF_GR_iota,    1, 0x69 }, /* greek iota */
    { 0x03ba, XF_GR_kappa,   1, 0x6b }, /* greek kappa */
    { 0x03bb, XF_GR_lambda,  2, 0xca }, /* greek lambda */
    { 0x03bc, 0, 0, 0xe6 }, /* greek mu */
    { 0x03bd, 0, 0, 0x76 }, /* greek nu */
    { 0x03be, XF_GR_xi,      2, 0xd0 }, /* greek xi */
    { 0x03bf, 0, 0, 0x6f }, /* greek omicron */
    { 0x03c0, 0, 0, 0xe3 }, /* greek pi */
    { 0x03c1, XF_GR_rho,     1, 0x70 }, /* greek rho */
    { 0x03c2, XF_GR_FINALsigma, 1, 0x73 }, /* greek final sigma */
    { 0x03c3, 0, 0, 0xe5 }, /* greek sigma */
    { 0x03c4, 0, 0, 0xe7 }, /* greek tau */
    { 0x03c5, XF_GR_upsilon, 1, 0x75 }, /* greek upsilon */
    { 0x03c6, 0, 0, 0xed }, /* greek phi */
    { 0x03c7, XF_GR_chi,     1, 0x78 }, /* greek chi */
    { 0x03c8, XF_GR_psi,     2, 0xc2 }, /* greek psi */
    { 0x03c9, XF_GR_omega,   1, 0x77 }, /* greek omega */
    { 0x03ca, XF_GR_iotaDIA, 1, 0x69 }, /* greek iota with dialytica */
    { 0x03cb, XF_GR_upsilon, 1, 0x75 }, /* greek upsilon with dialytica */
    { 0x03cc, XF_GR_omicronTONOS, 1, 0x6f }, /* greek omicron with tonos */
    { 0x03cd, XF_GR_upsilonTONOS, 1, 0x75 }, /* greek upsilon with tonos */
    { 0x03ce, XF_GR_omegaTONOS,   1, 0x77 }, /* greek omega with tonos */

    { 0x03f3, 0, 0, 0x6a }, /* greek yot */

    /* Cyrillic */
    { 0x0400, XF_CYR_IEGRAVE,1, 0x45 }, /* cyrillic IE grave */
    { 0x0401, XF_CYR_IO,     1, 0x45 }, /* cyrillic IO */

    { 0x0405, 0, 0, 0x53 }, /* cyrillic DZE */
    { 0x0406, 0, 0, 0x49 }, /* cyrillic byeloruss-ukr. I */
    { 0x0407, XF_CYR_YI,     1, 0x49 }, /* cyrillic YI */
    { 0x0408, 0, 0, 0x4a }, /* cyrillic JE */

    { 0x0410, 0, 0, 0x41 }, /* cyrillic A */
    { 0x0411, XF_CYR_BE,     1, 0xeb }, /* cyrillic BE */
    { 0x0412, 0, 0, 0x42 }, /* cyrillic VE */
    { 0x0413, XF_CYR_GHE,    2, 0xb2 }, /* cyrillic GHE */
    { 0x0414, XF_CYR_DE,     2, 0x44 }, /* cyrillic DE */
    { 0x0415, 0, 0, 0x45 }, /* cyrillic IE */
    { 0x0416, XF_CYR_ZHE,    2, 0x2a }, /* cyrillic ZHE */
    { 0x0417, XF_CYR_ZE,     1, 0x33 }, /* cyrillic ZE */
    { 0x0418, XF_CYR_I,      1, 0x55 }, /* cyrillic I */
    { 0x0419, XF_CYR_SHORTI, 1, 0x55 }, /* cyrillic short I */
    { 0x041a, 0, 0, 0x4b }, /* cyrillic K */
    { 0x041b, XF_CYR_EL,     2, 0x4c }, /* cyrillic EL */
    { 0x041c, 0, 0, 0x4d }, /* cyrillic EM */
    { 0x041d, 0, 0, 0x48 }, /* cyrillic EN */
    { 0x041e, 0, 0, 0x4f }, /* cyrillic O */
    { 0x041f, XF_CYR_PE,     1, 0x14 }, /* cyrillic PE */
    { 0x0420, 0, 0, 0x50 }, /* cyrillic ER */
    { 0x0421, 0, 0, 0x43 }, /* cyrillic ES */
    { 0x0422, 0, 0, 0x54 }, /* cyrillic TE */
    { 0x0423, 0, 0, 0x59 }, /* cyrillic U */
    { 0x0424, 0, 0, 0xe8 }, /* cyrillic EF */
    { 0x0425, 0, 0, 0x58 }, /* cyrillic HA */
    { 0x0426, XF_CYR_TSE,    2, 0xd9 }, /* cyrillic TSE */
    { 0x0427, XF_CYR_CHE,    2, 0xd1 }, /* cyrillic CHE */
    { 0x0428, XF_CYR_SHA,    1, 0x57 }, /* cyrillic SHA */
    { 0x0429, XF_CYR_SHCHA,  1, 0x57 }, /* cyrillic SHCHA */
    { 0x042a, XF_CYR_HARD,   1, 0x62 }, /* cyrillic capital hard sign */
    { 0x042b, XF_CYR_YERU,   2, 0x1a }, /* cyrillic YERU */
    { 0x042c, 0, 0, 0x62 }, /* cyrillic capital soft sign */
    { 0x042d, XF_CYR_E,      2, 0xa6 }, /* cyrillic E */
    { 0x042e, XF_CYR_YU,     2, 0x1b }, /* cyrillic YU */
    { 0x042f, XF_CYR_YA,     2, 0xf3 }, /* cyrillic YA */
    { 0x0430, 0, 0, 0x61 }, /* cyrillic a */
    { 0x0431, 0, 0, 0xeb }, /* cyrillic be */
    { 0x0432, XF_CYR_ve,     1, 0xe1 }, /* cyrillic ve */
    { 0x0433, XF_CYR_ghe,    1, 0x72 }, /* cyrillic ghe */
    { 0x0434, XF_CYR_de,     2, 0x1f }, /* cyrillic de */
    { 0x0435, 0, 0, 0x65 }, /* cyrillic ie */
    { 0x0436, XF_CYR_zhe,    1, 0x2a }, /* cyrillic zhe */
    { 0x0437, XF_CYR_ze,     1, 0xae }, /* cyrillic ze */
    { 0x0438, XF_CYR_i,      1, 0x75 }, /* cyrillic i */
    { 0x0439, XF_CYR_SHORTi, 1, 0x75 }, /* cyrillic short i */
    { 0x043a, XF_CYR_ka,     1, 0x6b }, /* cyrillic ka */
    { 0x043b, XF_CYR_el,     2, 0xca }, /* cyrillic el */
    { 0x043c, XF_CYR_em,     1, 0x6d }, /* cyrillic em */
    { 0x043d, XF_CYR_en,     2, 0x48 }, /* cyrillic en */
    { 0x043e, 0, 0, 0x6f }, /* cyrillic o */
    { 0x043f, 0, 0, 0xe3 }, /* cyrillic pe */
    { 0x0440, 0, 0, 0x70 }, /* cyrillic er */
    { 0x0441, 0, 0, 0x63 }, /* cyrillic es */
    { 0x0442, 0, 0, 0xe7 }, /* cyrillic te */
    { 0x0443, 0, 0, 0x79 }, /* cyrillic u */
    { 0x0444, 0, 0, 0xed }, /* cyrillic ef */
    { 0x0445, 0, 0, 0x78 }, /* cyrillic ha */
    { 0x0446, XF_CYR_tse,    2, 0xd9 }, /* cyrillic tse */
    { 0x0447, XF_CYR_che,    2, 0xd1 }, /* cyrillic che */
    { 0x0448, XF_CYR_sha,    1, 0x77 }, /* cyrillic sha */
    { 0x0449, XF_CYR_shcha,  1, 0x77 }, /* cyrillic shcha */
    { 0x044a, XF_CYR_hard,   1, 0x62 }, /* cyrillic small hard sign */
    { 0x044b, XF_CYR_yeru,   2, 0x1a }, /* cyrillic yeru */
    { 0x044c, XF_CYR_soft,   1, 0x62 }, /* cyrillic small soft sign */
    { 0x044d, XF_CYR_e,      2, 0xa7 }, /* cyrillic e */
    { 0x044e, XF_CYR_yu,     2, 0x1b }, /* cyrillic yu */
    { 0x044f, XF_CYR_ya,     2, 0xfb }, /* cyrillic ya */
    { 0x0450, XF_CYR_ieGRAVE,1, 0x65 }, /* cyrillic ie grave */
    { 0x0451, XF_CYR_io,     1, 0x65 }, /* cyrillic io */

    { 0x0455, 0, 0, 0x73 }, /* cyrillic dze */
    { 0x0456, 0, 0, 0x69 }, /* cyrillic byeloruss-ukr. i */
    { 0x0457, XF_CYR_yi,     1, 0x69 }, /* cyrillic yi */
    { 0x0458, 0, 0, 0x6a }, /* cyrillic je */

    /* Runtime-definable characters */
    { 0xe000, 0x8000, 15, 0x20 }, /* variable character 0 */
    { 0xe001, 0x8001, 15, 0x20 }, /* variable character 1 */
    { 0xe002, 0x8002, 15, 0x20 }, /* variable character 2 */
    { 0xe003, 0x8003, 15, 0x20 }, /* variable character 3 */
    { 0xe004, 0x8004, 15, 0x20 }, /* variable character 4 */
    { 0xe005, 0x8005, 15, 0x20 }, /* variable character 5 */
    { 0xe006, 0x8006, 15, 0x20 }, /* variable character 6 */
    { 0xe007, 0x8007, 15, 0x20 }, /* variable character 7 */
    { 0xe008, 0x8008, 15, 0x20 }, /* variable character 8 */
    { 0xe009, 0x8009, 15, 0x20 }, /* variable character 9 */
    { 0xe00a, 0x800a, 15, 0x20 }, /* variable character 10 */
    { 0xe00b, 0x800b, 15, 0x20 }, /* variable character 11 */
    { 0xe00c, 0x800c, 15, 0x20 }, /* variable character 12 */
    { 0xe00d, 0x800d, 15, 0x20 }, /* variable character 13 */
    { 0xe00e, 0x800e, 15, 0x20 }, /* variable character 14 */
    { 0xe00f, 0x800f, 15, 0x20 }, /* variable character 15 */

    /* Icons and special symbols */
    { 0xe100, XF_ICON_UNKNOWN,  14, 0x3f }, /* unknown icon (mirrored ?) */
    { 0xe101, XF_ICON_BOOKMARK, 14, 0x94 }, /* bookmark icon */
    { 0xe102, XF_ICON_PLUGIN,   14, 0x29 }, /* plugin icon */
    { 0xe103, XF_ICON_FOLDER,   14, 0x91 }, /* folder icon */
    { 0xe104, XF_ICON_FIRMWARE, 14, 0x78 }, /* firmware icon */
    { 0xe105, XF_ICON_LANGUAGE, 14, 0x2b }, /* language icon */
    { 0xe106, XF_ICON_AUDIO,    14, 0x13 }, /* audio icon (note) */
    { 0xe107, XF_ICON_WPS,      14, 0x94 }, /* wps icon */
    { 0xe108, XF_ICON_PLAYLIST, 14, 0xd0 }, /* playlist icon */
    { 0xe109, XF_ICON_TEXTFILE, 14, 0xd0 }, /* text file icon */
    { 0xe10a, XF_ICON_CONFIG,   14, 0xd0 }, /* config icon */
    { 0xe10b, 0, 0, 0x7f }, /* left arrow */
    { 0xe10c, 0, 0, 0x7e }, /* right arrow */
    { 0xe10d, 0, 0, 0x18 }, /* up arrow */
    { 0xe10e, 0, 0, 0x19 }, /* down arrow */
    { 0xe10f, 0, 0, 0x11 }, /* filled left arrow */
    { 0xe110, 0, 0, 0x10 }, /* filled right arrow */
    { 0xe111, 0, 0, 0x1f }, /* filled up arrow */
    { 0xe112, 0, 0, 0x1e }, /* filled down arrow */
    { 0xe113, 0, 0, 0x20 }, /* level 0/7 */
    { 0xe114, 0, 0, 0x80 }, /* level 1/7 */
    { 0xe115, 0, 0, 0x81 }, /* level 2/7 */
    { 0xe116, 0, 0, 0x82 }, /* level 3/7 */
    { 0xe117, 0, 0, 0x83 }, /* level 4/7 */
    { 0xe118, 0, 0, 0x84 }, /* level 5/7 */
    { 0xe119, 0, 0, 0x85 }, /* level 6/7 */
    { 0xe11a, 0, 0, 0x86 }, /* level 7/7 */
    
    /* Halfwidth CJK punctuation and katakana - new LCD only */
    { 0xff61, 0, 0, 0xa1 }, /* hw ideographic full stop */
    { 0xff62, 0, 0, 0xa2 }, /* hw left corner bracket */
    { 0xff63, 0, 0, 0xa3 }, /* hw right corner bracket */
    { 0xff64, 0, 0, 0xa4 }, /* hw ideographic comma */
    { 0xff65, 0, 0, 0xa5 }, /* hw katakana middle dot */
    { 0xff66, 0, 0, 0xa6 }, /* hw katakana WO */
    { 0xff67, 0, 0, 0xa7 }, /* hw katakana a */
    { 0xff68, 0, 0, 0xa8 }, /* hw katakana i */
    { 0xff69, 0, 0, 0xa9 }, /* hw katakana u */
    { 0xff6a, 0, 0, 0xaa }, /* hw katakana e */
    { 0xff6b, 0, 0, 0xab }, /* hw katakana o */
    { 0xff6c, 0, 0, 0xac }, /* hw katakana ya */
    { 0xff6d, 0, 0, 0xad }, /* hw katakana yu */
    { 0xff6e, 0, 0, 0xae }, /* hw katakana yo */
    { 0xff6f, 0, 0, 0xaf }, /* hw katakana tu */
    { 0xff70, 0, 0, 0xb0 }, /* hw katakana-hiragana prolonged soundmark */
    { 0xff71, 0, 0, 0xb1 }, /* hw katakana A */
    { 0xff72, 0, 0, 0xb2 }, /* hw katakana I */
    { 0xff73, 0, 0, 0xb3 }, /* hw katakana U */
    { 0xff74, 0, 0, 0xb4 }, /* hw katakana E */
    { 0xff75, 0, 0, 0xb5 }, /* hw katakana O */
    { 0xff76, 0, 0, 0xb6 }, /* hw katakana KA */
    { 0xff77, 0, 0, 0xb7 }, /* hw katakana KI */
    { 0xff78, 0, 0, 0xb8 }, /* hw katakana KU */
    { 0xff79, 0, 0, 0xb9 }, /* hw katakana KE */
    { 0xff7a, 0, 0, 0xba }, /* hw katakana KO */
    { 0xff7b, 0, 0, 0xbb }, /* hw katakana SA */
    { 0xff7c, 0, 0, 0xbc }, /* hw katakana SI */
    { 0xff7d, 0, 0, 0xbd }, /* hw katakana SU */
    { 0xff7e, 0, 0, 0xbe }, /* hw katakana SE */
    { 0xff7f, 0, 0, 0xbf }, /* hw katakana SO */
    { 0xff80, 0, 0, 0xc0 }, /* hw katakana TA */
    { 0xff81, 0, 0, 0xc1 }, /* hw katakana TI */
    { 0xff82, 0, 0, 0xc2 }, /* hw katakana TU */
    { 0xff83, 0, 0, 0xc3 }, /* hw katakana TE */
    { 0xff84, 0, 0, 0xc4 }, /* hw katakana TO */
    { 0xff85, 0, 0, 0xc5 }, /* hw katakana NA */
    { 0xff86, 0, 0, 0xc6 }, /* hw katakana NI */
    { 0xff87, 0, 0, 0xc7 }, /* hw katakana NU */
    { 0xff88, 0, 0, 0xc8 }, /* hw katakana NE */
    { 0xff89, 0, 0, 0xc9 }, /* hw katakana NO */
    { 0xff8a, 0, 0, 0xca }, /* hw katakana HA */
    { 0xff8b, 0, 0, 0xcb }, /* hw katakana HI */
    { 0xff8c, 0, 0, 0xcc }, /* hw katakana HU */
    { 0xff8d, 0, 0, 0xcd }, /* hw katakana HE */
    { 0xff8e, 0, 0, 0xce }, /* hw katakana HO */
    { 0xff8f, 0, 0, 0xcf }, /* hw katakana MA */
    { 0xff90, 0, 0, 0xd0 }, /* hw katakana MI */
    { 0xff91, 0, 0, 0xd1 }, /* hw katakana MU */
    { 0xff92, 0, 0, 0xd2 }, /* hw katakana ME */
    { 0xff93, 0, 0, 0xd3 }, /* hw katakana MO */
    { 0xff94, 0, 0, 0xd4 }, /* hw katakana YA */
    { 0xff95, 0, 0, 0xd5 }, /* hw katakana YU */
    { 0xff96, 0, 0, 0xd6 }, /* hw katakana YO */
    { 0xff97, 0, 0, 0xd7 }, /* hw katakana RA */
    { 0xff98, 0, 0, 0xd8 }, /* hw katakana RI */
    { 0xff99, 0, 0, 0xd9 }, /* hw katakana RU */
    { 0xff9a, 0, 0, 0xda }, /* hw katakana RE */
    { 0xff9b, 0, 0, 0xdb }, /* hw katakana RO */
    { 0xff9c, 0, 0, 0xdc }, /* hw katakana WA */
    { 0xff9d, 0, 0, 0xdd }, /* hw katakana N */
    { 0xff9e, 0, 0, 0xde }, /* hw katakana voiced sound mark */
    { 0xff9f, 0, 0, 0xdf }, /* hw katakana semi-voiced sound mark */
#endif /* !BOOTLOADER */

    /* no-char symbol */
    { 0xfffd, 0, 0, 0x91 },
};

const struct xchar_info xchar_info_oldlcd[] = {
    /* Standard ascii */
    {   0x20, 0, 0, 0x24 }, /*   */
    {   0x21, 0, 0, 0x25 }, /* ! */
    {   0x22, 0, 0, 0x26 }, /* " */
    {   0x23, 0, 0, 0x27 }, /* # */
    {   0x24, 0, 0, 0x06 }, /* $ */
    {   0x25, 0, 0, 0x29 }, /* % */
    {   0x26, 0, 0, 0x2a }, /* & */
    {   0x27, 0, 0, 0x2b }, /* ' */
    {   0x28, 0, 0, 0x2c }, /* ( */
    {   0x29, 0, 0, 0x2d }, /* ) */
    {   0x2a, 0, 0, 0x2e }, /* * */
    {   0x2b, 0, 0, 0x2f }, /* + */
    {   0x2c, 0, 0, 0x30 }, /* , */
    {   0x2d, 0, 0, 0x31 }, /* - */
    {   0x2e, 0, 0, 0x32 }, /* . */
    {   0x2f, 0, 0, 0x33 }, /* / */
    {   0x30, 0, 0, 0x34 }, /* 0 */
    {   0x31, 0, 0, 0x35 }, /* 1 */
    {   0x32, 0, 0, 0x36 }, /* 2 */
    {   0x33, 0, 0, 0x37 }, /* 3 */
    {   0x34, 0, 0, 0x38 }, /* 4 */
    {   0x35, 0, 0, 0x39 }, /* 5 */
    {   0x36, 0, 0, 0x3a }, /* 6 */
    {   0x37, 0, 0, 0x3b }, /* 7 */
    {   0x38, 0, 0, 0x3c }, /* 8 */
    {   0x39, 0, 0, 0x3d }, /* 9 */
    {   0x3a, 0, 0, 0x3e }, /* : */
    {   0x3b, 0, 0, 0x3f }, /* ; */
    {   0x3c, 0, 0, 0x40 }, /* < */
    {   0x3d, 0, 0, 0x41 }, /* = */
    {   0x3e, 0, 0, 0x42 }, /* > */
    {   0x3f, 0, 0, 0x43 }, /* ? */
    {   0x40, 0, 0, 0x04 }, /* @ */
    {   0x41, 0, 0, 0x45 }, /* A */
    {   0x42, 0, 0, 0x46 }, /* B */
    {   0x43, 0, 0, 0x47 }, /* C */
    {   0x44, 0, 0, 0x48 }, /* D */
    {   0x45, 0, 0, 0x49 }, /* E */
    {   0x46, 0, 0, 0x4a }, /* F */
    {   0x47, 0, 0, 0x4b }, /* G */
    {   0x48, 0, 0, 0x4c }, /* H */
    {   0x49, 0, 0, 0x4d }, /* I */
    {   0x4a, 0, 0, 0x4e }, /* J */
    {   0x4b, 0, 0, 0x4f }, /* K */
    {   0x4c, 0, 0, 0x50 }, /* L */
    {   0x4d, 0, 0, 0x51 }, /* M */
    {   0x4e, 0, 0, 0x52 }, /* N */
    {   0x4f, 0, 0, 0x53 }, /* O */
    {   0x50, 0, 0, 0x54 }, /* P */
    {   0x51, 0, 0, 0x55 }, /* Q */
    {   0x52, 0, 0, 0x56 }, /* R */
    {   0x53, 0, 0, 0x57 }, /* S */
    {   0x54, 0, 0, 0x58 }, /* T */
    {   0x55, 0, 0, 0x59 }, /* U */
    {   0x56, 0, 0, 0x5a }, /* V */
    {   0x57, 0, 0, 0x5b }, /* W */
    {   0x58, 0, 0, 0x5c }, /* X */
    {   0x59, 0, 0, 0x5d }, /* Y */
    {   0x5a, 0, 0, 0x5e }, /* Z */
    {   0x5b, 0, 0, 0xa9 }, /* [ */
    {   0x5c, XF_BACKSLASH,   2, 0x33 }, /* \ */
    {   0x5d, 0, 0, 0xce }, /* ] */
    {   0x5e, XF_CIRCUMFLEX,  2, 0xee }, /* ^ */
    {   0x5f, 0, 0, 0x15 }, /* _ */
    {   0x60, XF_GRAVEACCENT, 2, 0x2b }, /* ` */
    {   0x61, 0, 0, 0x65 }, /* a */
    {   0x62, 0, 0, 0x66 }, /* b */
    {   0x63, 0, 0, 0x67 }, /* c */
    {   0x64, 0, 0, 0x68 }, /* d */
    {   0x65, 0, 0, 0x69 }, /* e */
    {   0x66, 0, 0, 0x6a }, /* f */
    {   0x67, 0, 0, 0x6b }, /* g */
    {   0x68, 0, 0, 0x6c }, /* h */
    {   0x69, 0, 0, 0x6d }, /* i */
    {   0x6a, 0, 0, 0x6e }, /* j */
    {   0x6b, 0, 0, 0x6f }, /* k */
    {   0x6c, 0, 0, 0x70 }, /* l */
    {   0x6d, 0, 0, 0x71 }, /* m */
    {   0x6e, 0, 0, 0x72 }, /* n */
    {   0x6f, 0, 0, 0x73 }, /* o */
    {   0x70, 0, 0, 0x74 }, /* p */
    {   0x71, 0, 0, 0x75 }, /* q */
    {   0x72, 0, 0, 0x76 }, /* r */
    {   0x73, 0, 0, 0x77 }, /* s */
    {   0x74, 0, 0, 0x78 }, /* t */
    {   0x75, 0, 0, 0x79 }, /* u */
    {   0x76, 0, 0, 0x7a }, /* v */
    {   0x77, 0, 0, 0x7b }, /* w */
    {   0x78, 0, 0, 0x7c }, /* x */
    {   0x79, 0, 0, 0x7d }, /* y */
    {   0x7a, 0, 0, 0x7e }, /* z */
    {   0x7b, 0, 0, 0x2c }, /* { (hard-coded ( ) */
    {   0x7c, XF_VERTICALBAR, 2, 0x25 }, /* | */
    {   0x7d, 0, 0, 0x2d }, /* } (hard-coded ) ) */
    {   0x7e, XF_TILDE,       2, 0x31 }, /* ~ */
    {   0x7f, 0, 0, 0x8b }, /* (full grid) */

#ifndef BOOTLOADER /* bootloader only supports pure ASCII */
    /* Latin 1 */
    {   0xa0, 0, 0, 0x24 }, /*   (non-breaking space) */
    {   0xa1, 0, 0, 0x44 }, /* ¡ (inverted !) */
    {   0xa2, 0, 0, 0xa8 }, /* ¢ (cent sign) */
    {   0xa3, 0, 0, 0x05 }, /* £ (pound sign) */
    {   0xa4, 0, 0, 0x28 }, /* ¤ (currency sign) */
    {   0xa5, 0, 0, 0x07 }, /* ¥ (yen sign) */

    {   0xa7, 0, 0, 0x63 }, /* § (paragraph sign) */

	{   0xab, XF_LEFTDBLANGLEQUOT, 1, 0x40 }, /* « (left double-angle quotation mark) */

    {   0xad, 0, 0, 0x31 }, /* ­ (soft hyphen) */

    {   0xaf, 0, 0, 0xee }, /* ¯ (macron) */

    {   0xb1, XF_PLUSMINUS, 1, 0x2f }, /* ± (plus-minus sign) */
    {   0xb2, XF_SUPER2,    1, 0x36 }, /* ³ (superscript 2) */
    {   0xb3, XF_SUPER3,    1, 0x37 }, /* ³ (superscript 3) */

    {   0xb5, XF_MICRO,     1, 0x79 }, /* µ (micro sign) */
    {   0xb6, 0, 0, 0x1a }, /* ¶ (pilcrow sign) */
    {   0xb7, XF_MIDDLEDOT, 1, 0x32 }, /* · (middle dot) */

    {   0xbb, XF_RIGHTDBLANGLEQUOT, 1, 0x42 }, /* » (right double-angle quotation mark) */
    {   0xbc, XF_ONEQUARTER,    1, 0x29 }, /* ¼ (one quarter) */
    {   0xbd, XF_ONEHALF,       1, 0x29 }, /* ½ (one half) */
    {   0xbe, XF_THREEQUARTERS, 1, 0x29 }, /* ¾ (three quarters) */
    {   0xbf, 0, 0, 0x64 }, /* ¿ (inverted ?) */
    {   0xc0, 0, 0, 0x8c }, /* À (A grave) */
    {   0xc1, 0, 0, 0x8d }, /* Á (A acute) */
    {   0xc2, 0, 0, 0x8e }, /* Â (A circumflex) */
    {   0xc3, 0, 0, 0x8f }, /* Ã (A tilde) */
    {   0xc4, 0, 0, 0x5f }, /* Ä (A dieresis) */
    {   0xc5, 0, 0, 0x12 }, /* Å (A with ring above) */
    {   0xc6, 0, 0, 0x20 }, /* Æ (AE ligature) */
    {   0xc7, 0, 0, 0x0d }, /* Ç (C cedilla) */
    {   0xc8, 0, 0, 0x90 }, /* È (E grave) */
    {   0xc9, 0, 0, 0x23 }, /* É (E acute) */
    {   0xca, 0, 0, 0x91 }, /* Ê (E circumflex) */
    {   0xcb, 0, 0, 0x92 }, /* Ë (E dieresis) */
    {   0xcc, 0, 0, 0x93 }, /* Ì (I grave) */
    {   0xcd, 0, 0, 0x94 }, /* Í (I acute) */
    {   0xce, XF_ICIRCUMFLEX, 1, 0x4d }, /* Î (I circumflex) */
    {   0xcf, XF_IDIERESIS,   1, 0x4d }, /* Ï (I dieresis) */
    {   0xd0, 0, 0, 0x95 }, /* Ð (ETH) */
    {   0xd1, 0, 0, 0x61 }, /* Ñ (N tilde) */
    {   0xd2, 0, 0, 0x96 }, /* Ò (O grave) */
    {   0xd3, 0, 0, 0x97 }, /* Ó (O acute) */
    {   0xd4, 0, 0, 0x98 }, /* Ô (O circumflex) */
    {   0xd5, 0, 0, 0x99 }, /* Õ (O tilde) */
    {   0xd6, 0, 0, 0x60 }, /* Ö (O dieresis) */
    {   0xd7, 0, 0, 0xde }, /* × (multiplication sign) */
    {   0xd8, 0, 0, 0x0f }, /* Ø (O stroke) */
    {   0xd9, 0, 0, 0x9a }, /* Ù (U grave) */
    {   0xda, 0, 0, 0x9b }, /* Ú (U acute) */
    {   0xdb, XF_UCIRCUMFLEX, 1, 0x59 }, /* Û (U circumflex) */
    {   0xdc, 0, 0, 0x62 }, /* Ü (U dieresis) */
    {   0xdd, XF_YACUTE,      1, 0x5d }, /* Ý (Y acute) */

    {   0xdf, 0, 0, 0x22 }, /* ß (sharp s) */
    {   0xe0, 0, 0, 0x83 }, /* à (a grave) */
    {   0xe1, 0, 0, 0x9c }, /* á (a acute) */
    {   0xe2, 0, 0, 0x9d }, /* â (a circumflex) */
    {   0xe3, 0, 0, 0x9e }, /* ã (a tilde) */
    {   0xe4, 0, 0, 0x7f }, /* ä (a dieresis) */
    {   0xe5, 0, 0, 0x13 }, /* å (a with ring above) */
    {   0xe6, 0, 0, 0x21 }, /* æ (ae ligature */
    {   0xe7, 0, 0, 0x84 }, /* ç (c cedilla) */
    {   0xe8, 0, 0, 0x08 }, /* è (e grave) */
    {   0xe9, 0, 0, 0x09 }, /* é (e acute) */
    {   0xea, 0, 0, 0x9f }, /* ê (e circumflex) */
    {   0xeb, 0, 0, 0xa0 }, /* ë (e dieresis) */
    {   0xec, XF_iGRAVE, 1, 0x6d }, /* ì (i grave) */
    {   0xed, 0, 0, 0xa1 }, /* í (i acute) */
    {   0xee, 0, 0, 0xa2 }, /* î (i circumflex) */
    {   0xef, 0, 0, 0xa3 }, /* ï (i dieresis) */

    {   0xf1, 0, 0, 0x81 }, /* ñ (n tilde) */
    {   0xf2, 0, 0, 0x0c }, /* ò (o grave) */
    {   0xf3, 0, 0, 0xa4 }, /* ó (o acute) */
    {   0xf4, 0, 0, 0xa5 }, /* ô (o circumflex) */
    {   0xf5, 0, 0, 0xa6 }, /* õ (o tilde) */
    {   0xf6, 0, 0, 0x80 }, /* ö (o dieresis) */
    {   0xf7, XF_DIVISION,    1, 0x2f }, /* ÷ (division sign) */
    {   0xf8, 0, 0, 0x10 }, /* ø (o slash) */
    {   0xf9, 0, 0, 0x0a }, /* ù (u grave) */
    {   0xfa, 0, 0, 0xa7 }, /* ú (u acute) */
    {   0xfb, XF_uCIRCUMFLEX, 1, 0x79 }, /* û (u circumflex) */
    {   0xfc, 0, 0, 0xa2 }, /* ü (u dieresis) */
    {   0xfd, 0, 0, 0xaf }, /* ý (y acute) */

    {   0xff, XF_yDIERESIS,   1, 0x7d }, /* ÿ (y dieresis) */
    
    /* Latin extended A */
    { 0x0103, 0, 0, 0xe9 }, /* a breve */
    { 0x0105, 0, 0, 0xb3 }, /* a ogonek */
    { 0x0107, 0, 0, 0xb1 }, /* c acute */
    { 0x010d, 0, 0, 0xab }, /* c caron */
    { 0x010f, 0, 0, 0xbc }, /* d caron */
    { 0x0110, 0, 0, 0x95 }, /* D stroke */
    { 0x0111, 0, 0, 0xb0 }, /* d stroke */
    { 0x0119, 0, 0, 0xb2 }, /* e ogonek */
    { 0x011b, 0, 0, 0xad }, /* e caron */
    { 0x011e, 0, 0, 0xc1 }, /* G breve */
    { 0x011f, 0, 0, 0xc2 }, /* g breve */
    { 0x0130, 0, 0, 0xc5 }, /* I with dot above */
    { 0x0131, 0, 0, 0xc6 }, /* dotless i */
    { 0x0141, XF_LSTROKE, 1, 0x50 }, /* L stroke */
    { 0x0142, 0, 0, 0xb8 }, /* l stroke */
    { 0x0144, 0, 0, 0xb7 }, /* n acute */
    { 0x0148, 0, 0, 0xba }, /* n caron */
    { 0x0150, 0, 0, 0xc8 }, /* O double acute */
    { 0x0151, 0, 0, 0xca }, /* o double acute */
    { 0x0158, XF_RCARON,  1, 0x56 }, /* R caron */
    { 0x0159, 0, 0, 0xaa }, /* r caron */
    { 0x015a, 0, 0, 0xb6 }, /* S acute */
    { 0x015b, 0, 0, 0xb6 }, /* s acute */
    { 0x015e, 0, 0, 0xc3 }, /* S cedilla */
    { 0x015f, 0, 0, 0xc4 }, /* s cedilla */
    { 0x0160, 0, 0, 0xac }, /* S caron */
    { 0x0161, 0, 0, 0xac }, /* s caron */
    { 0x0163, 0, 0, 0xd9 }, /* t cedilla */
    { 0x0165, 0, 0, 0xbb }, /* t caron */
    { 0x016f, 0, 0, 0xae }, /* u with ring above */
    { 0x0170, 0, 0, 0xc7 }, /* U double acute */
    { 0x0171, 0, 0, 0xc9 }, /* u double acute */
    { 0x017a, 0, 0, 0xb5 }, /* z acute */
    { 0x017c, 0, 0, 0xb4 }, /* z with dot above */
    { 0x017e, 0, 0, 0xbd }, /* z caron */

    /* Greek */
    { 0x037e, 0, 0, 0x3f }, /* greek question mark */

    { 0x0386, 0, 0, 0x45 }, /* greek ALPHA with tonos */
    { 0x0387, XF_GR_ANOTELEIA,1, 0x32 }, /* greek ano teleia */
    { 0x0388, 0, 0, 0x49 }, /* greek EPSILON with tonos */
    { 0x0389, 0, 0, 0x4c }, /* greek ETA with tonos */
    { 0x038a, 0, 0, 0x4d }, /* greek IOTA with tonos */
    /* reserved */
    { 0x038c, 0, 0, 0x53 }, /* greek OMICRON with tonos */
    /* reserved */
    { 0x038e, 0, 0, 0x5d }, /* greek YPSILON with tonos */
    { 0x038f, 0, 0, 0x19 }, /* greek OMEGA with tonos */
    { 0x0390, 0, 0, 0xa1 }, /* greek iota with dialytica + tonos */
    { 0x0391, 0, 0, 0x45 }, /* greek ALPHA */
    { 0x0392, 0, 0, 0x46 }, /* greek BETA */
    { 0x0393, 0, 0, 0x17 }, /* greek GAMMA */
    { 0x0394, 0, 0, 0x14 }, /* greek DELTA */
    { 0x0395, 0, 0, 0x49 }, /* greek EPSILON */
    { 0x0396, 0, 0, 0x5e }, /* greek ZETA */
    { 0x0397, 0, 0, 0x4c }, /* greek ETA */
    { 0x0398, 0, 0, 0x1d }, /* greek THETA */
    { 0x0399, 0, 0, 0x4d }, /* greek IOTA */
    { 0x039a, 0, 0, 0x4f }, /* greek KAPPA */
    { 0x039b, 0, 0, 0x18 }, /* greek LAMBDA */
    { 0x039c, 0, 0, 0x51 }, /* greek MU */
    { 0x039d, 0, 0, 0x52 }, /* greek NU */
    { 0x039e, 0, 0, 0x1e }, /* greek XI */
    { 0x039f, 0, 0, 0x53 }, /* greek OMICRON */
    { 0x03a0, 0, 0, 0x1a }, /* greek PI */
    { 0x03a1, 0, 0, 0x54 }, /* greek RHO */
    /* reserved */
    { 0x03a3, 0, 0, 0x1c }, /* greek SIGMA */
    { 0x03a4, 0, 0, 0x58 }, /* greek TAU */
    { 0x03a5, 0, 0, 0x5d }, /* greek UPSILON */
    { 0x03a6, 0, 0, 0x16 }, /* greek PHI */
    { 0x03a7, 0, 0, 0x5c }, /* greek CHI */
    { 0x03a8, 0, 0, 0x1b }, /* greek PSI */
    { 0x03a9, 0, 0, 0x19 }, /* greek OMEGA */
    { 0x03aa, 0, 0, 0x4d }, /* greek IOTA with dialytica */
    { 0x03ab, 0, 0, 0x5d }, /* greek UPSILON with dialytica */
    { 0x03ac, XF_GR_alphaTONOS,   1, 0x9c }, /* greek alpha with tonos */
    { 0x03ad, XF_GR_epsilonTONOS, 1, 0x69 }, /* greek epsilon with tonos */
    { 0x03ae, XF_GR_etaTONOS,     1, 0x72 }, /* greek eta with tonos */
    { 0x03af, 0, 0, 0xa1 }, /* greek iota with tonos */
    { 0x03b0, XF_GR_upsilonTONOS, 1, 0xa7 }, /* greek upsilon with dialytica + tonos */
    { 0x03b1, XF_GR_alpha,   1, 0x65 }, /* greek alpha */
    { 0x03b2, 0, 0, 0x22 }, /* greek beta */
    { 0x03b3, XF_GR_gamma,   1, 0x7d }, /* greek gamma */
    { 0x03b4, XF_GR_delta,   2, 0x14 }, /* greek delta */
    { 0x03b5, XF_GR_epsilon, 1, 0x69 }, /* greek epsilon */
    { 0x03b6, XF_GR_zeta,    1, 0x7e }, /* greek zeta */
    { 0x03b7, XF_GR_eta,     1, 0x72 }, /* greek eta */
    { 0x03b8, 0, 0, 0x1d }, /* greek theta */
    { 0x03b9, 0, 0, 0xc6 }, /* greek iota */
    { 0x03ba, XF_GR_kappa,   1, 0x6f }, /* greek kappa */
    { 0x03bb, XF_GR_lambda,  1, 0x18 }, /* greek lambda */
    { 0x03bc, XF_GR_mu,      1, 0x79 }, /* greek mu */
    { 0x03bd, 0, 0, 0x7a }, /* greek nu */
    { 0x03be, XF_GR_xi,      2, 0x1e }, /* greek xi */
    { 0x03bf, 0, 0, 0x73 }, /* greek omicron */
    { 0x03c0, XF_GR_pi,      1, 0x72 }, /* greek pi */
    { 0x03c1, XF_GR_rho,     1, 0x74 }, /* greek rho */
    { 0x03c2, XF_GR_FINALsigma, 1, 0x77 }, /* greek final sigma */
    { 0x03c3, XF_GR_sigma,   1, 0x73 }, /* greek sigma */
    { 0x03c4, XF_GR_tau,     1, 0x78 }, /* greek tau */
    { 0x03c5, XF_GR_upsilon, 1, 0x79 }, /* greel upsilon */
    { 0x03c6, 0, 0, 0x10 }, /* greek phi */
    { 0x03c7, XF_GR_chi,     1, 0x7c }, /* greek chi */
    { 0x03c8, XF_GR_psi,     1, 0x1b }, /* greek psi */
    { 0x03c9, XF_GR_omega,   1, 0x7b }, /* greek omega */
    { 0x03ca, 0, 0, 0xa3 }, /* greek iota with dialytica */
    { 0x03cb, XF_GR_upsilon, 1, 0x82 }, /* greek upsilon with dialytica */
    { 0x03cc, 0, 0, 0xa4 }, /* greek omicron with tonos */
    { 0x03cd, XF_GR_upsilonTONOS, 1, 0xa7 }, /* greek upsilon with tonos */
    { 0x03ce, XF_GR_omegaTONOS,   1, 0x7b }, /* greek omega with tonos */

    { 0x03f3, 0, 0, 0x6e }, /* greek yot */

    /* Cyrillic */
    { 0x0400, 0, 0, 0x90 }, /* cyrillic IE grave */
    { 0x0401, 0, 0, 0x92 }, /* cyrillic IO */

    { 0x0405, 0, 0, 0x57 }, /* cyrillic DZE */
    { 0x0406, 0, 0, 0x4d }, /* cyrillic byeloruss-ukr. I */
    { 0x0407, XF_CYR_YI,     1, 0x4d }, /* cyrillic YI */
    { 0x0408, 0, 0, 0x4e }, /* cyrillic JE */

    { 0x0410, 0, 0, 0x45 }, /* cyrillic A */
    { 0x0411, XF_CYR_BE,     1, 0x3a }, /* cyrillic BE */
    { 0x0412, 0, 0, 0x46 }, /* cyrillic VE */
    { 0x0413, 0, 0, 0x17 }, /* cyrillic GHE */
    { 0x0414, XF_CYR_DE,     1, 0x14 }, /* cyrillic DE */
    { 0x0415, 0, 0, 0x49 }, /* cyrillic IE */
    { 0x0416, XF_CYR_ZHE,    2, 0x2e }, /* cyrillic ZHE */
    { 0x0417, XF_CYR_ZE,     1, 0x37 }, /* cyrillic ZE */
    { 0x0418, XF_CYR_I,      1, 0x59 }, /* cyrillic I */
    { 0x0419, XF_CYR_SHORTI, 1, 0x9b }, /* cyrillic short I */
    { 0x041a, 0, 0, 0x4f }, /* cyrillic K */
    { 0x041b, XF_CYR_EL,     1, 0x18 }, /* cyrillic EL */
    { 0x041c, 0, 0, 0x51 }, /* cyrillic EM */
    { 0x041d, 0, 0, 0x4c }, /* cyrillic EN */
    { 0x041e, 0, 0, 0x53 }, /* cyrillic O */
    { 0x041f, 0, 0, 0x1a }, /* cyrillic PE */
    { 0x0420, 0, 0, 0x54 }, /* cyrillic ER */
    { 0x0421, 0, 0, 0x47 }, /* cyrillic ES */
    { 0x0422, 0, 0, 0x58 }, /* cyrillic TE */
    { 0x0423, 0, 0, 0x5d }, /* cyrillic U */
    { 0x0424, 0, 0, 0x16 }, /* cyrillic EF */
    { 0x0425, 0, 0, 0x5c }, /* cyrillic HA */
    { 0x0426, XF_CYR_TSE,    2, 0x5e }, /* cyrillic TSE */
    { 0x0427, XF_CYR_CHE,    2, 0x0e }, /* cyrillic CHE */
    { 0x0428, XF_CYR_SHA,    1, 0x5b }, /* cyrillic SHA */
    { 0x0429, XF_CYR_SHCHA,  1, 0x5b }, /* cyrillic SHCHA */
    { 0x042a, XF_CYR_HARD,   1, 0x66 }, /* cyrillic capital hard sign */
    { 0x042b, XF_CYR_YERU,   2, 0x66 }, /* cyrillic YERU */
    { 0x042c, 0, 0, 0x66 }, /* cyrillic capital soft sign */
    { 0x042d, XF_CYR_E,      2, 0x89 }, /* cyrillic E */
    { 0x042e, XF_CYR_YU,     2, 0x95 }, /* cyrillic YU */
    { 0x042f, XF_CYR_YA,     1, 0x0d }, /* cyrillic YA */
    { 0x0430, 0, 0, 0x65 }, /* cyrillic a */
    { 0x0431, XF_CYR_be,     1, 0x97 }, /* cyrillic be */
    { 0x0432, XF_CYR_ve,     1, 0x22 }, /* cyrillic ve */
    { 0x0433, XF_CYR_ghe,    1, 0x76 }, /* cyrillic ghe */
    { 0x0434, XF_CYR_de,     2, 0x14 }, /* cyrillic de */
    { 0x0435, 0, 0, 0x69 }, /* cyrillic ie */
    { 0x0436, XF_CYR_zhe,    1, 0x2e }, /* cyrillic zhe */
    { 0x0437, XF_CYR_ze,     1, 0x37 }, /* cyrillic ze */
    { 0x0438, XF_CYR_i,      1, 0x79 }, /* cyrillic i */
    { 0x0439, XF_CYR_SHORTi, 1, 0xa7 }, /* cyrillic short i */
    { 0x043a, XF_CYR_ka,     1, 0x6f }, /* cyrillic ka */
    { 0x043b, XF_CYR_el,     1, 0x18 }, /* cyrillic el */
    { 0x043c, XF_CYR_em,     1, 0x71 }, /* cyrillic em */
    { 0x043d, XF_CYR_en,     2, 0x4c }, /* cyrillic en */
    { 0x043e, 0, 0, 0x73 }, /* cyrillic o */
    { 0x043f, XF_CYR_pe,     1, 0x72 }, /* cyrillic pe */
    { 0x0440, 0, 0, 0x74 }, /* cyrillic er */
    { 0x0441, 0, 0, 0x67 }, /* cyrillic es */
    { 0x0442, XF_CYR_te,     1, 0x78 }, /* cyrillic te */
    { 0x0443, 0, 0, 0x7d }, /* cyrillic u */
    { 0x0444, 0, 0, 0x10 }, /* cyrillic ef */
    { 0x0445, 0, 0, 0x7c }, /* cyrillic ha */
    { 0x0446, XF_CYR_tse,    2, 0x7e }, /* cyrillic tse */
    { 0x0447, XF_CYR_che,    2, 0x0e }, /* cyrillic che */
    { 0x0448, XF_CYR_sha,    1, 0x7b }, /* cyrillic sha */
    { 0x0449, XF_CYR_shcha,  1, 0x7b }, /* cyrillic shcha */
    { 0x044a, XF_CYR_hard,   1, 0x66 }, /* cyrillic small hard sign */
    { 0x044b, XF_CYR_yeru,   2, 0x66 }, /* cyrillic yeru */
    { 0x044c, XF_CYR_soft,   1, 0x66 }, /* cyrillic small soft sign */
    { 0x044d, XF_CYR_e,      2, 0x89 }, /* cyrillic e */
    { 0x044e, XF_CYR_yu,     2, 0x95 }, /* cyrillic yu */
    { 0x044f, XF_CYR_ya,     2, 0x84 }, /* cyrillic ya */
    { 0x0450, 0, 0, 0x08 }, /* cyrillic ie grave */
    { 0x0451, 0, 0, 0xa0 }, /* cyrillic io */
    
    { 0x0455, 0, 0, 0x77 }, /* cyrillic dze */
    { 0x0456, 0, 0, 0x6d }, /* cyrillic byeloruss-ukr. i */
    { 0x0457, 0, 0, 0xa3 }, /* cyrillic yi */
    { 0x0458, 0, 0, 0x6e }, /* cyrillic je */

    /* Runtime-definable characters */
    { 0xe000, 0x8000, 15, 0x24 }, /* variable character 0 */
    { 0xe001, 0x8001, 15, 0x24 }, /* variable character 1 */
    { 0xe002, 0x8002, 15, 0x24 }, /* variable character 2 */
    { 0xe003, 0x8003, 15, 0x24 }, /* variable character 3 */
    { 0xe004, 0x8004, 15, 0x24 }, /* variable character 4 */
    { 0xe005, 0x8005, 15, 0x24 }, /* variable character 5 */
    { 0xe006, 0x8006, 15, 0x24 }, /* variable character 6 */
    { 0xe007, 0x8007, 15, 0x24 }, /* variable character 7 */
    { 0xe008, 0x8008, 15, 0x24 }, /* variable character 8 */
    { 0xe009, 0x8009, 15, 0x24 }, /* variable character 9 */
    { 0xe00a, 0x800a, 15, 0x24 }, /* variable character 10 */
    { 0xe00b, 0x800b, 15, 0x24 }, /* variable character 11 */
    { 0xe00c, 0x800c, 15, 0x24 }, /* variable character 12 */
    { 0xe00d, 0x800d, 15, 0x24 }, /* variable character 13 */
    { 0xe00e, 0x800e, 15, 0x24 }, /* variable character 14 */
    { 0xe00f, 0x800f, 15, 0x24 }, /* variable character 15 */

    /* Icons and special symbols */
    { 0xe100, XF_ICON_UNKNOWN,  14, 0x43 }, /* unknown icon (mirrored ?) */
    { 0xe101, XF_ICON_BOOKMARK, 14, 0xd4 }, /* bookmark icon */
    { 0xe102, XF_ICON_PLUGIN,   14, 0x2d }, /* plugin icon */
    { 0xe103, XF_ICON_FOLDER,   14, 0x34 }, /* folder icon */
    { 0xe104, XF_ICON_FIRMWARE, 14, 0x7c }, /* firmware icon */
    { 0xe105, XF_ICON_LANGUAGE, 14, 0x2f }, /* language icon */
    { 0xe106, 0, 0, 0xfc }, /* audio icon (note) */
    { 0xe107, XF_ICON_WPS,      14, 0xd4 }, /* wps icon */
    { 0xe108, XF_ICON_PLAYLIST, 14, 0xfa }, /* playlist icon */
    { 0xe109, XF_ICON_TEXTFILE, 14, 0xfa }, /* text file icon */
    { 0xe10a, XF_ICON_CONFIG,   14, 0xfa }, /* config icon */
    { 0xe10b, 0, 0, 0x88 }, /* left arrow */
    { 0xe10c, 0, 0, 0x89 }, /* right arrow */
    { 0xe10d, 0, 0, 0x86 }, /* up arrow */
    { 0xe10e, 0, 0, 0x87 }, /* down arrow */
    { 0xe10f, 0, 0, 0x88 }, /* filled left arrow */
    { 0xe110, 0, 0, 0x89 }, /* filled right arrow */
    { 0xe111, 0, 0, 0x86 }, /* filled up arrow */
    { 0xe112, 0, 0, 0x87 }, /* filled down arrow */
    { 0xe113, 0, 0, 0x24 }, /* level 0/7 */
    { 0xe114, 0, 0, 0x15 }, /* level 1/7 */
    { 0xe115, 0, 0, 0xdf }, /* level 2/7 */
    { 0xe116, 0, 0, 0xe0 }, /* level 3/7 */
    { 0xe117, 0, 0, 0xe1 }, /* level 4/7 */
    { 0xe118, 0, 0, 0xe2 }, /* level 5/7 */
    { 0xe119, 0, 0, 0xe3 }, /* level 6/7 */
    { 0xe11a, 0, 0, 0xec }, /* level 7/7 */
#endif /* !BOOTLOADER */

    /* no-char symbol */
    { 0xfffd, 0, 0, 0xd8 },
};

const unsigned char xfont_fixed[][HW_PATTERN_SIZE] = {
    /* Standard ascii */
    [XF_BACKSLASH] =     { 0x00, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00}, /* \ */
    [XF_CIRCUMFLEX] =    { 0x04, 0x0a, 0x11, 0x00, 0x00, 0x00, 0x00}, /* ^ */
    [XF_GRAVEACCENT] =   { 0x08, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00}, /* ` */
    [XF_VERTICALBAR] =   { 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, /* | */
    [XF_TILDE] =         { 0x00, 0x00, 0x08, 0x15, 0x02, 0x00, 0x00}, /* ~ */

#ifndef BOOTLOADER /* bootloader only supports pure ASCII */
    /* Icons and special symbols */
    [XF_ICON_UNKNOWN] =  { 0x0c, 0x12, 0x12, 0x08, 0x08, 0x00, 0x08},
    [XF_ICON_BOOKMARK] = { 0x00, 0x03, 0x07, 0x0e, 0x1c, 0x08, 0x00},
    [XF_ICON_PLUGIN] =   { 0x04, 0x1e, 0x07, 0x1f, 0x05, 0x01, 0x06},
    [XF_ICON_FOLDER] =   { 0x0c, 0x13, 0x11, 0x11, 0x11, 0x11, 0x1f},
    [XF_ICON_FIRMWARE] = { 0x1f, 0x11, 0x1b, 0x15, 0x1b, 0x11, 0x1f},
    [XF_ICON_LANGUAGE] = { 0x00, 0x1f, 0x15, 0x1f, 0x15, 0x1f, 0x00},
    [XF_ICON_AUDIO] =    { 0x03, 0x05, 0x09, 0x09, 0x0b, 0x1b, 0x18},
    [XF_ICON_WPS] =      { 0x01, 0x01, 0x02, 0x02, 0x14, 0x0c, 0x04},
    [XF_ICON_PLAYLIST] = { 0x17, 0x00, 0x17, 0x00, 0x17, 0x00, 0x17},
    [XF_ICON_TEXTFILE] = { 0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x1f},
    [XF_ICON_CONFIG] =   { 0x0b, 0x10, 0x0b, 0x00, 0x1f, 0x00, 0x1f},
    /* Latin 1 */
    [XF_INVEXCLAMATION]= { 0x04, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04}, /* ¡ */
    [XF_CENTSIGN] =      { 0x04, 0x04, 0x0f, 0x10, 0x10, 0x0f, 0x04}, /* ¢ */
    [XF_POUNDSIGN] =     { 0x06, 0x09, 0x08, 0x1e, 0x08, 0x08, 0x1f}, /* £ */
    [XF_CURRENCY] =      { 0x00, 0x11, 0x0e, 0x0a, 0x0e, 0x11, 0x00}, /* ¤ */
    [XF_LEFTDBLANGLEQUOT] = { 0x00, 0x05, 0x0a, 0x14, 0x0a, 0x05, 0x00}, /* « */
    [XF_MACRON] =        { 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* ¯ */
    [XF_PLUSMINUS] =     { 0x04, 0x04, 0x1f, 0x04, 0x04, 0x00, 0x1f}, /* ± */
    [XF_SUPER2] =        { 0x1c, 0x04, 0x1c, 0x10, 0x1c, 0x00, 0x00}, /* ³ */
    [XF_SUPER3] =        { 0x1c, 0x04, 0x1c, 0x04, 0x1c, 0x00, 0x00}, /* ³ */
    [XF_MICRO] =         { 0x00, 0x09, 0x09, 0x09, 0x0f, 0x08, 0x10}, /* µ */
    [XF_MIDDLEDOT] =     { 0x00, 0x00, 0x00, 0x0c, 0x0c, 0x00, 0x00}, /* · */
    [XF_RIGHTDBLANGLEQUOT] = { 0x00, 0x14, 0x0a, 0x05, 0x0a, 0x14, 0x00}, /* » */
    [XF_ONEQUARTER] =    { 0x11, 0x12, 0x14, 0x09, 0x13, 0x07, 0x01}, /* ¼ */
    [XF_ONEHALF] =       { 0x11, 0x12, 0x17, 0x09, 0x17, 0x04, 0x07}, /* ½ */
    [XF_THREEQUARTERS] = { 0x18, 0x09, 0x1a, 0x0d, 0x1b, 0x17, 0x01}, /* ¾ */
    [XF_INVQUESTION] =   { 0x04, 0x00, 0x04, 0x08, 0x10, 0x11, 0x0e}, /* ¿ */
    [XF_AGRAVE] =        { 0x08, 0x04, 0x0e, 0x11, 0x1f, 0x11, 0x11}, /* À */
    [XF_AACUTE] =        { 0x02, 0x04, 0x0e, 0x11, 0x1f, 0x11, 0x11}, /* Á */
    [XF_ACIRCUMFLEX] =   { 0x04, 0x0a, 0x0e, 0x11, 0x1f, 0x11, 0x11}, /* Â */
    [XF_ATILDE] =        { 0x0d, 0x12, 0x0e, 0x11, 0x1f, 0x11, 0x11}, /* Ã */
    [XF_ADIERESIS] =     { 0x0a, 0x00, 0x04, 0x0a, 0x11, 0x1f, 0x11}, /* Ä */
    [XF_ARING] =         { 0x04, 0x0a, 0x04, 0x0e, 0x11, 0x1f, 0x11}, /* Å */
    [XF_AELIGATURE] =    { 0x0f, 0x14, 0x14, 0x1f, 0x14, 0x14, 0x17}, /* Æ */
    [XF_CCEDILLA] =      { 0x0f, 0x10, 0x10, 0x10, 0x0f, 0x02, 0x0e}, /* Ç */
    [XF_EGRAVE] =        { 0x08, 0x04, 0x1f, 0x10, 0x1e, 0x10, 0x1f}, /* È */
    [XF_EACUTE] =        { 0x02, 0x04, 0x1f, 0x10, 0x1c, 0x10, 0x1f}, /* É */
    [XF_ECIRCUMFLEX] =   { 0x04, 0x0a, 0x1f, 0x10, 0x1c, 0x10, 0x1f}, /* Ê */
    [XF_EDIERESIS] =     { 0x0a, 0x00, 0x1f, 0x10, 0x1c, 0x10, 0x1f}, /* Ë */
    [XF_IGRAVE] =        { 0x08, 0x04, 0x0e, 0x04, 0x04, 0x04, 0x0e}, /* Ì */
    [XF_IACUTE] =        { 0x02, 0x04, 0x0e, 0x04, 0x04, 0x04, 0x0e}, /* Í */
    [XF_ICIRCUMFLEX] =   { 0x04, 0x0a, 0x0e, 0x04, 0x04, 0x04, 0x0e}, /* Î */
    [XF_IDIERESIS] =     { 0x0a, 0x00, 0x0e, 0x04, 0x04, 0x04, 0x0e}, /* Ï */
    [XF_ETH] =           { 0x0c, 0x0a, 0x09, 0x1d, 0x09, 0x0a, 0x0c}, /* Ð */
    [XF_NTILDE] =        { 0x0d, 0x12, 0x00, 0x19, 0x15, 0x13, 0x11}, /* Ñ */
    [XF_OGRAVE] =        { 0x08, 0x04, 0x0e, 0x11, 0x11, 0x11, 0x0e}, /* Ò */
    [XF_OACUTE] =        { 0x02, 0x04, 0x0e, 0x11, 0x11, 0x11, 0x0e}, /* Ó */
    [XF_OCIRCUMFLEX] =   { 0x04, 0x0a, 0x0e, 0x11, 0x11, 0x11, 0x0e}, /* Ô */
    [XF_OTILDE] =        { 0x0d, 0x12, 0x0e, 0x11, 0x11, 0x11, 0x0e}, /* Õ */
    [XF_ODIERESIS] =     { 0x0a, 0x00, 0x0e, 0x11, 0x11, 0x11, 0x0e}, /* Ö */
    [XF_OSTROKE] =       { 0x01, 0x0e, 0x13, 0x15, 0x19, 0x0e, 0x10}, /* Ø */
    [XF_UGRAVE] =        { 0x08, 0x04, 0x11, 0x11, 0x11, 0x11, 0x0e}, /* Ù */
    [XF_UACUTE] =        { 0x02, 0x04, 0x11, 0x11, 0x11, 0x11, 0x0e}, /* Ú */
    [XF_UCIRCUMFLEX] =   { 0x04, 0x0a, 0x11, 0x11, 0x11, 0x11, 0x0e}, /* Û */
    [XF_UDIERESIS] =     { 0x0a, 0x00, 0x11, 0x11, 0x11, 0x11, 0x0e}, /* Ü */
    [XF_YACUTE] =        { 0x02, 0x04, 0x11, 0x11, 0x0a, 0x04, 0x04}, /* Ý */
    [XF_aGRAVE] =        { 0x08, 0x04, 0x0e, 0x01, 0x0f, 0x11, 0x0f}, /* à */
    [XF_aACUTE] =        { 0x02, 0x04, 0x0e, 0x01, 0x0f, 0x11, 0x0f}, /* á */
    [XF_aCIRCUMFLEX] =   { 0x04, 0x0a, 0x0e, 0x01, 0x0f, 0x11, 0x0f}, /* â */
    [XF_aTILDE] =        { 0x0d, 0x12, 0x0e, 0x01, 0x0f, 0x11, 0x0f}, /* ã */
    [XF_aDIERESIS] =     { 0x0a, 0x00, 0x0e, 0x01, 0x0f, 0x11, 0x0f}, /* ä */
    [XF_aRING] =         { 0x04, 0x0a, 0x0e, 0x01, 0x0f, 0x11, 0x0f}, /* å */
    [XF_aeLIGATURE] =    { 0x00, 0x00, 0x1a, 0x05, 0x0f, 0x14, 0x0f}, /* æ */
    [XF_cCEDILLA] =      { 0x00, 0x0f, 0x10, 0x10, 0x0f, 0x02, 0x04}, /* ç */
    [XF_eGRAVE] =        { 0x08, 0x04, 0x0e, 0x11, 0x1f, 0x10, 0x0e}, /* è */
    [XF_eACUTE] =        { 0x02, 0x04, 0x0e, 0x11, 0x1f, 0x10, 0x0e}, /* é */
    [XF_eCIRCUMFLEX] =   { 0x04, 0x0a, 0x0e, 0x11, 0x1f, 0x10, 0x0e}, /* ê */
    [XF_eDIERESIS] =     { 0x0a, 0x00, 0x0e, 0x11, 0x1f, 0x10, 0x0e}, /* ë */
    [XF_iGRAVE] =        { 0x08, 0x04, 0x00, 0x0c, 0x04, 0x04, 0x0e}, /* ì */
    [XF_iACUTE] =        { 0x02, 0x04, 0x00, 0x0c, 0x04, 0x04, 0x0e}, /* í */
    [XF_iCIRCUMFLEX] =   { 0x04, 0x0a, 0x00, 0x0c, 0x04, 0x04, 0x0e}, /* î */
    [XF_iDIERESIS] =     { 0x0a, 0x00, 0x00, 0x0c, 0x04, 0x04, 0x0e}, /* ï */
    [XF_nTILDE] =        { 0x0d, 0x12, 0x00, 0x16, 0x19, 0x11, 0x11}, /* ñ */
    [XF_oGRAVE] =        { 0x08, 0x04, 0x00, 0x0e, 0x11, 0x11, 0x0e}, /* ò */
    [XF_oACUTE] =        { 0x02, 0x04, 0x00, 0x0e, 0x11, 0x11, 0x0e}, /* ó */
    [XF_oCIRCUMFLEX] =   { 0x04, 0x0a, 0x00, 0x0e, 0x11, 0x11, 0x0e}, /* ô */
    [XF_oTILDE] =        { 0x0d, 0x12, 0x00, 0x0e, 0x11, 0x11, 0x0e}, /* õ */
    [XF_oDIERESIS] =     { 0x00, 0x0a, 0x00, 0x0e, 0x11, 0x11, 0x0e}, /* ö */
    [XF_DIVISION] =      { 0x00, 0x04, 0x00, 0x1f, 0x00, 0x04, 0x00}, /* ÷ */
    [XF_oSLASH] =        { 0x00, 0x02, 0x0e, 0x15, 0x15, 0x0e, 0x08}, /* ø */
    [XF_uGRAVE] =        { 0x08, 0x04, 0x00, 0x11, 0x11, 0x13, 0x0d}, /* ù */
    [XF_uACUTE] =        { 0x02, 0x04, 0x00, 0x11, 0x11, 0x13, 0x0d}, /* ú */
    [XF_uCIRCUMFLEX] =   { 0x04, 0x0a, 0x00, 0x11, 0x11, 0x13, 0x0d}, /* û */
    [XF_uDIERESIS] =     { 0x00, 0x0a, 0x00, 0x11, 0x11, 0x13, 0x0d}, /* ü */
    [XF_yACUTE] =        { 0x02, 0x04, 0x11, 0x11, 0x0f, 0x01, 0x0e}, /* ý */
    [XF_yDIERESIS] =     { 0x0a, 0x00, 0x11, 0x11, 0x0f, 0x01, 0x0e}, /* ÿ */
    /* Latin extended A */
    [XF_aBREVE] =        { 0x09, 0x06, 0x0e, 0x01, 0x0f, 0x11, 0x0f},
    [XF_aOGONEK] =       { 0x0e, 0x01, 0x0f, 0x11, 0x0f, 0x02, 0x03},
    [XF_cACUTE] =        { 0x02, 0x04, 0x0f, 0x10, 0x10, 0x10, 0x0f},
    [XF_cCARON] =        { 0x0a, 0x04, 0x0f, 0x10, 0x10, 0x10, 0x0f},
    [XF_dCARON] =        { 0x05, 0x05, 0x0c, 0x14, 0x14, 0x14, 0x0c},
    [XF_dSTROKE] =       { 0x02, 0x0f, 0x02, 0x0e, 0x12, 0x12, 0x0e},
    [XF_eOGONEK] =       { 0x0e, 0x11, 0x1f, 0x10, 0x0e, 0x04, 0x06},
    [XF_eCARON] =        { 0x0a, 0x04, 0x0e, 0x11, 0x1f, 0x10, 0x0e},
    [XF_GBREVE] =        { 0x1f, 0x00, 0x0e, 0x10, 0x17, 0x11, 0x0e},
    [XF_gBREVE] =        { 0x1f, 0x00, 0x0f, 0x11, 0x0f, 0x01, 0x0e},
    [XF_IDOT] =          { 0x04, 0x00, 0x0e, 0x04, 0x04, 0x04, 0x0e},
    [XF_DOTLESSi] =      { 0x00, 0x00, 0x0c, 0x04, 0x04, 0x04, 0x0e},
    [XF_LSTROKE] =       { 0x10, 0x10, 0x14, 0x18, 0x10, 0x10, 0x1f},
    [XF_lSTROKE] =       { 0x0c, 0x04, 0x06, 0x0c, 0x04, 0x04, 0x0e},
    [XF_nACUTE] =        { 0x02, 0x04, 0x16, 0x19, 0x11, 0x11, 0x11},
    [XF_nCARON] =        { 0x0a, 0x04, 0x16, 0x19, 0x11, 0x11, 0x11},
    [XF_ODBLACUTE] =     { 0x09, 0x12, 0x0e, 0x11, 0x11, 0x11, 0x0e},
    [XF_oDBLACUTE] =     { 0x09, 0x12, 0x00, 0x0e, 0x11, 0x11, 0x0e},
    [XF_RCARON] =        { 0x0a, 0x04, 0x1e, 0x11, 0x1e, 0x12, 0x11},
    [XF_rCARON] =        { 0x0a, 0x04, 0x0b, 0x0c, 0x08, 0x08, 0x08},
    [XF_sACUTE] =        { 0x02, 0x04, 0x0e, 0x10, 0x0e, 0x01, 0x1e},
    [XF_SCEDILLA] =      { 0x0e, 0x10, 0x0e, 0x01, 0x0e, 0x04, 0x0c},
    [XF_sCEDILLA] =      { 0x00, 0x0e, 0x10, 0x0e, 0x01, 0x0e, 0x04},
    [XF_sCARON] =        { 0x0a, 0x04, 0x0e, 0x10, 0x0e, 0x01, 0x1e},
    [XF_tCEDILLA] =      { 0x04, 0x0f, 0x04, 0x04, 0x04, 0x03, 0x06},
    [XF_tCARON] =        { 0x09, 0x09, 0x08, 0x1e, 0x08, 0x08, 0x06},
    [XF_uRING] =         { 0x04, 0x0a, 0x04, 0x11, 0x11, 0x13, 0x0d},
    [XF_UDBLACUTE] =     { 0x05, 0x0a, 0x11, 0x11, 0x11, 0x11, 0x0e},
    [XF_uDBLACUTE] =     { 0x09, 0x12, 0x00, 0x11, 0x11, 0x13, 0x0d},
    [XF_zACUTE] =        { 0x02, 0x04, 0x1f, 0x02, 0x04, 0x08, 0x1f},
    [XF_zDOT] =          { 0x04, 0x00, 0x1f, 0x02, 0x04, 0x08, 0x1f},
    [XF_zCARON] =        { 0x0a, 0x04, 0x1f, 0x02, 0x04, 0x08, 0x1f},
    /* Greek */
    [XF_GR_DELTA] =        { 0x04, 0x04, 0x0a, 0x0a, 0x11, 0x11, 0x1f},
    [XF_GR_THETA] =        { 0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x0e},
    [XF_GR_LAMBDA] =       { 0x04, 0x04, 0x0a, 0x0a, 0x11, 0x11, 0x11},
    [XF_GR_XI] =           { 0x1f, 0x11, 0x00, 0x0e, 0x00, 0x11, 0x1f},
    [XF_GR_PSI] =          { 0x15, 0x15, 0x15, 0x15, 0x0e, 0x04, 0x04},
    [XF_GR_alpha] =        { 0x00, 0x00, 0x09, 0x15, 0x12, 0x12, 0x0d},
    [XF_GR_alphaTONOS] =   { 0x02, 0x04, 0x09, 0x15, 0x12, 0x12, 0x0d},
    [XF_GR_gamma] =        { 0x00, 0x11, 0x0a, 0x0a, 0x04, 0x04, 0x08},
    [XF_GR_epsilon] =      { 0x00, 0x00, 0x0f, 0x10, 0x0e, 0x10, 0x0f},
    [XF_GR_epsilonTONOS] = { 0x02, 0x04, 0x0f, 0x10, 0x0e, 0x10, 0x0f},
    [XF_GR_zeta] =         { 0x1e, 0x08, 0x10, 0x10, 0x0e, 0x01, 0x06},
    [XF_GR_eta] =          { 0x00, 0x16, 0x19, 0x11, 0x11, 0x11, 0x01},
    [XF_GR_etaTONOS] =     { 0x02, 0x04, 0x16, 0x19, 0x11, 0x11, 0x01},
    [XF_GR_iota] =         { 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x02},
    [XF_GR_lambda] =       { 0x88, 0x04, 0x04, 0x0a, 0x0a, 0x11, 0x11},
    [XF_GR_xi] =           { 0x0c, 0x10, 0x0c, 0x10, 0x0e, 0x01, 0x06},
    [XF_GR_rho] =          { 0x00, 0x0e, 0x11, 0x11, 0x19, 0x16, 0x10},
    [XF_GR_FINALsigma] =   { 0x00, 0x0e, 0x10, 0x10, 0x0e, 0x01, 0x06},
    [XF_GR_sigma] =        { 0x00, 0x00, 0x0f, 0x14, 0x12, 0x11, 0x0e},
    [XF_GR_upsilon] =      { 0x00, 0x00, 0x11, 0x09, 0x09, 0x09, 0x06},
    [XF_GR_upsilonTONOS] = { 0x02, 0x04, 0x11, 0x09, 0x09, 0x09, 0x06},
    [XF_GR_chi] =          { 0x00, 0x12, 0x0a, 0x04, 0x04, 0x0a, 0x09},
    [XF_GR_psi] =          { 0x00, 0x15, 0x15, 0x15, 0x0e, 0x04, 0x04},
    [XF_GR_omega] =        { 0x00, 0x00, 0x0a, 0x11, 0x15, 0x15, 0x0a},
    [XF_GR_omegaTONOS] =   { 0x02, 0x04, 0x0a, 0x11, 0x15, 0x15, 0x0a},
    /* Cyrillic */
    [XF_CYR_BE] =        { 0x1f, 0x10, 0x10, 0x1e, 0x11, 0x11, 0x1e},
    [XF_CYR_GHE] =       { 0x1f, 0x11, 0x10, 0x10, 0x10, 0x10, 0x10},
    [XF_CYR_DE] =        { 0x07, 0x09, 0x09, 0x09, 0x09, 0x1f, 0x11},
    [XF_CYR_ZHE] =       { 0x15, 0x15, 0x0e, 0x04, 0x0e, 0x15, 0x15},
    [XF_CYR_ZE] =        { 0x0e, 0x11, 0x01, 0x0e, 0x01, 0x11, 0x0e},
    [XF_CYR_I] =         { 0x11, 0x11, 0x13, 0x15, 0x19, 0x11, 0x11},
    [XF_CYR_SHORTI] =    { 0x0a, 0x04, 0x11, 0x13, 0x15, 0x19, 0x11},
    [XF_CYR_EL] =        { 0x0f, 0x09, 0x09, 0x09, 0x09, 0x09, 0x11},
    [XF_CYR_PE] =        { 0x1f, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11},
    [XF_CYR_TSE] =       { 0x11, 0x11, 0x11, 0x11, 0x11, 0x1f, 0x01},
    [XF_CYR_CHE] =       { 0x11, 0x11, 0x11, 0x0f, 0x01, 0x01, 0x01},
    [XF_CYR_SHA] =       { 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x1f},
    [XF_CYR_SHCHA] =     { 0x15, 0x15, 0x15, 0x15, 0x15, 0x1f, 0x01},
    [XF_CYR_HARD] =      { 0x18, 0x08, 0x0e, 0x09, 0x09, 0x09, 0x0e},
    [XF_CYR_YERU] =      { 0x11, 0x11, 0x19, 0x15, 0x15, 0x15, 0x19},
    [XF_CYR_E] =         { 0x0e, 0x11, 0x01, 0x07, 0x01, 0x11, 0x0e},
    [XF_CYR_YU] =        { 0x12, 0x15, 0x15, 0x1d, 0x15, 0x15, 0x12},
    [XF_CYR_YA] =        { 0x0f, 0x11, 0x11, 0x0f, 0x05, 0x09, 0x11},
    [XF_CYR_be] =        { 0x0f, 0x10, 0x0e, 0x11, 0x11, 0x11, 0x0e},
    [XF_CYR_ve] =        { 0x00, 0x00, 0x1e, 0x11, 0x1e, 0x11, 0x1e},
    [XF_CYR_ghe] =       { 0x00, 0x00, 0x1f, 0x10, 0x10, 0x10, 0x10},
    [XF_CYR_de] =        { 0x00, 0x00, 0x06, 0x0a, 0x0a, 0x1f, 0x11},
    [XF_CYR_zhe] =       { 0x00, 0x00, 0x15, 0x0e, 0x04, 0x0e, 0x15},
    [XF_CYR_ze] =        { 0x00, 0x00, 0x1e, 0x01, 0x0e, 0x01, 0x1e},
    [XF_CYR_i] =         { 0x00, 0x00, 0x11, 0x13, 0x15, 0x19, 0x11},
    [XF_CYR_SHORTi] =    { 0x0a, 0x04, 0x00, 0x11, 0x13, 0x15, 0x19},
    [XF_CYR_ka] =        { 0x00, 0x00, 0x11, 0x12, 0x1c, 0x12, 0x11},
    [XF_CYR_el] =        { 0x00, 0x00, 0x0f, 0x09, 0x09, 0x09, 0x11},
    [XF_CYR_em] =        { 0x00, 0x00, 0x11, 0x1b, 0x15, 0x11, 0x11},
    [XF_CYR_en] =        { 0x00, 0x00, 0x11, 0x11, 0x1f, 0x11, 0x11},
    [XF_CYR_pe] =        { 0x00, 0x00, 0x1f, 0x11, 0x11, 0x11, 0x11},
    [XF_CYR_te] =        { 0x00, 0x00, 0x1f, 0x04, 0x04, 0x04, 0x02},
    [XF_CYR_tse] =       { 0x00, 0x00, 0x11, 0x11, 0x11, 0x1f, 0x01},
    [XF_CYR_che] =       { 0x00, 0x00, 0x11, 0x11, 0x0f, 0x01, 0x01},
    [XF_CYR_sha] =       { 0x00, 0x00, 0x15, 0x15, 0x15, 0x15, 0x1f},
    [XF_CYR_shcha] =     { 0x00, 0x00, 0x15, 0x15, 0x15, 0x1f, 0x01},
    [XF_CYR_hard] =      { 0x00, 0x00, 0x18, 0x0e, 0x09, 0x09, 0x0e},
    [XF_CYR_yeru] =      { 0x00, 0x00, 0x11, 0x19, 0x15, 0x15, 0x19},
    [XF_CYR_soft] =      { 0x00, 0x00, 0x08, 0x0e, 0x09, 0x09, 0x0e},
    [XF_CYR_e] =         { 0x00, 0x00, 0x0e, 0x11, 0x03, 0x11, 0x0e},
    [XF_CYR_yu] =        { 0x00, 0x00, 0x12, 0x15, 0x1d, 0x15, 0x12},
    [XF_CYR_ya] =        { 0x00, 0x00, 0x0f, 0x11, 0x0f, 0x09, 0x11},
#endif /* !BOOTLOADER */
};

void lcd_charset_init(void)
{
    if (is_new_player())
    {
        lcd_pattern_count = 8;
        xchar_info = xchar_info_newlcd;
        xchar_info_size = sizeof(xchar_info_newlcd)/sizeof(struct xchar_info);
    }
    else /* old lcd */
    {
        lcd_pattern_count = 4;
        xchar_info = xchar_info_oldlcd;
        xchar_info_size = sizeof(xchar_info_oldlcd)/sizeof(struct xchar_info);
    }
}
