/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Nils Wallménius
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
#include "plugin.h"
#include "lib/md5.h"
PLUGIN_HEADER


/* FIXME: use "PLUGIN_APPS_DIR" */
#define KEYBOX_FILE PLUGIN_DIR "/apps/keybox.dat"
#define BLOCK_SIZE 8
#define MAX_ENTRIES 12*BLOCK_SIZE /* keep this a multiple of BLOCK_SIZE */
#define FIELD_LEN 32 /* should be enough for anyone ;) */

/* salt 4 bytes (needed for decryption) not encrypted padded with 4 bytes of zeroes
   pwhash 16 bytes (to check for the right password) encrypted
   encrypted data. */

#define HEADER_LEN 24

enum
{
    FILE_OPEN_ERROR = -1
};

struct pw_entry
{
    bool used;
    char title[FIELD_LEN];
    char name[FIELD_LEN];
    char password[FIELD_LEN];
    struct pw_entry *next;
};

struct pw_list
{
    struct pw_entry first; /* always points to the first element in the list */
    struct pw_entry entries[MAX_ENTRIES];
    int num_entries;
} pw_list;

/* use this to access hashes in different ways, not byte order
   independent but does it matter? */
union hash
{
    uint8_t bytes[16];
    uint32_t words[4];
};

static const struct plugin_api* rb;
MEM_FUNCTION_WRAPPERS(rb);
static char buffer[sizeof(struct pw_entry)*MAX_ENTRIES];
static int bytes_read = 0; /* bytes read into the buffer */
static struct gui_synclist kb_list;
static union hash key;
static char master_pw[FIELD_LEN];
static uint32_t salt;
static union hash pwhash;
static bool data_changed = false;

static int context_item_cb(int action, const struct menu_item_ex *this_item);
static void encrypt_buffer(char *buf, size_t size, uint32_t *key);
static void decrypt_buffer(char *buf, size_t size, uint32_t *key);

/* the following two functions are the reference TEA implementation by
   David Wheeler and Roger Needham taken from 
   http://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm */

static void encrypt(uint32_t* v, uint32_t* k)
{
    uint32_t v0=v[0], v1=v[1], sum=0, i;           /* set up */
    static const uint32_t delta=0x9e3779b9;        /* a key schedule constant */
    uint32_t k0=k[0], k1=k[1], k2=k[2], k3=k[3];   /* cache key */
    for (i=0; i < 32; i++) {                            /* basic cycle start */
        sum += delta;
        v0 += ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
        v1 += ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);  /* end cycle */
    }
    v[0]=v0; v[1]=v1;
}

static void decrypt(uint32_t* v, uint32_t* k)
{
    uint32_t v0=v[0], v1=v[1], sum=0xC6EF3720, i;  /* set up */
    static const uint32_t delta=0x9e3779b9;        /* a key schedule constant */
    uint32_t k0=k[0], k1=k[1], k2=k[2], k3=k[3];   /* cache key */
    for (i=0; i<32; i++) {                              /* basic cycle start */
        v1 -= ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);
        v0 -= ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
        sum -= delta;                                   /* end cycle */
    }
    v[0]=v0; v[1]=v1;
}

MENUITEM_RETURNVALUE(context_add_entry, "Add entry", 0,
                     NULL, Icon_NOICON);
MENUITEM_RETURNVALUE(context_edit_title, "Edit title", 1,
                     &context_item_cb, Icon_NOICON);
MENUITEM_RETURNVALUE(context_edit_name, "Edit user name", 2,
                     &context_item_cb, Icon_NOICON);
MENUITEM_RETURNVALUE(context_edit_password, "Edit password", 3,
                     &context_item_cb, Icon_NOICON);
MENUITEM_RETURNVALUE(context_delete_entry, "Delete entry", 4,
                     &context_item_cb, Icon_NOICON);
MENUITEM_RETURNVALUE(context_debug, "debug", 5,
                     &context_item_cb, Icon_NOICON);

MAKE_MENU(context_m, "Context menu",
          context_item_cb, Icon_NOICON,
          &context_add_entry, &context_edit_title, &context_edit_name,
          &context_edit_password, &context_delete_entry);

static int context_item_cb(int action, const struct menu_item_ex *this_item)
{
    if (action == ACTION_REQUEST_MENUITEM
        && pw_list.num_entries == 0
        && this_item != &context_add_entry)
    {
        return ACTION_EXIT_MENUITEM;
    }
    return action;
}

static char * kb_list_cb(int selected_item, void *data,
                         char *buffer, size_t buffer_len)
{
    (void)data;
    int i;
    struct pw_entry *entry = pw_list.first.next;
    for (i = 0; i < selected_item; i++)
    {
        if (entry)
            entry = entry->next;
    }
    if (!entry)
        return NULL;
    
    rb->snprintf(buffer, buffer_len, "%s", entry->title);

    return buffer;
}

