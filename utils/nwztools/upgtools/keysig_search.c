/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 Amaury Pouly
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
#include "keysig_search.h"
#include "misc.h"
#include "mg.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#define cprintf(col, ...) do {color(col); printf(__VA_ARGS__); }while(0)

/** Generic search code */

/* The generator sends chunks to the workers. The exact type of chunks depends
 * on the method used. */
static struct
{
    pthread_mutex_t mutex; /* mutex for the whole structure */
    pthread_cond_t avail_cond; /* condition to signal available or stop */
    pthread_cond_t req_cond; /* condition to signal request or stop */
    bool stop; /* if true, stop searcg */
    void *chunk; /* pointer to chunk (NULL if not available) */
    size_t chunk_sz; /* chunk size */
}g_producer;

/* init producer */
static void producer_init(void)
{
    pthread_cond_init(&g_producer.avail_cond, NULL);
    pthread_cond_init(&g_producer.req_cond, NULL);
    pthread_mutex_init(&g_producer.mutex, NULL);
    g_producer.stop = false;
    g_producer.chunk = NULL;
    g_producer.chunk_sz = 0;
}

/* consumer get: called by worker to get a new chunk, return NULL to stop */
static void *consumer_get(size_t *sz)
{
    pthread_mutex_lock(&g_producer.mutex);
    /* loop until stop or new chunk */
    while(true)
    {
        /* stop if requested */
        if(g_producer.stop)
        {
            pthread_mutex_unlock(&g_producer.mutex);
            return NULL;
        }
        if(g_producer.chunk)
            break;
        /* request a new chunk */
        pthread_cond_signal(&g_producer.req_cond);
        /* wait for availability */
        pthread_cond_wait(&g_producer.avail_cond, &g_producer.mutex);
    }
    void *c = g_producer.chunk;
    if(sz)
        *sz = g_producer.chunk_sz;
    g_producer.chunk = NULL;
    pthread_mutex_unlock(&g_producer.mutex);
    /* request a new chunk, so that if other consumers are waiting, the producer
     * will also wake them up */
    pthread_cond_signal(&g_producer.req_cond);
    return c;
}

/* stop: called by worker to stop the search */
static void consumer_stop(void)
{
    pthread_mutex_lock(&g_producer.mutex);
    /* set stop */
    g_producer.stop = true;
    /* wake up everyone */
    pthread_cond_broadcast(&g_producer.req_cond);
    pthread_cond_broadcast(&g_producer.avail_cond);
    pthread_mutex_unlock(&g_producer.mutex);
}

/* producer yield: called by generator to give a new chunk, return true to stop */
static bool producer_yield(void *chunk, size_t sz)
{
    pthread_mutex_lock(&g_producer.mutex);
    /* wait until stop or request */
    while(true)
    {
        /* stop if requested */
        if(g_producer.stop)
        {
            pthread_mutex_unlock(&g_producer.mutex);
            return true;
        }
        /* if the chunk is empty, fill it */
        if(g_producer.chunk == NULL)
            break;
        /* otherwise wait for request */
        pthread_cond_wait(&g_producer.req_cond, &g_producer.mutex);
    }
    g_producer.chunk = malloc(sz);
    memcpy(g_producer.chunk, chunk, sz);
    g_producer.chunk_sz = sz;
    /* signal availability */
    pthread_cond_signal(&g_producer.avail_cond);
    pthread_mutex_unlock(&g_producer.mutex);
    return false;
}

static void producer_stop(void)
{
    pthread_mutex_lock(&g_producer.mutex);
    /* if we are not already stopping and there is a chunk still waiting to
     * be consumed, wait until next request */
    if(!g_producer.stop && g_producer.chunk)
        pthread_cond_wait(&g_producer.req_cond, &g_producer.mutex);
    /* set stop */
    g_producer.stop = true;
    /* wake up everyone */
    pthread_cond_broadcast(&g_producer.avail_cond);
    pthread_mutex_unlock(&g_producer.mutex);
}

