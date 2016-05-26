/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 Franklin Wei
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

/* simple HOTP plugin */

#include "plugin.h"

#include "lib/sha1.h"
#include "lib/pluginlib_actions.h"

#define MAX_NAME   200
#define SECRET_MAX 100
#define MAX_ACCTS  20
#define URI_MAX    256
#define ACCT_FILE  PLUGIN_APPS_DATA_DIR "/otp.dat"

struct account_t {
    char name[MAX_NAME];

    bool is_totp; // hotp otherwise

    union {
        uint64_t hotp_counter;
        int totp_period;
    };

    int digits;

    unsigned char secret[SECRET_MAX];
    size_t sec_len;
};

static struct account_t accounts[MAX_ACCTS];

static int next_slot = 0;

static int HOTP(unsigned char *secret, size_t sec_len, uint64_t ctr, int digits)
{
    ctr = htobe64(ctr);
    unsigned char hash[20];
    if(hmac_sha1(secret, sec_len, &ctr, 8, hash))
    {
        return -1;
    }

    int offs = hash[19] & 0xF;
    uint32_t code = (hash[offs] & 0x7F) << 24 |
        hash[offs + 1] << 16 |
        hash[offs + 2] << 8  |
        hash[offs + 3];

    int mod = 1;
    for(int i = 0; i < digits; ++i)
        mod *= 10;

    // debug
    // rb->splashf(HZ * 5, "HOTP %*s, %llu, %d: %d", sec_len, secret, htobe64(ctr), digits, code % mod);

    return code % mod;
}

#ifdef CONFIG_RTC
static int TOTP(unsigned char *secret, size_t sec_len, uint64_t step, int digits)
{
    uint64_t tm = rb->mktime(rb->get_time()) / step;
    return HOTP(secret, sec_len, tm, digits);
}
#endif

/* search the accounts for a duplicate */
static bool acct_exists(const char *name)
{
    for(int i = 0; i < next_slot; ++i)
        if(!strcmp(accounts[i].name, name))
            return true;
    return false;
}

// Base32 implementation
//
// Copyright 2010 Google Inc.
// Author: Markus Gutschke
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

static int base32_decode(uint8_t *result, int bufSize, const uint8_t *encoded) {
  int buffer = 0;
  int bitsLeft = 0;
  int count = 0;
  for (const uint8_t *ptr = encoded; count < bufSize && *ptr; ++ptr) {
    uint8_t ch = *ptr;
    if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '-') {
      continue;
    }
    buffer <<= 5;

    // Deal with commonly mistyped characters
    if (ch == '0') {
      ch = 'O';
    } else if (ch == '1') {
      ch = 'L';
    } else if (ch == '8') {
      ch = 'B';
    }

    // Look up one base32 digit
    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
      ch = (ch & 0x1F) - 1;
    } else if (ch >= '2' && ch <= '7') {
      ch -= '2' - 26;
    } else {
      return -1;
    }

    buffer |= ch;
    bitsLeft += 5;
    if (bitsLeft >= 8) {
      result[count++] = buffer >> (bitsLeft - 8);
      bitsLeft -= 8;
    }
  }
  if (count < bufSize) {
    result[count] = '\000';
  }
  return count;
}

/***********************************************************************
 * File browser (from rockpaint)
 ***********************************************************************/

static bool browse( char *dst, int dst_size, const char *start )
{
    struct browse_context browse;

    rb->browse_context_init(&browse, SHOW_ALL,
                            BROWSE_SELECTONLY|BROWSE_NO_CONTEXT_MENU,
                            NULL, NOICON, start, NULL);

    browse.buf = dst;
    browse.bufsize = dst_size;

    rb->rockbox_browse(&browse);

    return (browse.flags & BROWSE_SELECTED);
}