static void init_ll(void)
{
    pw_list.first.next = &pw_list.entries[0];
    pw_list.entries[0].next = NULL;
    pw_list.num_entries = 0;
}

static void delete_entry(int selected_item)
{
    int i;
    struct pw_entry *entry = &pw_list.first;
    struct pw_entry *entry2;

    /* find the entry before the one to delete */
    for (i = 0; i < selected_item; i++)
    {
        if (entry->next)
            entry = entry->next;
    }
    entry2 = entry->next;
    if (!entry2)
        return;
    
    entry->next = entry2->next;

    entry2->used = false;
    entry2->name[0] = '\0';
    entry2->password[0] = '\0';
    entry2->next = NULL;

    rb->gui_synclist_set_nb_items(&kb_list, --pw_list.num_entries);
    data_changed = true;
}

static void add_entry(int selected_item)
{
    int i, j;
    struct pw_entry *entry = pw_list.first.next;
    for (i = 0; i < MAX_ENTRIES && pw_list.entries[i].used; i++)
        ;

    if (pw_list.entries[i].used)
    {
        rb->splash(HZ, "Password list full");
        return;
    }

    rb->splash(HZ, "Enter title");
    pw_list.entries[i].title[0] = '\0';
    rb->kbd_input(pw_list.entries[i].title, FIELD_LEN);
    rb->splash(HZ, "Enter name");
    pw_list.entries[i].name[0] = '\0';
    rb->kbd_input(pw_list.entries[i].name, FIELD_LEN);
    rb->splash(HZ, "Enter password");
    pw_list.entries[i].password[0] = '\0';
    rb->kbd_input(pw_list.entries[i].password, FIELD_LEN);

    for (j = 0; j < selected_item; j++)
    {
        if (entry->next)
            entry = entry->next;
    }

    rb->gui_synclist_set_nb_items(&kb_list, ++pw_list.num_entries);

    pw_list.entries[i].used = true;
    pw_list.entries[i].next = entry->next;

    entry->next = &pw_list.entries[i];

    if (entry->next == entry)
        entry->next = NULL;

    data_changed = true;
}

static void edit_title(int selected_item)
{
    int i;
    struct pw_entry *entry = pw_list.first.next;
    for (i = 0; i < selected_item; i++)
    {
        if (entry->next)
            entry = entry->next;
    }
    if (rb->kbd_input(entry->title, FIELD_LEN) == 0)
        data_changed = true;
}

static void edit_name(int selected_item)
{
    int i;
    struct pw_entry *entry = pw_list.first.next;
    for (i = 0; i < selected_item; i++)
    {
        if (entry->next)
            entry = entry->next;
    }
    if (rb->kbd_input(entry->name, FIELD_LEN) == 0)
        data_changed = true;
}

static void edit_pw(int selected_item)
{
    int i;
    struct pw_entry *entry = pw_list.first.next;
    for (i = 0; i < selected_item; i++)
    {
        if (entry->next)
            entry = entry->next;
    }
    if (rb->kbd_input(entry->password, FIELD_LEN) == 0)
        data_changed = true;
}

static void context_menu(int selected_item)
{
    int selection, result;
    bool exit = false;

    do {
        result = rb->do_menu(&context_m, &selection, NULL, false);
        switch (result) {
        case 0:
            add_entry(selected_item);
            return;
        case 1:
            edit_title(selected_item);
            return;
        case 2:
            edit_name(selected_item);
            return;
        case 3:
            edit_pw(selected_item);
            return;
        case 4:
            delete_entry(selected_item);
            return;
        default:
            exit = true;
            break;
        }
        rb->yield();
    } while (!exit);
}

static void splash_pw(int selected_item)
{
    int i;
    struct pw_entry *entry = pw_list.first.next;

    for (i = 0; i < selected_item; i++)
    {
        if (entry->next)
            entry = entry->next;
    }
    if (entry->name != '\0')
        rb->splashf(0, "%s  %s", entry->name, entry->password);
    else
        rb->splashf(0, "%s", entry->password);
    rb->get_action(CONTEXT_STD, TIMEOUT_BLOCK);
}

static void hash_pw(union hash *out)
{
    int i;
    struct md5_s pw_md5;

    InitMD5(&pw_md5);
    AddMD5(&pw_md5, master_pw, rb->strlen(master_pw));
    EndMD5(&pw_md5);

    for (i = 0; i < 4; i++)
        out->words[i] = htole32(pw_md5.p_digest[i]);
}

