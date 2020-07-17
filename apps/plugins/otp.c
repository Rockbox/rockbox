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

/* simple OTP plugin */

/* see RFCs 4226, 6238 for more information about the algorithms used */

#include "plugin.h"

#include "lib/display_text.h"
#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_exit.h"
#include "lib/sha1.h"

#define MAX_NAME   50
#define SECRET_MAX 256
#define URI_MAX    256
#define ACCT_FILE  PLUGIN_APPS_DATA_DIR "/otp.dat"

#define MAX(a, b) (((a)>(b))?(a):(b))

struct account_t {
    char name[MAX_NAME];

    bool is_totp; // hotp otherwise

    union {
        uint64_t hotp_counter;
        int totp_period;
    };

    int digits;

    unsigned char secret[SECRET_MAX];
    int sec_len;
};

static int max_accts = 0;

/* in plugin buffer */
static struct account_t *accounts = NULL;

static int next_slot = 0;

/* in SECONDS, asked for on first run */
static int time_offs = 0;

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

#if CONFIG_RTC
static time_t get_utc(void)
{
    return rb->mktime(rb->get_time()) - time_offs;
}

static int TOTP(unsigned char *secret, size_t sec_len, uint64_t step, int digits)
{
    uint64_t tm = get_utc() / step;
    return HOTP(secret, sec_len, tm, digits);
}
#endif

