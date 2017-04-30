/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
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
#include "debug.h"
#include "pcm_sw_volume.h"
#include "settings.h"

#include "pcm.h"

#include <tinyalsa/asoundlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

/*
Mixer name: 'WM9711,WM9712,WM9715'
Number of controls: 93
ctl	type	num	name                                     value
0	BOOL	2	Master Playback Switch                   On On
1	INT		2	Master Playback Volume                   8 8
2	BOOL	2	Headphone Playback Switch                On On
3	INT		2	Headphone Playback Volume                31 31
4	BOOL	2	Master Mono Playback Switch              On On
5	INT		1	Master Mono Playback Volume              8
6	INT		1	Tone Control - Bass                      0
7	INT		1	Tone Control - Treble                    0
8	BOOL	1	Phone Playback Switch                    Off
9	INT		1	Phone Playback Volume                    0
10	BOOL	1	Line Playback Switch                     Off
11	INT		2	Line Playback Volume                     0 0
12	BOOL	2	Aux Playback Switch                      On On
13	INT		2	Aux Playback Volume                      23 23
14	BOOL	1	PCM Playback Switch                      On
15	INT		2	PCM Playback Volume                      8 8
16	ENUM	1	PCM Out Path & Mute                      pre 3D
17	BOOL	1	3D Control - Switch                      31 31
18	BOOL	1	Loudness (bass boost)                    Off
19	ENUM	1	Mono Output Select                       Mix
20	ENUM	1	Mic Select                               Mic1
21	INT		1	3D Control - Center                      31 31
22	INT		1	3D Control - Depth                       31 31
23	IEC958	1	IEC958 Playback Con Mask                 unknown
24	IEC958	1	IEC958 Playback Pro Mask                 unknown
25	IEC958	1	IEC958 Playback Default                  unknown
26	BOOL	1	IEC958 Playback Switch                   On
27	INT		1	IEC958 Playback AC97-SPSA                0
28	INT		1	ALC Target Volume                        0
29	INT		1	ALC Hold Time                            8
30	INT		1	ALC Decay Time                           0
31	INT		1	ALC Attack Time                          8
32	ENUM	1	ALC Function                             None
33	INT		1	ALC Max Volume                           6
34	INT		1	ALC ZC Timeout                           3
35	BOOL	1	ALC ZC Switch                            Off
36	BOOL	1	ALC NG Switch                            Off
37	ENUM	1	ALC NG Type                              Constant Gain
38	INT		1	ALC NG Threshold                         23
39	BOOL	1	Side Tone Switch                         On
40	INT		1	Side Tone Volume                         7
41	ENUM	1	ALC Headphone Mux                        Left
42	INT		1	ALC Headphone Volume                     7
43	BOOL	1	Out3 Switch                              On
44	BOOL	1	Out3 ZC Switch                           Off
45	ENUM	1	Out3 Mux                                 Left
46	ENUM	1	Out3 LR Mux                              Master Mix
47	INT		1	Out3 Volume                              23
48	BOOL	1	Beep to Headphone Switch                 On
49	INT		1	Beep to Headphone Volume                 7
50	BOOL	1	Beep to Side Tone Switch                 Off
51	INT		1	Beep to Side Tone Volume                 7
52	BOOL	1	Beep to Phone Switch                     On
53	INT		1	Beep to Phone Volume                     7
54	BOOL	1	Aux to Headphone Switch                  On
55	INT		1	Aux to Headphone Volume                  7
56	BOOL	1	Aux to Side Tone Switch                  On
57	INT		1	Aux to Side Tone Volume                  7
58	BOOL	1	Aux to Phone Switch                      On
59	INT		1	Aux to Phone Volume                      7
60	BOOL	1	Phone to Headphone Switch                Off
61	BOOL	1	Phone to Master Switch                   On
62	BOOL	1	Line to Headphone Switch                 Off
63	BOOL	1	Line to Master Switch                    On
64	BOOL	1	Line to Phone Switch                     Off
65	BOOL	1	PCM Playback to Headphone Switch         On
66	BOOL	1	PCM Playback to Master Switch            On
67	BOOL	1	PCM Playback to Phone Switch             Off
68	BOOL	1	Capture 20dB Boost Switch                Off
69	ENUM	1	Capture to Phone Mux                     Stereo
70	BOOL	1	Capture to Phone 20dB Boost Switch       On
71	ENUM	2	Capture Select                           Mic 1 Mic 1
72	BOOL	1	3D Upper Cut-off Switch                  31 31
73	BOOL	1	3D Lower Cut-off Switch                  31 31
74	ENUM	1	Bass Control                             Linear Control
75	BOOL	1	Bass Cut-off Switch                      On
76	BOOL	1	Tone Cut-off Switch                      On
77	BOOL	1	Playback Attenuate (-6dB) Switch         Off
78	BOOL	1	ADC Switch                               Off
79	ENUM	2	Capture Volume Steps                     +1.5dB Steps +1.5dB Steps
80	INT		2	Capture Volume                           53 57
81	BOOL	1	Capture ZC Switch                        Off
82	BOOL	1	Mic 1 to Phone Switch                    On
83	BOOL	1	Mic 2 to Phone Switch                    On
84	ENUM	1	Mic Select Source                        Mic 2
85	INT		1	Mic 1 Volume                             31
86	INT		1	Mic 2 Volume                             31
87	BOOL	1	Mic 20dB Boost Switch                    Off
88	BOOL	1	Master Left Inv Switch                   Off
89	BOOL	1	Master ZC Switch                         Off
90	BOOL	1	Headphone ZC Switch                      Off
91	BOOL	1	Mono ZC Switch                           Off
92	BOOL	1	External Amplifier                       On

 */