static void save_accts(void)
{
    int fd = rb->open(ACCT_FILE, O_WRONLY | O_CREAT, 0600);
    for(int i = 0; i < next_slot; ++i)
        rb->write(fd, accounts + i, sizeof(struct account_t));
    rb->close(fd);
}

static void add_acct_file(void)
{
    char fname[MAX_PATH];
    rb->splash(HZ * 2, "Please choose file containing URI(s).");
    int before = next_slot;
    if(browse(fname, sizeof(fname), "/"))
    {
        int fd = rb->open(fname, O_RDONLY);
        do {
            memset(accounts + next_slot, 0, sizeof(struct account_t));

            accounts[next_slot].digits = 6;

            char uri_buf[URI_MAX];
            if(!rb->read_line(fd, uri_buf, sizeof(uri_buf)))
                break;

            /* check for URI prefix */
            if(strncmp(uri_buf, "otpauth://", 10))
                continue;

            char *save;
            char *tok = strtok_r(uri_buf + 10, "/", &save);
            if(!strcmp(tok, "totp"))
            {
                accounts[next_slot].is_totp = true;
                accounts[next_slot].totp_period = 30;
#ifndef CONFIG_RTC
                rb->splash(2 * HZ, "TOTP not supported!");
                continue;
#endif
            }
            else if(!strcmp(tok, "hotp"))
            {
                accounts[next_slot].is_totp = false;
                accounts[next_slot].hotp_counter = 0;
            }

            tok = strtok_r(NULL, "?", &save);
            if(!tok)
                break;

            if(acct_exists(tok))
            {
                rb->splashf(HZ * 2, "Not adding account with duplicate name `%s'!", tok);
                continue;
            }

            rb->strlcpy(accounts[next_slot].name, tok, sizeof(accounts[next_slot].name));

            do {
                tok = strtok_r(NULL, "=", &save);
                if(!tok)
                    continue;

                if(!strcmp(tok, "secret"))
                {
                    tok = strtok_r(NULL, "&", &save);
                    if((accounts[next_slot].sec_len = base32_decode(accounts[next_slot].secret, SECRET_MAX, tok)) <= 0)
                        goto fail;
                }
                else if(!strcmp(tok, "counter"))
                {
                    if(accounts[next_slot].is_totp)
                    {
                        rb->splash(HZ * 2, "WARNING: counter parameter specified for TOTP!?");
                        goto fail;
                    }
                    tok = strtok_r(NULL, "&", &save);
                    accounts[next_slot].hotp_counter = atoi(tok);
                }
            } while(tok);

            ++next_slot;

        fail:

            ;
        } while(1);
        rb->close(fd);
    }
    if(before == next_slot)
        rb->splash(HZ * 2, "No accounts added.");
    else
    {
        rb->splashf(HZ * 2, "Added %d account(s).", next_slot - before);
        save_accts();
    }
}