/* search the accounts for a duplicate */
static bool acct_exists(const char *name)
{
    for(int i = 0; i < next_slot; ++i)
        if(!rb->strcmp(accounts[i].name, name))
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

static int base32_encode(const uint8_t *data, int length, uint8_t *result,
                         int bufSize) {
  if (length < 0 || length > (1 << 28)) {
    return -1;
  }
  int count = 0;
  if (length > 0) {
    int buffer = data[0];
    int next = 1;
    int bitsLeft = 8;
    while (count < bufSize && (bitsLeft > 0 || next < length)) {
      if (bitsLeft < 5) {
        if (next < length) {
          buffer <<= 8;
          buffer |= data[next++] & 0xFF;
          bitsLeft += 8;
        } else {
          int pad = 5 - bitsLeft;
          buffer <<= pad;
          bitsLeft += pad;
        }
      }
      int index = 0x1F & (buffer >> (bitsLeft - 5));
      bitsLeft -= 5;
      result[count++] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"[index];
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

static bool read_accts(void)
{
    int fd = rb->open(ACCT_FILE, O_RDONLY);
    if(fd < 0)
        return false;

    char buf[4];
    char magic[4] = { 'O', 'T', 'P', '1' };
    rb->read(fd, buf, 4);
    if(memcmp(magic, buf, 4))
    {
        rb->splash(HZ * 2, "Corrupt save data!");
        rb->close(fd);
        return false;
    }

    rb->read(fd, &time_offs, sizeof(time_offs));

    while(next_slot < max_accts)
    {
        if(rb->read(fd, accounts + next_slot, sizeof(struct account_t)) != sizeof(struct account_t))
            break;
        ++next_slot;
    }

    rb->close(fd);
    return true;
}

static void save_accts(void)
{
    int fd = rb->open(ACCT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    rb->fdprintf(fd, "OTP1");

    rb->write(fd, &time_offs, sizeof(time_offs));

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

            if(next_slot >= max_accts)
            {
                rb->splash(HZ * 2, "Account limit reached: some accounts not added.");
                break;
            }

            /* check for URI prefix */
            if(rb->strncmp(uri_buf, "otpauth://", 10))
                continue;

            char *save;
            char *tok = rb->strtok_r(uri_buf + 10, "/", &save);
            if(!rb->strcmp(tok, "totp"))
            {
                accounts[next_slot].is_totp = true;
                accounts[next_slot].totp_period = 30;
#if !CONFIG_RTC
                rb->splash(2 * HZ, "TOTP not supported!");
                continue;
#endif
            }
            else if(!rb->strcmp(tok, "hotp"))
            {
                accounts[next_slot].is_totp = false;
                accounts[next_slot].hotp_counter = 0;
            }

            tok = rb->strtok_r(NULL, "?", &save);
            if(!tok)
                continue;

            if(acct_exists(tok))
            {
                rb->splashf(HZ * 2, "Not adding account with duplicate name `%s'!", tok);
                continue;
            }

            if(!rb->strlen(tok))
            {
                rb->splashf(HZ * 2, "Skipping account with empty name.");
                continue;
            }

            rb->strlcpy(accounts[next_slot].name, tok, sizeof(accounts[next_slot].name));

            bool have_secret = false;

            do {
                tok = rb->strtok_r(NULL, "=", &save);
                if(!tok)
                    continue;

                if(!rb->strcmp(tok, "secret"))
                {
                    if(have_secret)
                    {
                        rb->splashf(HZ * 2, "URI with multiple `secret' parameters found, skipping!");
                        goto fail;
                    }
                    have_secret = true;
                    tok = rb->strtok_r(NULL, "&", &save);
                    if((accounts[next_slot].sec_len = base32_decode(accounts[next_slot].secret, SECRET_MAX, tok)) <= 0)
                        goto fail;
                }
                else if(!rb->strcmp(tok, "counter"))
                {
                    if(accounts[next_slot].is_totp)
                    {
                        rb->splash(HZ * 2, "Counter parameter specified for TOTP!? Skipping...");
                        goto fail;
                    }
                    tok = rb->strtok_r(NULL, "&", &save);
                    accounts[next_slot].hotp_counter = rb->atoi(tok);
                }
                else if(!rb->strcmp(tok, "period"))
                {
                    if(!accounts[next_slot].is_totp)
                    {
                        rb->splash(HZ * 2, "Period parameter specified for HOTP!? Skipping...");
                        goto fail;
                    }
                    tok = rb->strtok_r(NULL, "&", &save);
                    accounts[next_slot].totp_period = rb->atoi(tok);
                }
                else if(!rb->strcmp(tok, "digits"))
                {
                    tok = rb->strtok_r(NULL, "&", &save);
                    accounts[next_slot].digits = rb->atoi(tok);
                    if(accounts[next_slot].digits < 1 || accounts[next_slot].digits > 9)
                    {
                        rb->splashf(HZ * 2, "Digits parameter not in acceptable range, skipping.");
                        goto fail;
                    }
                }
                else
                    rb->splashf(HZ, "Unnown parameter `%s' ignored.", tok);
            } while(tok);

            if(!have_secret)
            {
                rb->splashf(HZ * 2, "URI with NO `secret' parameter found, skipping!");
                goto fail;
            }

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
    if(next_slot >= max_accts)
    {
        rb->splashf(HZ * 2, "Account limit reached!");
        return;
    }
    memset(accounts + next_slot, 0, sizeof(struct account_t));

    rb->splash(HZ * 1, "Enter account name.");
    char* buf = accounts[next_slot].name;
    if(rb->kbd_input(buf, sizeof(accounts[next_slot].name), NULL) < 0)
        return;

    if(acct_exists(buf))
    {
        rb->splash(HZ * 2, "Duplicate account name!");
        return;
    }

    rb->splash(HZ * 2, "Enter base32-encoded secret.");

    char temp_buf[SECRET_MAX * 2];
    memset(temp_buf, 0, sizeof(temp_buf));

    if(rb->kbd_input(temp_buf, sizeof(temp_buf), NULL) < 0)
        return;

    if((accounts[next_slot].sec_len = base32_decode(accounts[next_slot].secret, SECRET_MAX, temp_buf)) <= 0)
    {
        rb->splash(HZ * 2, "Invalid Base32 secret!");
        return;
    }

#if CONFIG_RTC
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

    if(rb->kbd_input(temp_buf, sizeof(temp_buf), NULL) < 0)
        return;

    if(!accounts[next_slot].is_totp)
        accounts[next_slot].hotp_counter = rb->atoi(temp_buf);
    else
        accounts[next_slot].totp_period = rb->atoi(temp_buf);

    rb->splash(HZ * 2, "Enter code length (6 is normal).");

    memset(temp_buf, 0, sizeof(temp_buf));
    temp_buf[0] = '6';

    if(rb->kbd_input(temp_buf, sizeof(temp_buf), NULL) < 0)
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
        rb->splashf(0, "%0*d", accounts[acct].digits,
                               HOTP(accounts[acct].secret,
                                    accounts[acct].sec_len,
                                    accounts[acct].hotp_counter,
                                    accounts[acct].digits));
        ++accounts[acct].hotp_counter;
    }
#if CONFIG_RTC
    else
    {
        rb->splashf(0, "%0*d (%ld seconds(s) left)", accounts[acct].digits,
                                                     TOTP(accounts[acct].secret,
                                                          accounts[acct].sec_len,
                                                          accounts[acct].totp_period,
                                                          accounts[acct].digits),
                    accounts[acct].totp_period - get_utc() % accounts[acct].totp_period);
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
        rb->lcd_putsf(0, 0, "Generate Code");
        rb->lcd_putsf(0, 1, "%s", accounts[0].name);
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
            exit_on_usb(button);
            return;
        default:
            break;
        }
        rb->lcd_clear_display();
        rb->lcd_putsf(0, 0, "Generate Code");
        rb->lcd_putsf(0, 1, "%s", accounts[idx].name);
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

char data_buf[MAX(MAX_NAME, SECRET_MAX * 2)];
char temp_sec[SECRET_MAX];
size_t old_len;

static void edit_menu(int acct)
{
    rb->splashf(HZ, "Editing account `%s'.", accounts[acct].name);

    /* HACK ALERT */
    /* two different menus, one handling logic */
    MENUITEM_STRINGLIST(menu_1, "Edit Account", NULL,
                        "Rename",
                        "Delete",
                        "Change HOTP Counter",
                        "Change Digit Count",
                        "Change Shared Secret",
                        "Back");

    MENUITEM_STRINGLIST(menu_2, "Edit Account", NULL,
                        "Rename", // 0
                        "Delete", // 1
                        "Change TOTP Period", // 2
                        "Change Digit Count", // 3
                        "Change Shared Secret", // 4
                        "Back"); // 5

    const struct menu_item_ex *menu = (accounts[acct].is_totp) ? &menu_2 : &menu_1;

    bool quit = false;
    int sel = 0;
    while(!quit)
    {
        switch(rb->do_menu(menu, &sel, NULL, false))
        {
        case 0: // rename
            rb->splash(HZ, "Enter new name.");
            rb->strlcpy(data_buf, accounts[acct].name, sizeof(data_buf));
            if(rb->kbd_input(data_buf, sizeof(data_buf), NULL) < 0)
                break;
            if(acct_exists(data_buf))
            {
                rb->splash(HZ * 2, "Duplicate account name!");
                break;
            }
            rb->strlcpy(accounts[acct].name, data_buf, sizeof(accounts[acct].name));
            save_accts();
            break;
        case 1: // delete
            if(danger_confirm())
            {
                rb->memmove(accounts + acct, accounts + acct + 1, (next_slot - acct - 1) * sizeof(struct account_t));
                --next_slot;
                save_accts();
                rb->splashf(HZ, "Deleted.");
                return;
            }
            else
                rb->splash(HZ, "Not confirmed.");
            break;
        case 2: // HOTP counter OR TOTP period
            if(accounts[acct].is_totp)
                rb->snprintf(data_buf, sizeof(data_buf), "%d", (int)accounts[acct].hotp_counter);
            else
                rb->snprintf(data_buf, sizeof(data_buf), "%d", accounts[acct].totp_period);

            if(rb->kbd_input(data_buf, sizeof(data_buf), NULL) < 0)
                break;

            if(accounts[acct].is_totp)
                accounts[acct].totp_period = rb->atoi(data_buf);
            else
                accounts[acct].hotp_counter = rb->atoi(data_buf);

            save_accts();

            rb->splash(HZ, "Success.");
            break;
        case 3: // digits
            rb->snprintf(data_buf, sizeof(data_buf), "%d", accounts[acct].digits);
            if(rb->kbd_input(data_buf, sizeof(data_buf), NULL) < 0)
                break;

            accounts[acct].digits = rb->atoi(data_buf);
            save_accts();

            rb->splash(HZ, "Success.");
            break;
        case 4: // secret
            old_len = accounts[acct].sec_len;
            memcpy(temp_sec, accounts[acct].secret, accounts[acct].sec_len);
            base32_encode(accounts[acct].secret, accounts[acct].sec_len, data_buf, sizeof(data_buf));

            if(rb->kbd_input(data_buf, sizeof(data_buf), NULL) < 0)
                break;

            int ret = base32_decode(accounts[acct].secret, sizeof(accounts[acct].secret), data_buf);
            if(ret <= 0)
            {
                memcpy(accounts[acct].secret, temp_sec, SECRET_MAX);
                accounts[acct].sec_len = old_len;
                rb->splash(HZ * 2, "Invalid Base32 secret!");
                break;
            }
            accounts[acct].sec_len = ret;

            save_accts();
            rb->splash(HZ, "Success.");
            break;
        case 5:
            quit = true;
            break;
        default:
            break;
        }
    }
}

static void edit_accts(void)
{
    rb->lcd_clear_display();
    /* native menus don't seem to support dynamic names easily, so we
     * roll our own */
    static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };
    int idx = 0;
    if(next_slot > 0)
    {
        rb->lcd_putsf(0, 0, "Edit Account");
        rb->lcd_putsf(0, 1, "%s", accounts[0].name);
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
            edit_menu(idx);
            if(!next_slot)
                return;
            if(idx == next_slot)
                idx = 0;
            break;
        case PLA_CANCEL:
        case PLA_EXIT:
            return;
        default:
            exit_on_usb(button);
            break;
        }
        rb->lcd_clear_display();
        rb->lcd_putsf(0, 0, "Edit Account");
        rb->lcd_putsf(0, 1, "%s", accounts[idx].name);
        rb->lcd_update();
    }
}

#if CONFIG_RTC

/* label is like this: [+/-]HH:MM ... */
static int get_time_seconds(const char *label)
{
    if(!rb->strcmp(label, "UTC"))
        return 0;

    char buf[32];

    /* copy the part after "UTC" */
    rb->strlcpy(buf, label + 3, sizeof(buf));

    char *save, *tok;

    tok = rb->strtok_r(buf, ":", &save);
    /* positive or negative: sign left */
    int hr = rb->atoi(tok);

    tok = rb->strtok_r(NULL, ": ", &save);
    int min = rb->atoi(tok);

    return 3600 * hr + 60 * min;
}

/* returns the offset in seconds associated with a time zone */
static int get_time_offs(void)
{
    MENUITEM_STRINGLIST(menu, "Select Time Offset", NULL,
                        "UTC-12:00", // 0
                        "UTC-11:00", // 1
                        "UTC-10:00 (HAST)", // 2
                        "UTC-9:30",  // 3
                        "UTC-9:00 (AKST, HADT)", // 4
                        "UTC-8:00 (PST, AKDT)", // 5
                        "UTC-7:00 (MST, PDT)", // 6
                        "UTC-6:00 (CST, MDT)", // 7
                        "UTC-5:00 (EST, CDT)", // 8
                        "UTC-4:00 (AST, EDT)", // 9
                        "UTC-3:30 (NST)", // 10
                        "UTC-3:00 (ADT)", // 11
                        "UTC-2:30 (NDT)", // 12
                        "UTC-2:00", // 13
                        "UTC-1:00", // 14
                        "UTC",      // 15
                        "UTC+1:00", // 16
                        "UTC+2:00", // 17
                        "UTC+3:00", // 18
                        "UTC+3:30", // 19
                        "UTC+4:00", // 20
                        "UTC+4:30", // 21
                        "UTC+5:00", // 22
                        "UTC+5:30", // 23
                        "UTC+5:45", // 24
                        "UTC+6:00", // 25
                        "UTC+6:30", // 26
                        "UTC+7:00", // 27
                        "UTC+8:00", // 28
                        "UTC+8:30", // 29
                        "UTC+8:45", // 30
                        "UTC+9:00", // 31
                        "UTC+9:30", // 32
                        "UTC+10:00", // 33
                        "UTC+10:30", // 34
                        "UTC+11:00", // 35
                        "UTC+12:00", // 36
                        "UTC+12:45", // 37
                        "UTC+13:00", // 38
                        "UTC+14:00", // 39
        );

    int sel = 0;
    for(unsigned int i = 0; i < ARRAYLEN(menu_); ++i)
        if(time_offs == get_time_seconds(menu_[i]))
        {
            sel = i;
            break;
        }

    /* relies on menu internals */
    rb->do_menu(&menu, &sel, NULL, false);

    /* see apps/menu.h */
    const char *label = menu_[sel];

    return get_time_seconds(label);

#if 0
    /* provided in case menu internals change */
    switch(rb->do_menu(&menu, &sel, NULL, false))
    {
    case 0: case 1: case 2:
        return (sel - 12) * 3600;
    case 3:
        return -9 * 3600 - 30 * 60;
    case 4: case 5: case 6: case 7: case 8: case 9:
        return (sel - 13) * 3600;
    case 10:
        return -3 * 3600 - 30 * 60;
    case 11:
        return -3 * 3600;
    case 12:
        return -3 * 3600 - 30 * 60;
    case 13: case 14: case 15: case 16: case 17: case 18:
        return (sel - 15) * 3600;

    case 19:
        return 3 * 3600 + 30 * 60;
    case 20:
        return 4 * 3600;
    case 21:
        return 4 * 3600 + 30 * 60;
    case 22:
        return 5 * 3600;
    case 23:
        return 5 * 3600 + 30 * 60;
    case 24:
        return 5 * 3600 + 45 * 60;
    case 25:
        return 6 * 3600;
    case 26:
        return 6 * 3600 + 30 * 60;
    case 27: case 28:
        return (sel - 20) * 3600;
    case 29:
        return 8 * 3600 + 30 * 60;
    case 30:
        return 8 * 3600 + 45 * 60;
    case 31:
        return 9 * 3600;
    case 32:
        return 9 * 3600 + 30 * 60;
    case 33:
        return 10 * 3600;
    case 34:
        return 10 * 3600 + 30 * 60;
    case 35: case 36:
        return (sel - 24) * 3600;
    case 37:
        return 12 * 3600 + 45 * 60;
    case 38: case 39:
        return (sel - 25) * 3600;
    default:
        rb->splash(0, "BUG: time zone fall-through: REPORT ME!!!");
        break;
    }
    return 0;
#endif
}
#endif

static void adv_menu(void)
{
    MENUITEM_STRINGLIST(menu, "Advanced", NULL,
                        "Edit Account",
                        "Delete ALL accounts",
#if CONFIG_RTC
                        "Change Time Offset",
#endif
                        "Back");

    bool quit = false;
    int sel = 0;
    while(!quit)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            edit_accts();
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
#if CONFIG_RTC
        case 2:
            time_offs = get_time_offs();
            break;
        case 3:
#else
        case 2:
#endif
            quit = 1;
            break;
        default:
            break;
        }
    }
}

/* displays the help text */
static void show_help(void)
{

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
#endif

    rb->lcd_setfont(FONT_UI);

    static char *help_text[] = { "One-Time Password Manager", "",
                                 "Introduction", "",
                                 "This", "plugin", "allows", "you", "to", "generate", "one-time", "passwords", "to", "provide", "a", "second", "factor", "of", "authentication", "for", "services", "that", "support", "it.",
                                 "It", "suppports", "both", "event-based", "(HOTP),", "and", "time-based", "(TOTP)", "password", "schemes.",
                                 "In", "order", "to", "ensure", "proper", "functioning", "of", "time-based", "passwords", "ensure", "that", "the", "clock", "is", "accurate", "to", "within", "30", "seconds", "of", "actual", "time."
                                 "Note", "that", "some", "devices", "lack", "a", "real-time", "clock,", "so", "time-based", "passwords", "are", "not", "supported", "on", "those", "targets." };

    struct style_text style[] = {
        {0,  TEXT_CENTER | TEXT_UNDERLINE},
        {2,  C_RED},
        LAST_STYLE_ITEM
    };

    display_text(ARRAYLEN(help_text), help_text, style, NULL, true);
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;

    /* self-test with RFC 4226 values */
    if(HOTP("12345678901234567890", rb->strlen("12345678901234567890"), 1, 6) != 287082)
    {
        return PLUGIN_ERROR;
    }

    size_t bufsz;
    accounts = rb->plugin_get_buffer(&bufsz);
    max_accts = bufsz / sizeof(struct account_t);

    if(!read_accts())
#if CONFIG_RTC
    {
        time_offs = get_time_offs();
    }
#else
    {
        ;
    }
#endif

    MENUITEM_STRINGLIST(menu, "One-Time Password Manager", NULL,
                        "Add Account",
                        "Generate Code",
                        "Help",
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
            show_help();
            break;
        case 3:
            adv_menu();
            break;
        case 4:
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