/* Key search methods
 *
 * This code tries to find the key and signature of a device using an upgrade
 * file. It more or less relies on brute force and makes the following assumptions.
 * It assumes the key and the signature are hexadecimal strings (it appears to be
 * true thus far). The code lists all possible keys and decrypts the first
 * 8 bytes of the file. If the decrypted signature happens to be an hex string,
 * the code reports the key and signature as potentially valid. Note that some
 * key/sig pairs may not be valid but since the likelyhood of decrypting a
 * random 8-byte sequence using an hex string key and to produce an hex string
 * is very small, there should be almost no false positive.
 *
 * Since the key is supposedly random, the code starts by looking at "balanced"
 * keys: keys with slightly more digits (0-9) than letters (a-f) and then moving
 * towards very unbalanced strings (only digits or only letters).
 */

static struct
{
    pthread_mutex_t mutex;
    uint8_t *enc_buf;
    size_t enc_buf_sz;
    bool found_keysig;
    uint8_t key[NWZ_KEY_SIZE]; /* result */
    uint8_t sig[NWZ_SIG_SIZE]; /* result */
}g_keysig_search;

static bool is_hex[256];
static bool is_alnum[256];
static bool is_init = false;

static void keysig_search_init()
{
    if(is_init) return;
    is_init = true;
    memset(is_hex, 0, sizeof(is_hex));
    for(int i = '0'; i <= '9'; i++)
    {
        is_alnum[i] = true;
        is_hex[i] = true;
    }
    for(int i = 'a'; i <= 'f'; i++)
        is_hex[i] = true;
    for(int i = 'A'; i <= 'F'; i++)
        is_hex[i] = true;
    for(int i = 'a'; i <= 'z'; i++)
        is_alnum[i] = true;
    for(int i = 'A'; i <= 'Z'; i++)
        is_alnum[i] = true;
}

static bool hex_validate_sig(uint8_t *arr)
{
    for(int i = 0; i < 8; i++)
        if(!is_hex[arr[i]])
            return false;
    return true;
}

static bool alnum_validate_sig(uint8_t *arr)
{
    for(int i = 0; i < 8; i++)
        if(!is_alnum[arr[i]])
            return false;
    return true;
}

struct upg_header_t
{
    uint8_t sig[NWZ_SIG_SIZE];
    uint32_t nr_files;
    uint32_t pad; // make sure structure size is a multiple of 8
} __attribute__((packed));

typedef bool (*sig_validate_fn_t)(uint8_t *key);

static bool check_key(uint8_t key[NWZ_KEY_SIZE], sig_validate_fn_t validate)
{
    struct upg_header_t hdr;
    mg_decrypt_fw(g_keysig_search.enc_buf, sizeof(hdr.sig), (void *)&hdr, key);
    if(validate(hdr.sig))
    {
        /* the signature looks correct, so decrypt the header futher to be sure */
        mg_decrypt_fw(g_keysig_search.enc_buf, sizeof(hdr), (void *)&hdr, key);
        /* we expect the number of files to be small and the padding to be 0 */
        if(hdr.nr_files == 0 || hdr.nr_files > 10 || hdr.pad != 0)
            return false;
        cprintf(RED, "    Found key: %.8s (sig=%.8s, nr_files=%u)\n", key, hdr.sig, (unsigned)hdr.nr_files);
        pthread_mutex_lock(&g_keysig_search.mutex);
        g_keysig_search.found_keysig = true;
        memcpy(g_keysig_search.key, key, NWZ_KEY_SIZE);
        memcpy(g_keysig_search.sig, hdr.sig, NWZ_SIG_SIZE);
        pthread_mutex_unlock(&g_keysig_search.mutex);
        consumer_stop();
        return true;
    }
    return false;
}

/** Hex search */

struct hex_chunk_t
{
    uint8_t key[NWZ_KEY_SIZE]; /* partially pre-filled key */
    bool upper_case; /* allow upper case in letters */
    int pos;
    int rem_letters;
    int rem_digits;
};