static void add_acct_manual(void)
{
    memset(accounts + next_slot, 0, sizeof(struct account_t));

    rb->splash(HZ * 1, "Enter account name.");
    if(rb->kbd_input(accounts[next_slot].name, sizeof(accounts[next_slot].name)) < 0)
        return;

    if(acct_exists(accounts[next_slot].name))
    {
        rb->splash(HZ * 2, "Duplicate account name!");
        return;
    }

    rb->splash(HZ * 2, "Enter base32-encoded secret.");

    char temp_buf[SECRET_MAX * 2];
    memset(temp_buf, 0, sizeof(temp_buf));

    if(rb->kbd_input(temp_buf, sizeof(temp_buf)) < 0)
        return;

    if((accounts[next_slot].sec_len = base32_decode(accounts[next_slot].secret, SECRET_MAX, temp_buf)) <= 0)
    {
        rb->splash(HZ * 2, "Invalid Base32 secret!");
        return;
    }

#ifdef CONFIG_RTC
    const struct text_message prompt = { (const char*[]) {"Is this a TOTP account?", "The protocol can be determined from the URI."}, 2};
    enum yesno_res response = rb->gui_syncyesno_run(&prompt, NULL, NULL);
    if(response == YESNO_NO)
        accounts[next_slot].is_totp = false;
    else
        accounts[next_slot].is_totp = true;
#endif

    memset(temp_buf, 0, sizeof(temp_buf));

    if(!accounts[next_slot].is_totp)
    {
        rb->splash(HZ * 2, "Enter counter (0 is normal).");
        temp_buf[0] = '0';
    }
    else
    {
        rb->splash(HZ * 2, "Enter time step (30 is normal).");
        temp_buf[0] = '3';
        temp_buf[1] = '0';
    }

    if(rb->kbd_input(temp_buf, sizeof(temp_buf)) < 0)
        return;

    if(!accounts[next_slot].is_totp)
        accounts[next_slot].hotp_counter = rb->atoi(temp_buf);
    else
        accounts[next_slot].totp_period = rb->atoi(temp_buf);

    rb->splash(HZ * 2, "Enter code length (6 is normal).");

    memset(temp_buf, 0, sizeof(temp_buf));
    temp_buf[0] = '6';

    if(rb->kbd_input(temp_buf, sizeof(temp_buf)) < 0)
        return;

    accounts[next_slot].digits = rb->atoi(temp_buf);

    if(accounts[next_slot].digits < 1 || accounts[next_slot].digits > 9)
    {
        rb->splash(HZ, "Invalid length!");
        return;
    }

    ++next_slot;

    save_accts();

    rb->splashf(HZ * 2, "Success.");
}

static void add_acct(void)
{
    MENUITEM_STRINGLIST(menu, "Add Account", NULL,
                        "From URI on disk",
                        "Manual Entry",
                        "Back");
    int sel = 0;
    bool quit = false;
    while(!quit)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            add_acct_file();
            break;
        case 1:
            add_acct_manual();
            break;
        case 2:
        default:
            quit = true;
            break;
        }
    }
}

static void show_code(int acct)
{
    if(!accounts[acct].is_totp)
    {
        rb->splashf(0, "%0*d", accounts[acct].digits, HOTP(accounts[acct].secret,
                                                           accounts[acct].sec_len,
                                                           accounts[acct].hotp_counter,
                                                           accounts[acct].digits));
        ++accounts[acct].hotp_counter;
    }
#ifdef CONFIG_RTC
    else
    {
        rb->splashf(0, "%0*d", accounts[acct].digits, TOTP(accounts[acct].secret,
                                                           accounts[acct].sec_len,
                                                           accounts[acct].totp_period,
                                                           accounts[acct].digits));
    }
#else
    else
    {
        rb->splash(0, "TOTP not supported on this device!");
    }
#endif
    rb->sleep(HZ * 2);
    while(1)
    {
        int button = rb->button_get(true);
        if(button && !(button & BUTTON_REL))
            break;
        rb->yield();
    }

    save_accts();
    rb->lcd_clear_display();
}

static void gen_codes(void)
{
    rb->lcd_clear_display();
    /* native menus don't seem to support dynamic names easily, so we
     * roll our own */
    static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };
    int idx = 0;
    if(next_slot > 0)
    {
        rb->lcd_putsf(0, 0, "%s", accounts[0].name);
        rb->lcd_update();
    }
    else
    {
        rb->splash(HZ * 2, "No accounts configured!");
        return;
    }
    while(1)
    {
        int button = pluginlib_getaction(-1, plugin_contexts, ARRAYLEN(plugin_contexts));
        switch(button)
        {
        case PLA_LEFT:
            --idx;
            if(idx < 0)
                idx = next_slot - 1;
            break;
        case PLA_RIGHT:
            ++idx;
            if(idx >= next_slot)
                idx = 0;
            break;
        case PLA_SELECT:
            show_code(idx);
            break;
        case PLA_CANCEL:
        case PLA_EXIT:
            return;
        default:
            break;
        }
        rb->lcd_clear_display();
        rb->lcd_putsf(0, 0, "%s", accounts[idx].name);
        rb->lcd_update();
    }
}