static void tinymix_list_controls(struct mixer *mixer);
static void tinymix_detail_control(struct mixer *mixer, const char *control,
                                   int print_all);
static void tinymix_set_value(struct mixer *mixer, const char *control,
                              char **values, unsigned int num_values);
static void tinymix_print_enum(struct mixer_ctl *ctl, int print_all);

void audiohw_close(void)
{
//    pcm_close_device();
}

void audiohw_postinit(void)
{
    
}

void set_software_volume(void)
{
    /* -73dB (?) minimum software volume in decibels. See pcm-internal.h. */
    static const int SW_VOLUME_MIN = 730;

    int sw_volume_l = 0;
    int sw_volume_r = 0;

    if(global_settings.balance > 0)
    {
        if(global_settings.balance == 100)
        {
            sw_volume_l = PCM_MUTE_LEVEL;
        }
        else
        {
            sw_volume_l -= SW_VOLUME_MIN * global_settings.balance / 100;
        }
    }
    else if(global_settings.balance < 0)
    {
        if(global_settings.balance == -100)
        {
            sw_volume_r = PCM_MUTE_LEVEL;
        }
        else
        {
            sw_volume_r = SW_VOLUME_MIN * global_settings.balance / 100;
        }
    }

    DEBUGF("DEBUG %s: global_settings.balance: %d, sw_volume_l: %d, sw_volume_r: %d.",
           __func__,
           global_settings.balance,
           sw_volume_l,
           sw_volume_r);

    /* Emulate balance with software volume. */
    pcm_set_master_volume(sw_volume_l, sw_volume_r);
}

