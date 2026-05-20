/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 ****************************************************************************/

#include "config.h"
#include "lang.h"
#include "menu.h"
#include "action.h"
#include "splash.h"
#include "button.h"
#include "kernel.h"
#include "settings.h"
#include "usbstack/usb_serial.h"
#include "bluetooth-x1000.h"
#include "gpio-x1000.h"
#include <stdio.h>
#include <string.h>

static char bt_status_text[64] = "Status: idle";

static void bt_refresh_status_text(void)
{
    if (bluetooth_init_is_running())
        snprintf(bt_status_text, sizeof(bt_status_text), "Status: initializing");
    else if (bluetooth_is_ready())
        snprintf(bt_status_text, sizeof(bt_status_text), "Status: controller ready");
    else
        snprintf(bt_status_text, sizeof(bt_status_text), "Status: not ready");
}

void bt_set_status(const char *text)
{
    snprintf(bt_status_text, sizeof(bt_status_text), "Status: %s", text);
}

static void bt_wait_init_done(bool lcd_log)
{
    unsigned waited = 0;
    const unsigned tmax = HZ * 120;

    bluetooth_set_diag_lcd(lcd_log);
    bluetooth_init();
    while (bluetooth_init_is_running() && waited < tmax) {
        sleep(HZ / 10);
        waited += HZ / 10;
    }
    bluetooth_set_diag_lcd(false);
    bluetooth_save_log();
    bt_refresh_status_text();
}

static int bt_enable_callback(int action,
                              const struct menu_item_ex *this_item,
                              struct gui_synclist *this_list)
{
    (void)this_item;
    (void)this_list;
    if (action == ACTION_EXIT_MENUITEM && global_settings.bluetooth_enabled)
        bt_wait_init_done(false);
    return action;
}

static int bt_init_callback(void)
{
    splash(0, "Bluetooth init...");
    bt_wait_init_done(false);
    if (bluetooth_is_ready())
        splash(HZ * 2, "Bluetooth ready");
    else
        splash(HZ * 3, "Init failed (see bt_init_log.txt)");
    return ACTION_NONE;
}

static int bt_power_cycle_callback(void)
{
    splash(HZ, "Power cycling BT...");
    bluetooth_power_cycle();
    sleep(HZ / 2);
    bt_refresh_status_text();
    splash(HZ * 2, "Power cycle done");
    return ACTION_NONE;
}

static int bt_save_log_callback(void)
{
    bluetooth_save_log();
    splash(HZ * 2, "Log saved to SD");
    return ACTION_NONE;
}

static int bt_diag_lcd_callback(void)
{
    splash(0, "BT init (LCD log)...");
    bt_wait_init_done(true);
    if (bluetooth_is_ready())
        splash(HZ * 2, "Bluetooth ready");
    else
        splash(HZ * 3, "Init failed (see bt_init_log.txt)");
    return ACTION_NONE;
}

static int parse_gpio_token(const char *tok)
{
    if (!tok || !tok[0] || !tok[1])
        return -1;
    int pin = atoi(&tok[1]);
    if (pin < 0 || pin > 31)
        return -1;
    switch (tok[0]) {
    case 'A': case 'a': return GPIO_PA(pin);
    case 'B': case 'b': return GPIO_PB(pin);
    case 'C': case 'c': return GPIO_PC(pin);
    case 'D': case 'd': return GPIO_PD(pin);
    default: return -1;
    }
}