static bool danger_confirm(void)
{
    int sel = 0;
    MENUITEM_STRINGLIST(menu, "Are you REALLY SURE?", NULL,
                        "No",
                        "No",
                        "No",
                        "No",
                        "No",
                        "No",
                        "No",
                        "Yes, DO IT", // 7
                        "No",
                        "No",
                        "No",
                        "No");

    switch(rb->do_menu(&menu, &sel, NULL, false))
    {
    case 7:
        return true;
    default:
        return false;
    }
}

static void del_acct(void)
{
    rb->lcd_clear_display();
    /* native menus don't seem to support dynamic names easily, so we
     * roll our own */
    static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };
    int idx = 0;
    if(next_slot > 0)
    {
        rb->lcd_putsf(0, 0, "%s", accounts[0].name);
        rb->lcd_update();
    }
    else
    {
        rb->splash(HZ * 2, "No accounts configured!");
        return;
    }
    while(1)
    {
        int button = pluginlib_getaction(-1, plugin_contexts, ARRAYLEN(plugin_contexts));
        switch(button)
        {
        case PLA_LEFT:
            --idx;
            if(idx < 0)
                idx = next_slot - 1;
            break;
        case PLA_RIGHT:
            ++idx;
            if(idx >= next_slot)
                idx = 0;
            break;
        case PLA_SELECT:
            if(danger_confirm())
            {
                rb->memmove(accounts + idx, accounts + idx + 1, (next_slot - idx - 1) * sizeof(struct account_t));
                --next_slot;
                save_accts();
                rb->splashf(HZ, "Deleted.");
                if(!next_slot)
                    return;
            }
            else
                rb->splash(HZ, "Not confirmed.");
            break;
        case PLA_CANCEL:
        case PLA_EXIT:
            return;
        default:
            break;
        }
        rb->lcd_clear_display();
        rb->lcd_putsf(0, 0, "%s", accounts[idx].name);
        rb->lcd_update();
    }
}

static void adv_menu(void)
{
    MENUITEM_STRINGLIST(menu, "Advanced", NULL,
                        "Delete account",
                        "Delete ALL accounts",
                        "Back");

    bool quit = false;
    int sel = 0;
    while(!quit)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            del_acct();
            break;
        case 1:
            if(danger_confirm())
            {
                next_slot = 0;
                save_accts();
                rb->splash(HZ, "It is done, my master.");
            }
            else
                rb->splash(HZ, "Not confirmed.");
            break;
        case 2:
            quit = 1;
            break;
        default:
            break;
        }
    }
}

static void read_accts(void)
{
    int fd = rb->open(ACCT_FILE, O_RDONLY);
    if(fd < 0)
        return;
    while(1)
    {
        if(rb->read(fd, accounts + next_slot, sizeof(struct account_t)) != sizeof(struct account_t))
            break;
        ++next_slot;
    }
    rb->close(fd);
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;

    /* self-test with RFC 4226 values */
    if(HOTP("12345678901234567890", strlen("12345678901234567890"), 1, 6) != 287082)
    {
        return PLUGIN_ERROR;
    }

    read_accts();

    MENUITEM_STRINGLIST(menu, "One-Time Password Manager", NULL,
                        "Add Account",
                        "Generate",
                        "Advanced",
                        "Quit");

    bool quit = false;
    int sel = 0;
    while(!quit)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            add_acct();
            break;
        case 1:
            gen_codes();
            break;
        case 2:
            adv_menu();
            break;
        case 3:
            quit = 1;
            break;
        default:
            break;
        }
    }

    /* save to disk */
    save_accts();

    /* tell Rockbox that we have completed successfully */
    return PLUGIN_OK;
}