void audiohw_set_volume(int vol_r)
{
    set_software_volume();

    printf("SW vol %d\n", vol_r);

    /*
        See codec-dx50.h.
        -128db        -> -1 (adjusted to 0, mute)
        -127dB to 0dB ->  1 to 255 in steps of 2
        volume is in centibels (tenth-decibels).
    */
    int volume_adjusted = (vol_r + 470) / 15; // 1.5 dB steps / 10 => 15
    printf("SW scaled %d\n", volume_adjusted);


    if(volume_adjusted > 31)
    {
        volume_adjusted = 31;
    }
    if(volume_adjusted < 0)
    {
        volume_adjusted = 0;
    }

    struct mixer *mixer;
    int card = 0;

    mixer = mixer_open(card);
    if (!mixer) {
        fprintf(stderr, "Failed to open mixer\n");
    }
    else
    {

    	char *val[2];
    	val[0] = malloc(10);
    	val[1] = malloc(10);
    	sprintf(val[0], "%d", volume_adjusted);
    	sprintf(val[1], "%d", volume_adjusted);
//		if (argc == 1) {
//			printf("Mixer name: '%s'\n", mixer_get_name(mixer));
//			tinymix_list_controls(mixer);
//		} else if (argc == 2) {
//			tinymix_detail_control(mixer, argv[1], 1);
//		} else if (argc >= 3) {
//			tinymix_set_value(mixer, argv[1], &argv[2], argc - 2);
//		} else {
//			printf("Usage: tinymix [-D card] [control id] [value to set]\n");
//		}
    	tinymix_set_value(mixer, "3", val, 2);
    	free(val[0]);
    	free(val[1]);
    }

    mixer_close(mixer);

    /*
        /dev/codec_volume
        0 ... 255
    */
//    if(! sysfs_set_int(SYSFS_DX50_CODEC_VOLUME, volume_adjusted))
//    {
//        DEBUGF("ERROR %s: Can not set volume.", __func__);
//    }
}

static void tinymix_list_controls(struct mixer *mixer)
{
    struct mixer_ctl *ctl;
    const char *name, *type;
    unsigned int num_ctls, num_values;
    unsigned int i;

    num_ctls = mixer_get_num_ctls(mixer);

    printf("Number of controls: %d\n", num_ctls);

    printf("ctl\ttype\tnum\t%-40s value\n", "name");
    for (i = 0; i < num_ctls; i++) {
        ctl = mixer_get_ctl(mixer, i);

        name = mixer_ctl_get_name(ctl);
        type = mixer_ctl_get_type_string(ctl);
        num_values = mixer_ctl_get_num_values(ctl);
        printf("%d\t%s\t%d\t%-40s", i, type, num_values, name);
        tinymix_detail_control(mixer, name, 0);
    }
}

static void tinymix_print_enum(struct mixer_ctl *ctl, int print_all)
{
    unsigned int num_enums;
    unsigned int i;
    const char *string;

    num_enums = mixer_ctl_get_num_enums(ctl);

    for (i = 0; i < num_enums; i++) {
        string = mixer_ctl_get_enum_string(ctl, i);
        if (print_all)
            printf("\t%s%s", mixer_ctl_get_value(ctl, 0) == (int)i ? ">" : "",
                   string);
        else if (mixer_ctl_get_value(ctl, 0) == (int)i)
            printf(" %-s", string);
    }
}

static void tinymix_detail_control(struct mixer *mixer, const char *control,
                                   int print_all)
{
    struct mixer_ctl *ctl;
    enum mixer_ctl_type type;
    unsigned int num_values;
    unsigned int i;
    int min, max;
    int ret;
    char buf[512] = { 0 };
    size_t len;

    if (isdigit(control[0]))
        ctl = mixer_get_ctl(mixer, atoi(control));
    else
        ctl = mixer_get_ctl_by_name(mixer, control);

    if (!ctl) {
        fprintf(stderr, "Invalid mixer control\n");
        return;
    }

    type = mixer_ctl_get_type(ctl);
    num_values = mixer_ctl_get_num_values(ctl);

    if (type == MIXER_CTL_TYPE_BYTE) {
        len = num_values;
        if (len > sizeof(buf)) {
            fprintf(stderr, "Truncating get to %zu bytes\n", sizeof(buf));
            len = sizeof(buf);
        }
        ret = mixer_ctl_get_array(ctl, buf, len);
        if (ret < 0) {
            fprintf(stderr, "Failed to mixer_ctl_get_array\n");
            return;
        }
    }

    if (print_all)
        printf("%s:", mixer_ctl_get_name(ctl));

    for (i = 0; i < num_values; i++) {
        switch (type)
        {
        case MIXER_CTL_TYPE_INT:
            printf(" %d", mixer_ctl_get_value(ctl, i));
            break;
        case MIXER_CTL_TYPE_BOOL:
            printf(" %s", mixer_ctl_get_value(ctl, i) ? "On" : "Off");
            break;
        case MIXER_CTL_TYPE_ENUM:
            tinymix_print_enum(ctl, print_all);
            break;
        case MIXER_CTL_TYPE_BYTE:
            printf("%02x", buf[i]);
            break;
        default:
            printf(" unknown");
            break;
        };
    }

    if (print_all) {
        if (type == MIXER_CTL_TYPE_INT) {
            min = mixer_ctl_get_range_min(ctl);
            max = mixer_ctl_get_range_max(ctl);
            printf(" (range %d->%d)", min, max);
        }
    }
    printf("\n");
}