static void make_key(void)
{
    int i;
    char buf[sizeof(master_pw) + sizeof(salt) + 1];
    struct md5_s key_md5;
    size_t len = rb->strlen(master_pw);

    rb->strncpy(buf, master_pw, sizeof(buf));

    rb->memcpy(&buf[len], &salt, sizeof(salt));

    InitMD5(&key_md5);
    AddMD5(&key_md5, buf, rb->strlen(buf));
    EndMD5(&key_md5);

    for (i = 0; i < 4; i++)
        key.words[i] = key_md5.p_digest[i];
}

static void decrypt_buffer(char *buf, size_t size, uint32_t *key)
{
    unsigned int i;
    uint32_t block[2];

    for (i = 0; i < size/BLOCK_SIZE; i++)
    {
        rb->memcpy(&block[0], &buf[i*BLOCK_SIZE], sizeof(block));

        block[0] = letoh32(block[0]);
        block[1] = letoh32(block[1]);

        decrypt(&block[0], key);

        /* byte swap one block */
        block[0] = letoh32(block[0]);
        block[1] = letoh32(block[1]);

        rb->memcpy(&buf[i*BLOCK_SIZE], &block[0], sizeof(block));
    }
}

static void encrypt_buffer(char *buf, size_t size, uint32_t *key)
{
    unsigned int i;
    uint32_t block[2];

    for (i = 0; i < size/BLOCK_SIZE; i++)
    {
        rb->memcpy(&block[0], &buf[i*BLOCK_SIZE], sizeof(block));

        /* byte swap one block */
        block[0] = htole32(block[0]);
        block[1] = htole32(block[1]);

        encrypt(&block[0], key);

        block[0] = htole32(block[0]);
        block[1] = htole32(block[1]);

        rb->memcpy(&buf[i*BLOCK_SIZE], &block[0], sizeof(block));
    }
}

static int parse_buffer(void)
{
    int i;
    int len;
    struct pw_entry *entry = pw_list.first.next;
    char *start, *end;
    start = &buffer[HEADER_LEN];

    rb->memcpy(&salt, &buffer[0], sizeof(salt));
    make_key();

    decrypt_buffer(&buffer[8], bytes_read - 8, &key.words[0]);

    if (rb->memcmp(&buffer[8], &pwhash, sizeof(union hash)))
    {
        rb->splash(HZ*2, "Wrong password");
        return -1;
    }

    for (i = 0; i < MAX_ENTRIES; i++)
    {
        end = rb->strchr(start, '\0'); /* find eol */
        len = end - &buffer[HEADER_LEN];
        if ((len > bytes_read + HEADER_LEN) || start == end)
        {
            break;
        }

        rb->strncpy(entry->title, start, FIELD_LEN);
        start = end + 1;

        end = rb->strchr(start, '\0'); /* find eol */
        len = end - &buffer[HEADER_LEN];
        if (len > bytes_read + HEADER_LEN)
        {
            break;
        }

        rb->strncpy(entry->name, start, FIELD_LEN);
        start = end + 1;

        end = rb->strchr(start, '\0'); /* find eol */
        len = end - &buffer[HEADER_LEN];
        if (len > bytes_read + HEADER_LEN)
        {
            break;
        }
        rb->strncpy(entry->password, start, FIELD_LEN);
        start = end + 1;
        entry->used = true;
        if (i + 1 < MAX_ENTRIES - 1)
        {
            entry->next = &pw_list.entries[i+1];
            entry = entry->next;
        }
        else
        {
            break;
        }
    }
    entry->next = NULL;
    pw_list.num_entries = i;
    rb->gui_synclist_set_nb_items(&kb_list, pw_list.num_entries);
    return 0;
}