static int bt_uart_bridge_callback(void)
{
    unsigned char buf[64];
    char cmd[96];
    int cmd_len = 0;
    const char *help =
        "CMD HELP | CMD STAT | CMD GPIO | CMD BAUD <n> | CMD UART <0|1|2> | CMD WIRE <rtscts|rts0|rts1> | "
        "CMD PIN <19|20|22> <i|0|1> | CMD CPIN <17|18> <i|0|1> | CMD COMBO <on|off> | CMD GP <C17|C22...> <i|0|1> | "
        "CMD GGET <C17|C22...> | CMD CYCLE | CMD FEEDBEAD | CMD SEQ <id|all> | CMD RAIL <A|B|M>\r\n";

    splash(HZ, "BT UART bridge starting...");
    if (!bluetooth_uart_bridge_open()) {
        splash(HZ * 2, "Bridge open failed");
        return ACTION_NONE;
    }

    splash(0, "Bridge active: BACK/POWER to stop");
    usb_serial_send((const unsigned char *)help, strlen(help));
    while (1) {
        int a = button_get_w_tmo(HZ / 50);
        if (a == ACTION_STD_CANCEL || a == BUTTON_POWER)
            break;

        int n_usb = usb_serial_recv(buf, sizeof(buf));
        for (int i = 0; i < n_usb; i++) {
            unsigned char c = buf[i];
            if (c == '\r' || c == '\n') {
                if (cmd_len > 0) {
                    cmd[cmd_len] = '\0';
                    if (!strcmp(cmd, "CMD HELP")) {
                        usb_serial_send((const unsigned char *)help, strlen(help));
                    } else if (!strcmp(cmd, "CMD STAT")) {
                        uint8_t lsr, msr, umcr;
                        char out[80];
                        bluetooth_uart_bridge_get_uart_status(&lsr, &msr, &umcr);
                        snprintf(out, sizeof(out), "STAT LSR=0x%02x MSR=0x%02x UMCR=0x%02x\r\n",
                                 lsr, msr, umcr);
                        usb_serial_send((const unsigned char *)out, strlen(out));
                    } else if (!strcmp(cmd, "CMD GPIO")) {
                        int pc19, pc20, pc22, pc17, pc18;
                        char out[96];
                        bluetooth_uart_bridge_get_gpio_levels(&pc19, &pc20, &pc22, &pc17, &pc18);
                        snprintf(out, sizeof(out), "GPIO PC19=%d PC20=%d PC22=%d PC17=%d PC18=%d\r\n",
                                 pc19, pc20, pc22, pc17, pc18);
                        usb_serial_send((const unsigned char *)out, strlen(out));
                    } else if (!strncmp(cmd, "CMD BAUD ", 9)) {
                        uint32_t baud = (uint32_t)atoi(&cmd[9]);
                        char out[64];
                        if (baud > 0) {
                            bluetooth_uart_bridge_set_baud(baud);
                            snprintf(out, sizeof(out), "OK BAUD %lu\r\n", (unsigned long)baud);
                        } else {
                            snprintf(out, sizeof(out), "ERR BAUD\r\n");
                        }
                        usb_serial_send((const unsigned char *)out, strlen(out));
                    } else if (!strncmp(cmd, "CMD UART ", 9)) {
                        int idx = atoi(&cmd[9]);
                        if (idx >= 0 && idx <= 2) {
                            char out[32];
                            bluetooth_uart_bridge_set_uart_index(idx);
                            snprintf(out, sizeof(out), "OK UART %d\r\n", idx);
                            usb_serial_send((const unsigned char *)out, strlen(out));
                        } else {
                            usb_serial_send((const unsigned char *)"ERR UART\r\n", 10);
                        }
                    } else if (!strncmp(cmd, "CMD WIRE ", 9)) {
                        const char *m = &cmd[9];
                        if (!strcmp(m, "rtscts")) {
                            bluetooth_uart_bridge_set_wire_mode(0);
                            usb_serial_send((const unsigned char *)"OK WIRE rtscts\r\n", 16);
                        } else if (!strcmp(m, "rts0")) {
                            bluetooth_uart_bridge_set_wire_mode(1);
                            usb_serial_send((const unsigned char *)"OK WIRE rts0\r\n", 14);
                        } else if (!strcmp(m, "rts1")) {
                            bluetooth_uart_bridge_set_wire_mode(2);
                            usb_serial_send((const unsigned char *)"OK WIRE rts1\r\n", 14);
                        } else {
                            usb_serial_send((const unsigned char *)"ERR WIRE\r\n", 10);
                        }
                    } else if (!strncmp(cmd, "CMD PIN ", 8)) {
                        int pin = 0;
                        char mode = 0;
                        int pc19 = 2, pc20 = 2, pc22 = 2;
                        const char *p = &cmd[8];
                        if (p[0] == '1' && p[1] == '9' && p[2] == ' ')
                            pin = 19;
                        else if (p[0] == '2' && p[1] == '0' && p[2] == ' ')
                            pin = 20;
                        else if (p[0] == '2' && p[1] == '2' && p[2] == ' ')
                            pin = 22;
                        mode = p[3];
                        if ((pin == 19 || pin == 20 || pin == 22) &&
                            (mode == 'i' || mode == '0' || mode == '1')) {
                            int v = (mode == 'i') ? -1 : ((mode == '1') ? 1 : 0);
                            if (pin == 19) pc19 = v;
                            else if (pin == 20) pc20 = v;
                            else if (pin == 22) pc22 = v;
                            if (pc19 == 2) pc19 = -2;
                            if (pc20 == 2) pc20 = -2;
                            if (pc22 == 2) pc22 = -2;
                            bluetooth_uart_bridge_set_pin_modes(pc19, pc20, pc22);
                            usb_serial_send((const unsigned char *)"OK PIN\r\n", 8);
                        } else {
                            usb_serial_send((const unsigned char *)"ERR PIN\r\n", 9);
                        }
                    } else if (!strncmp(cmd, "CMD CPIN ", 9)) {
                        int pin = 0;
                        char mode = 0;
                        int pc17 = -2, pc18 = -2;
                        const char *p = &cmd[9];
                        if (p[0] == '1' && p[1] == '7' && p[2] == ' ')
                            pin = 17;
                        else if (p[0] == '1' && p[1] == '8' && p[2] == ' ')
                            pin = 18;
                        mode = p[3];
                        if ((pin == 17 || pin == 18) && (mode == 'i' || mode == '0' || mode == '1')) {
                            int v = (mode == 'i') ? -1 : ((mode == '1') ? 1 : 0);
                            if (pin == 17) pc17 = v;
                            else pc18 = v;
                            bluetooth_uart_bridge_set_alt_pin_modes(pc17, pc18);
                            usb_serial_send((const unsigned char *)"OK CPIN\r\n", 9);
                        } else {
                            usb_serial_send((const unsigned char *)"ERR CPIN\r\n", 10);
                        }
                    } else if (!strncmp(cmd, "CMD COMBO ", 10)) {
                        const char *m = &cmd[10];
                        if (!strcmp(m, "on")) {
                            bluetooth_uart_bridge_combo_mode(true);
                            usb_serial_send((const unsigned char *)"OK COMBO ON\r\n", 13);
                        } else if (!strcmp(m, "off")) {
                            bluetooth_uart_bridge_combo_mode(false);
                            usb_serial_send((const unsigned char *)"OK COMBO OFF\r\n", 14);
                        } else {
                            usb_serial_send((const unsigned char *)"ERR COMBO\r\n", 11);
                        }
                    } else if (!strncmp(cmd, "CMD GP ", 7)) {
                        char token[8] = {0};
                        char mode = 0;
                        int gpio = -1;
                        const char *p = &cmd[7];
                        int ti = 0;
                        while (*p && *p != ' ' && ti < (int)sizeof(token) - 1)
                            token[ti++] = *p++;
                        token[ti] = '\0';
                        if (*p == ' ')
                            mode = p[1];
                        if (token[0] != '\0')
                            gpio = parse_gpio_token(token);
                        if (gpio >= 0 && (mode == 'i' || mode == '0' || mode == '1')) {
                            int v = (mode == 'i') ? -1 : ((mode == '1') ? 1 : 0);
                            bluetooth_uart_bridge_set_gpio(gpio, v);
                            usb_serial_send((const unsigned char *)"OK GP\r\n", 7);
                        } else {
                            usb_serial_send((const unsigned char *)"ERR GP\r\n", 8);
                        }
                    } else if (!strncmp(cmd, "CMD GGET ", 9)) {
                        char token[8] = {0};
                        int gpio = -1;
                        const char *p = &cmd[9];
                        int ti = 0;
                        while (*p && *p != ' ' && ti < (int)sizeof(token) - 1)
                            token[ti++] = *p++;
                        token[ti] = '\0';
                        if (token[0] != '\0')
                            gpio = parse_gpio_token(token);
                        if (gpio >= 0) {
                            char out[48];
                            int lvl = bluetooth_uart_bridge_get_gpio(gpio);
                            snprintf(out, sizeof(out), "GGET %s=%d\r\n", token, lvl);
                            usb_serial_send((const unsigned char *)out, strlen(out));
                        } else {
                            usb_serial_send((const unsigned char *)"ERR GGET\r\n", 10);
                        }
                    } else if (!strcmp(cmd, "CMD CYCLE")) {
                        bluetooth_uart_bridge_power_cycle();
                        usb_serial_send((const unsigned char *)"OK CYCLE\r\n", 10);
                    } else if (!strcmp(cmd, "CMD FEEDBEAD")) {
                        char out[48];
                        int rc = bluetooth_uart_bridge_feedbead_probe();
                        snprintf(out, sizeof(out), "FEEDBEAD rc=%d\r\n", rc);
                        usb_serial_send((const unsigned char *)out, strlen(out));
                    } else if (!strncmp(cmd, "CMD SEQ ", 8)) {
                        const char *arg = &cmd[8];
                        if (!strcmp(arg, "all")) {
                            bluetooth_uart_bridge_set_seq_id(-1);
                            usb_serial_send((const unsigned char *)"OK SEQ all\r\n", 12);
                        } else {
                            int seq = atoi(arg);
                            bluetooth_uart_bridge_set_seq_id(seq);
                            usb_serial_send((const unsigned char *)"OK SEQ\r\n", 8);
                        }
                    } else if (!strncmp(cmd, "CMD RAIL ", 9)) {
                        const char *arg = &cmd[9];
                        char out[48];
                        if (!strcmp(arg, "A") || !strcmp(arg, "a") || !strcmp(arg, "0")) {
                            bluetooth_uart_bridge_set_rail_profile(0);
                        } else if (!strcmp(arg, "B") || !strcmp(arg, "b") || !strcmp(arg, "1")) {
                            bluetooth_uart_bridge_set_rail_profile(1);
                        } else if (!strcmp(arg, "M") || !strcmp(arg, "m") ||
                                   !strcmp(arg, "matrix") || !strcmp(arg, "all")) {
                            bluetooth_uart_bridge_set_rail_profile(-1);
                        } else {
                            usb_serial_send((const unsigned char *)"ERR RAIL\r\n", 10);
                            cmd_len = 0;
                            continue;
                        }
                        snprintf(out, sizeof(out), "OK RAIL %d\r\n", bluetooth_uart_bridge_get_rail_profile());
                        usb_serial_send((const unsigned char *)out, strlen(out));
                    } else {
                        (void)bluetooth_uart_bridge_write((const uint8_t *)cmd, cmd_len);
                        (void)bluetooth_uart_bridge_write((const uint8_t *)"\r", 1);
                    }
                    cmd_len = 0;
                }
            } else if (cmd_len < (int)sizeof(cmd) - 1) {
                cmd[cmd_len++] = (char)c;
            }
        }

        int n_uart = bluetooth_uart_bridge_read(buf, sizeof(buf));
        if (n_uart > 0)
            usb_serial_send(buf, n_uart);
    }

    bluetooth_uart_bridge_close();
    splash(HZ, "Bridge stopped");
    return ACTION_NONE;
}