static void tinymix_set_byte_ctl(struct mixer_ctl *ctl, const char *control,
    char **values, unsigned int num_values)
{
    int ret;
    char buf[512] = { 0 };
    char *end;
    int i;
    long n;

    if (num_values > sizeof(buf)) {
        fprintf(stderr, "Truncating set to %zu bytes\n", sizeof(buf));
        num_values = sizeof(buf);
    }

    for (i = 0; i < num_values; i++) {
        errno = 0;
        n = strtol(values[i], &end, 0);
        if (*end) {
            fprintf(stderr, "%s not an integer\n", values[i]);
            exit(EXIT_FAILURE);
        }
        if (errno) {
            fprintf(stderr, "strtol: %s: %s\n", values[i],
                strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (n < 0 || n > 0xff) {
            fprintf(stderr, "%s should be between [0, 0xff]\n",
                values[i]);
            exit(EXIT_FAILURE);
        }
        buf[i] = n;
    }

    ret = mixer_ctl_set_array(ctl, buf, num_values);
    if (ret < 0) {
        fprintf(stderr, "Failed to set binary control\n");
        exit(EXIT_FAILURE);
    }
}

static int is_int(char *value)
{
    char* end;
    long int result;

    errno = 0;
    result = strtol(value, &end, 10);

    if (result == LONG_MIN || result == LONG_MAX)
        return 0;

    return errno == 0 && *end == '\0';
}

static void tinymix_set_value(struct mixer *mixer, const char *control,
                              char **values, unsigned int num_values)
{
    struct mixer_ctl *ctl;
    enum mixer_ctl_type type;
    unsigned int num_ctl_values;
    unsigned int i;

    if (isdigit(control[0]))
        ctl = mixer_get_ctl(mixer, atoi(control));
    else
        ctl = mixer_get_ctl_by_name(mixer, control);

    if (!ctl) {
        fprintf(stderr, "Invalid mixer control\n");
        return;
    }

    type = mixer_ctl_get_type(ctl);
    num_ctl_values = mixer_ctl_get_num_values(ctl);

    if (type == MIXER_CTL_TYPE_BYTE) {
        tinymix_set_byte_ctl(ctl, control, values, num_values);
        return;
    }

    if (is_int(values[0])) {
        if (num_values == 1) {
            /* Set all values the same */
            int value = atoi(values[0]);

            for (i = 0; i < num_ctl_values; i++) {
                if (mixer_ctl_set_value(ctl, i, value)) {
                    fprintf(stderr, "Error: invalid value\n");
                    return;
                }
            }
        } else {
        	printf("MULTIPLE\n");
        	printf("MULTIPLE\n");
            /* Set multiple values */
            if (num_values > num_ctl_values) {
                fprintf(stderr,
                        "Error: %d values given, but control only takes %d\n",
                        num_values, num_ctl_values);
                return;
            }
            for (i = 0; i < num_values; i++) {
            	printf("%d\n", atoi(values[i]));
                if (mixer_ctl_set_value(ctl, i, atoi(values[i]))) {
                    fprintf(stderr, "Error: invalid value for index %d\n", i);
                    return;
                }
            }
        }
    } else {
        if (type == MIXER_CTL_TYPE_ENUM) {
            if (num_values != 1) {
                fprintf(stderr, "Enclose strings in quotes and try again\n");
                return;
            }
            if (mixer_ctl_set_enum_by_string(ctl, values[0]))
                fprintf(stderr, "Error: invalid enum value\n");
        } else {
            fprintf(stderr, "Error: only enum types can be set with strings\n");
        }
    }
}