static bool hex_rec(bool producer, struct hex_chunk_t *ch)
{
    /* we list the first 4 pos in generator, and remaining 4 in workers */
    if(producer && ch->pos == 4)
    {
        //printf("yield(%.8s,%d,%d,%d)\n", ch->key, ch->pos, ch->rem_digits, ch->rem_letters);
        return producer_yield(ch, sizeof(struct hex_chunk_t));
    }
    /* filled the key ? */
    if(!producer && ch->pos == NWZ_KEY_SIZE)
        return check_key(ch->key, hex_validate_sig);
    /* list next possibilities
     *
     * NOTE (42) Since the cipher is DES, the key is actually 56-bit: the least
     * significant bit of each byte is an (unused) parity bit. We thus only
     * generate keys where the least significant bit is 0. */
    int p = ch->pos++;
    if(ch->rem_digits > 0)
    {
        ch->rem_digits--;
        /* NOTE (42) */
        for(int i = '0'; i <= '9'; i += 2)
        {
            ch->key[p] = i;
            if(hex_rec(producer, ch))
                return true;
        }
        ch->rem_digits++;
    }
    if(ch->rem_letters > 0)
    {
        ch->rem_letters--;
        /* NOTE (42) */
        for(int i = 'a'; i <= 'f'; i += 2)
        {
            ch->key[p] = i;
            if(hex_rec(producer, ch))
                return true;
        }
        if(ch->upper_case)
        {
            for(int i = 'A'; i <= 'F'; i += 2)
            {
                ch->key[p] = i;
                if(hex_rec(producer, ch))
                    return true;
            }
        }
        ch->rem_letters++;
    }
    ch->pos--;
    return false;
}

static void *hex_worker(void *arg)
{
    (void) arg;
    while(true)
    {
        struct hex_chunk_t *ch = consumer_get(NULL);
        if(ch == NULL)
            break;
        hex_rec(false, ch);
    }
    return NULL;
}

static bool hex_producer_list(bool upper_case, int nr_digits, int nr_letters)
{
    struct hex_chunk_t ch;
    cprintf(BLUE, "    Listing keys with %d letters and %d digits\n", nr_letters,
        nr_digits);
    memset(ch.key, ' ', 8);
    ch.pos = 0;
    ch.upper_case = upper_case;
    ch.rem_letters = nr_letters;
    ch.rem_digits = nr_digits;
    return hex_rec(true, &ch);
}

void *hex_producer(void *arg)
{
    (void) arg;
    // sorted by probability:
    bool stop = hex_producer_list(false, 5, 3) // 5 digits, 3 letters: 0.281632
        || hex_producer_list(false, 6, 2) // 6 digits, 2 letters: 0.234693
        || hex_producer_list(false, 4, 4) // 4 digits, 4 letters: 0.211224
        || hex_producer_list(false, 7, 1) // 7 digits, 1 letters: 0.111759
        || hex_producer_list(false, 3, 5) // 3 digits, 5 letters: 0.101388
        || hex_producer_list(false, 2, 6) // 2 digits, 6 letters: 0.030416
        || hex_producer_list(false, 8, 0) // 8 digits, 0 letters: 0.023283
        || hex_producer_list(false, 1, 7) // 1 digits, 7 letters: 0.005214
        || hex_producer_list(false, 0, 8);// 0 digits, 8 letters: 0.000391
    if(!stop)
        producer_stop();
    return NULL;
}

void *hex_producer_up(void *arg)
{
    (void) arg;
    // sorted by probability:
    // TODO sort
    bool stop = hex_producer_list(true, 5, 3) // 5 digits, 3 letters: 0.281632
        || hex_producer_list(true, 6, 2) // 6 digits, 2 letters: 0.234693
        || hex_producer_list(true, 4, 4) // 4 digits, 4 letters: 0.211224
        || hex_producer_list(true, 7, 1) // 7 digits, 1 letters: 0.111759
        || hex_producer_list(true, 3, 5) // 3 digits, 5 letters: 0.101388
        || hex_producer_list(true, 2, 6) // 2 digits, 6 letters: 0.030416
        || hex_producer_list(true, 8, 0) // 8 digits, 0 letters: 0.023283
        || hex_producer_list(true, 1, 7) // 1 digits, 7 letters: 0.005214
        || hex_producer_list(true, 0, 8);// 0 digits, 8 letters: 0.000391
    if(!stop)
        producer_stop();
    return NULL;
}

/** Alphanumeric search */

struct alnum_chunk_t
{
    uint8_t key[NWZ_KEY_SIZE]; /* partially pre-filled key */
    int pos;
};