static int bluetooth_menu_callback(int action, const struct menu_item_ex *item,
                                  struct gui_synclist *list)
{
    (void)item;
    (void)list;
    if (action == ACTION_ENTER_MENUITEM)
        bt_refresh_status_text();
    return action;
}

MENUITEM_SETTING(bt_enable_item, &global_settings.bluetooth_enabled, bt_enable_callback);
MENUITEM_FUNCTION(bt_init_item, 0, (unsigned char*)"Initialize Bluetooth", bt_init_callback, NULL, Icon_NOICON);
MENUITEM_FUNCTION(bt_power_cycle_item, 0, (unsigned char*)"Power cycle chip", bt_power_cycle_callback, NULL, Icon_NOICON);
MENUITEM_FUNCTION(bt_save_log_item, 0, (unsigned char*)"Save log to SD", bt_save_log_callback, NULL, Icon_NOICON);
MENUITEM_FUNCTION(bt_diag_lcd_item, 0, (unsigned char*)"Init with LCD log", bt_diag_lcd_callback, NULL, Icon_NOICON);
MENUITEM_FUNCTION(bt_uart_bridge_item, 0, (unsigned char*)"USB UART bridge (advanced)", bt_uart_bridge_callback, NULL, Icon_NOICON);
MENUITEM_RETURNVALUE(bt_status_display_item, (unsigned char*)bt_status_text, 0, NULL, Icon_NOICON);

MAKE_MENU(bluetooth_menu, ID2P(LANG_BLUETOOTH), bluetooth_menu_callback, Icon_NOICON,
          &bt_enable_item,
          &bt_init_item,
          &bt_power_cycle_item,
          &bt_save_log_item,
          &bt_status_display_item,
          &bt_diag_lcd_item,
          &bt_uart_bridge_item);

int bluetooth_menu_run(void)
{
    int start_selected = 0;
    bt_refresh_status_text();
    return do_menu(&bluetooth_menu, &start_selected, NULL, false);
}