static void write_output(int fd)
{
    size_t bytes_written;
    int i;
    size_t len, size;
    char *p = &buffer[HEADER_LEN]; /* reserve space for salt + hash */

    rb->memcpy(&buffer[8], &pwhash, sizeof(union hash));
    struct pw_entry *entry = pw_list.first.next;

    for (i = 0; i < pw_list.num_entries; i++)
    {
        len = rb->strlen(entry->title);
        rb->strncpy(p, entry->title, len+1);
        p += len+1;
        len = rb->strlen(entry->name);
        rb->strncpy(p, entry->name, len+1);
        p += len+1;
        len = rb->strlen(entry->password);
        rb->strncpy(p, entry->password, len+1);
        p += len+1;
        if (entry->next)
            entry = entry->next;
    }
    *p++ = '\0'; /* mark the end of the list */

    /* round up to a number divisible by BLOCK_SIZE */
    size = ((p - buffer + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;

    salt = rb->rand();
    make_key();

    encrypt_buffer(&buffer[8], size, &key.words[0]);
    rb->memcpy(&buffer[0], &salt, sizeof(salt));

    bytes_written = rb->write(fd, &buffer, size);
}

static int enter_pw(char *pw_buf, size_t buflen, bool new_pw)
{
    char buf[2][sizeof(master_pw)];
    rb->memset(buf, 0, sizeof(buf));
    rb->memset(master_pw, 0, sizeof(master_pw));

    if (new_pw)
    {
        rb->splash(HZ, "Enter new master password");
        rb->kbd_input(buf[0], sizeof(buf[0]));
        rb->splash(HZ, "Confirm master password");
        rb->kbd_input(buf[1], sizeof(buf[1]));

        if (rb->strcmp(buf[0], buf[1]))
        {
            rb->splash(HZ, "Password mismatch");
            return -1;
        }
        else
        {
            rb->strncpy(pw_buf, buf[0], buflen);
            hash_pw(&pwhash);
            return 0;
        }
    }

    rb->splash(HZ, "Enter master password");
    if (rb->kbd_input(pw_buf, buflen))
        return -1;
    hash_pw(&pwhash);
    return 0;
}

static int keybox(void)
{
    int button, fd;
    bool new_file = !rb->file_exists(KEYBOX_FILE);
    bool done = false;

    if (enter_pw(master_pw, sizeof (master_pw), new_file))
        return 0;

    /* Read the existing file */
    if (!new_file)
    {
        fd = rb->open(KEYBOX_FILE, O_RDONLY);
        if (fd < 0)
            return FILE_OPEN_ERROR;
        bytes_read = rb->read(fd, &buffer, sizeof(buffer));

        if (parse_buffer())
            return 0;

        rb->close(fd);
    }

    while (!done)
    {
        rb->gui_synclist_draw(&kb_list);
        button = rb->get_action(CONTEXT_LIST, TIMEOUT_BLOCK);
        if (rb->gui_synclist_do_button(&kb_list, &button, LIST_WRAP_ON))
            continue;

        switch (button)
        {
            case ACTION_STD_OK:
                splash_pw(rb->gui_synclist_get_sel_pos(&kb_list));
                break;
            case ACTION_STD_CONTEXT:
                context_menu(rb->gui_synclist_get_sel_pos(&kb_list));
                break;
            case ACTION_STD_CANCEL:
                done = true;
                break;
        }
        rb->yield();
    }

    if (data_changed)
    {
        fd = rb->open(KEYBOX_FILE, O_WRONLY | O_CREAT | O_TRUNC);
        if (fd < 0)
            return FILE_OPEN_ERROR;
        write_output(fd);
        rb->close(fd);
    }

    return 0;
}

static void reset(void)
{
    static const char *message_lines[]=
        {"Do you really want", "to reset keybox?"};
    static const char *yes_lines[]=
        {"Keybox reset."};
    static const struct text_message message={message_lines, 2};
    static const struct text_message yes_message={yes_lines, 1};

    if(rb->gui_syncyesno_run(&message, &yes_message, NULL) == YESNO_YES)
    {
        rb->remove(KEYBOX_FILE);
        rb->memset(&buffer, 0, sizeof(buffer));
        rb->memset(&pw_list, 0, sizeof(pw_list));
        init_ll();
    }
}

static int main_menu(void)
{
    int selection, result, ret;
    bool exit = false;

    MENUITEM_STRINGLIST(menu,"Keybox", NULL, "Enter Keybox",
                        "Reset Keybox", "Exit");

    do {
        result = rb->do_menu(&menu, &selection, NULL, false);
        switch (result) {
        case 0:
            ret = keybox();
            if (ret)
                return ret;
            break;
        case 1:
            reset();
            break;
        case 2:
            exit = true;
            break;
        }
        rb->yield();
    } while (!exit);

    return 0;
}

enum plugin_status plugin_start(const struct plugin_api *api,
                                const void *parameter)
{
    (void)parameter;
    rb = api;
    int ret;

    rb->gui_synclist_init(&kb_list, &kb_list_cb, NULL, false, 1, NULL);

    rb->gui_synclist_set_title(&kb_list, "Keybox", NOICON);
    rb->gui_synclist_set_icon_callback(&kb_list, NULL);
    rb->gui_synclist_set_nb_items(&kb_list, 0);
    rb->gui_synclist_limit_scroll(&kb_list, false);
    rb->gui_synclist_select_item(&kb_list, 0);

    md5_init(api);

    init_ll();
    ret = main_menu();

    switch (ret)
    {
        case FILE_OPEN_ERROR:
            rb->splash(HZ*2, "Error opening file");
            return PLUGIN_ERROR;
    }

    return PLUGIN_OK;
}