static bool alnum_rec(bool producer, struct alnum_chunk_t *ch)
{
    /* we list the first 5 pos in generator, and remaining 3 in workers */
    if(producer && ch->pos == 4)
    {
        printf("yield(%.8s,%d)\n", ch->key, ch->pos);
        return producer_yield(ch, sizeof(struct alnum_chunk_t));
    }
    /* filled the key ? */
    if(!producer && ch->pos == NWZ_KEY_SIZE)
        return check_key(ch->key, alnum_validate_sig);
    /* list next possibilities
     *
     * NOTE (42) Since the cipher is DES, the key is actually 56-bit: the least
     * significant bit of each byte is an (unused) parity bit. We thus only
     * generate keys where the least significant bit is 0. */
    int p = ch->pos++;
    /* NOTE (42) */
    for(int i = '0'; i <= '9'; i += 2)
    {
        ch->key[p] = i;
        if(alnum_rec(producer, ch))
            return true;
    }
    /* NOTE (42) */
    for(int i = 'a'; i <= 'z'; i += 2)
    {
        ch->key[p] = i;
        if(alnum_rec(producer, ch))
            return true;
    }
    ch->pos--;
    return false;
}

static void *alnum_worker(void *arg)
{
    (void) arg;
    while(true)
    {
        struct alnum_chunk_t *ch = consumer_get(NULL);
        if(ch == NULL)
            break;
        alnum_rec(false, ch);
    }
    return NULL;
}

void *alnum_producer(void *arg)
{
    (void) arg;
    struct alnum_chunk_t ch;
    cprintf(BLUE, "    Listing alphanumeric keys\n");
    memset(ch.key, ' ', 8);
    ch.pos = 0;
    if(!alnum_rec(true, &ch))
        producer_stop();
    return NULL;
}

typedef void *(*routine_t)(void *);

bool keysig_search(int method, uint8_t *enc_buf, size_t buf_sz,
    keysig_notify_fn_t notify, void *user, int nr_threads)
{
    /* init producer */
    producer_init();
    /* init search */
    keysig_search_init();
    pthread_mutex_init(&g_keysig_search.mutex, NULL);
    g_keysig_search.enc_buf = enc_buf;
    g_keysig_search.enc_buf_sz = buf_sz;
    g_keysig_search.found_keysig = false;
    /* get methods */
    routine_t worker_fn = NULL;
    routine_t producer_fn = NULL;
    if(method == KEYSIG_SEARCH_XDIGITS)
    {
        worker_fn = hex_worker;
        producer_fn = hex_producer;
    }
    else if(method == KEYSIG_SEARCH_XDIGITS_UP)
    {
        worker_fn = hex_worker;
        producer_fn = hex_producer_up;
    }
    else if(method == KEYSIG_SEARCH_ALNUM)
    {
        worker_fn = alnum_worker;
        producer_fn = alnum_producer;
    }
    else
    {
        printf("Invalid method\n");
        return false;
    }
    /* create workers */
    pthread_t *worker = malloc(sizeof(pthread_t) * nr_threads);
    pthread_t producer;
    for(int i = 0; i < nr_threads; i++)
        pthread_create(&worker[i], NULL, worker_fn, NULL);
    pthread_create(&producer, NULL, producer_fn, NULL);
    /* wait for all threads */
    pthread_join(producer, NULL);
    for(int i = 0; i < nr_threads; i++)
        pthread_join(worker[i], NULL);
    free(worker);
    if(g_keysig_search.found_keysig)
        notify(user, g_keysig_search.key, g_keysig_search.sig);
    return g_keysig_search.found_keysig;
}

struct keysig_search_desc_t keysig_search_desc[KEYSIG_SEARCH_LAST] =
{
    [KEYSIG_SEARCH_NONE] =
    {
        .name = "none",
        .comment = "don't use",
    },
    [KEYSIG_SEARCH_XDIGITS] =
    {
        .name = "xdigits",
        .comment = "Try to find an hexadecimal string keysig"
    },
    [KEYSIG_SEARCH_XDIGITS_UP] =
    {
        .name = "xdigits-up",
        .comment = "Try to find an hexadecimal string keysig, including upper case"
    },
    [KEYSIG_SEARCH_ALNUM] =
    {
        .name = "alnum",
        .comment = "Try to find an alphanumeric string keysig"
    },
};
