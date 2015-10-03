/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2015 by Amaury Pouly
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
#include "hwstub_internal.h" 

/**
 * URI parser
 */
#define DEFAULT_PORT    "8888"

void hwstub_usage_uri(FILE *f)
{
    fprintf(f, "The device URI mus be of the following format:\n");
    fprintf(f, "  scheme:[//domain[:port]][/][path[?query]]\n");
    fprintf(f, "The following schemes are recognized:\n");
    fprintf(f, "  usb          Local USB device\n");
    fprintf(f, "  tcp          HWStub TCP server\n");
    fprintf(f, "The following domains are recognized:\n");
    fprintf(f, "  domain       Any IP address and domain to resolve (TCP only)\n");
    fprintf(f, "The following paths are recognized:\n");
    fprintf(f, "  vid:pid      Vendor and product ID in hexadecimal (USB only)\n");
    fprintf(f, "  bus.addr     Bus number and device address (USB only)\n");
    fprintf(f, "  all          All devices\n");
    fprintf(f, "The optional query is a '&' separated list of attribute pairs:\n");
    fprintf(f, "  vid=<vid>    Vendor ID in hexadecimal (USB only)\n");
    fprintf(f, "  pid=<pid>    Product ID in hexadecimal (USB only)\n");
    fprintf(f, "  bus=<bus>    Bus number (USB only)\n");
    fprintf(f, "  addr=<addr>  Device address (USB only)\n");
    fprintf(f, "Examples of URI:\n");
    fprintf(f, "  tcp://localhost:8888\n");
    fprintf(f, "  usb:\n");
    fprintf(f, "  usb:066f:3780\n");
    fprintf(f, "  usb:3.42\n");
    fprintf(f, "  usb:all?vid=066f&bus=3\n");
}

static int parse_scheme_domain_port(FILE *f, char **uri, char **scheme, char **domain,
    char **port)
{
    /* extract scheme */
    *scheme = *uri;
    *uri = strchr(*uri, ':');
    if(*uri == NULL)
    {
        fprintf(f, "Invalid URI: missing ':'\n");
        return -1;
    }
    /* zero terminate scheme, skip ':' */
    *(*uri)++ = 0;
    /* check if there is domain */
    if(strncmp(*uri, "//", 2) != 0)
        return 0;
    /* skip '//' */
    (*uri) += 2;
    *domain = *uri;
    /* look for end of domain (:port or /path) */
    *uri = strpbrk(*uri, ":/");
    if(*uri && **uri == ':')
    {
        /* zero terminate domain, skip ':' */
        *(*uri)++ = 0;
        *port = *uri;
        *uri = strchr(*uri, '/');
    }
    /* zero terminate domain or port */
    if(*uri)
        *(*uri)++ = 0;
    return 0;
}

static int parse_path_query(FILE *f, char **uri, char **path, char **query)
{
    (void) f;
    *path = *uri;
    *query = NULL;
    /* check for end of path */
    *uri = strchr(*uri, '?');
    /* zero-terminate path if necessary and skip '?' */
    if(*uri)
        *(*uri)++ = 0;
    *query = *uri;
    return 0;
}

static int parse_query_attr(FILE *f, char **query, char **attr)
{
    (void) f;
    if(*query == NULL || **query == 0)
    {
        *attr = NULL;
        return 0;
    }
    *attr = *query;
    *query = strchr(*query, '&');
    if(*query)
        *(*query)++ = 0;
    return 0;
}

struct hwstub_device_list_entry_t
{
    enum { USB, TCP } type;
    libusb_device *usb_dev;
    const char *tcp_domain;
    const char *tcp_port;
};

static void free_dev(struct hwstub_device_list_entry_t *dev)
{
    if(dev->type == USB)
        libusb_unref_device(dev->usb_dev);
}

static void free_dev_list(struct hwstub_device_list_entry_t *list, ssize_t nr)
{
    for(int i = 0; i < nr; i++)
        free_dev(&list[i]);
    free(list);
}

static ssize_t get_usb_list(libusb_context *ctx, FILE *f, struct hwstub_device_list_entry_t **list)
{
    (void) f;
    libusb_device **ulist;
    ssize_t nr = hwstub_get_device_list(ctx, &ulist);
    if(nr < 0)
        return nr;
    *list = malloc(nr * sizeof(struct hwstub_device_list_entry_t));
    memset(*list, 0, nr * sizeof(struct hwstub_device_list_entry_t));
    for(int i = 0; i < nr; i++)
    {
        (*list)->type = USB;
        (*list)->usb_dev = libusb_ref_device(ulist[i]);
    }
    libusb_free_device_list(ulist, 1);
    return nr;
}

static ssize_t get_dev_list(libusb_context *ctx, FILE *f, const char *scheme,
    const char *domain, const char *port, struct hwstub_device_list_entry_t **list)
{
    if(strcmp(scheme, "usb") == 0)
    {
        if(domain == NULL && port == NULL)
            return get_usb_list(ctx, f, list);
        fprintf(f, "Invalid URI: usb scheme doesn't support domain\n");
        return -1;
    }
    else if(strcmp(scheme, "tcp") == 0)
    {
        if(domain == NULL)
        {
            fprintf(f, "Invalid URI: tcp scheme requires a domain\n");
            return -1;
        }
        if(port == NULL)
            port = DEFAULT_PORT;
        *list = malloc(sizeof(struct hwstub_device_list_entry_t));
        memset(*list, 0, sizeof(struct hwstub_device_list_entry_t));
        (*list)->type = TCP;
        (*list)->tcp_domain = domain;
        (*list)->tcp_port = port;
        return 1;
    }
    else
    {
        fprintf(f, "Invalid URI: unknown scheme '%s'\n", scheme);
        return -1;
    }
}

static void print_dev(FILE *f, struct hwstub_device_list_entry_t *dev)
{
    if(dev->type == USB)
    {
        struct libusb_device_descriptor dev_desc;
        memset(&dev_desc, 0, sizeof(dev_desc));
        char man[64];
        char prod[64];
        strcpy(man, "?");
        strcpy(prod, "?");
        libusb_device_handle *handle = NULL;
        libusb_get_device_descriptor(dev->usb_dev, &dev_desc);
        if(libusb_open(dev->usb_dev, &handle) == 0)
        {
            libusb_get_string_descriptor_ascii(handle, dev_desc.iManufacturer,
                (void *)man, sizeof(man));
            libusb_get_string_descriptor_ascii(handle, dev_desc.iProduct,
                (void *)prod, sizeof(prod));
            libusb_close(handle);
        }
        fprintf(f, "usb:%d.%d (%x:%x %s %s)", libusb_get_bus_number(dev->usb_dev),
            libusb_get_device_address(dev->usb_dev), dev_desc.idVendor,
            dev_desc.idProduct, man, prod);
    }
    else if(dev->type == TCP)
    {
        fprintf(f, "tcp://%s:%s", dev->tcp_domain, dev->tcp_port);
    }
    else
    {
        fprintf(f, "unk:???");
    }
}

static void print_dev_list(FILE *f, struct hwstub_device_list_entry_t *list, ssize_t nr)
{
    for(int i = 0; i < nr; i++)
    {
        fprintf(f, "  ");
        print_dev(f, &list[i]);
        fprintf(f, "\n");
    }
}

static void filter_dev_list(struct hwstub_device_list_entry_t *list, ssize_t *nr,
    int (*callback)(struct hwstub_device_list_entry_t *dev, void *user), void *user)
{
    for(int i = 0; i < *nr; i++)
    {
        if(callback(&list[i], user) == 0)
        {
            free_dev(&list[i]);
            memmove(&list[i], &list[*nr - 1], sizeof(struct hwstub_device_list_entry_t));
            (*nr)--;
            i--;
        }
    }
}

struct filter_t
{
    enum { VID = 1, PID = 2, BUS = 4, ADDR = 8 } mask;
    FILE *f;
    uint16_t vid;
    uint16_t pid;
    uint8_t bus;
    uint8_t addr;
};

static int apply_filter(struct hwstub_device_list_entry_t *dev, void *user)
{
#define _dbg(...) /*__VA_ARGS__*/
#define dbg(...) _dbg(fprintf(filt->f, __VA_ARGS__))
    struct filter_t *filt = user;
    dbg("filter ");
    _dbg(print_dev(filt->f, dev));
    struct libusb_device_descriptor dev_desc;
    memset(&dev_desc, 0, sizeof(dev_desc));
    if(dev->type == USB)
        libusb_get_device_descriptor(dev->usb_dev, &dev_desc);
    if(filt->mask & VID)
    {
        dbg(" with VID %x", filt->vid);
        if(dev->type != USB || dev_desc.idVendor != filt->vid)
            goto Lreject;
    }
    if(filt->mask & PID)
    {
        dbg(" with PID %x", filt->pid);
        if(dev->type != USB || dev_desc.idProduct != filt->pid)
            goto Lreject;
    }
    if(filt->mask & BUS)
    {
        dbg(" with BUS %u", filt->bus);
        if(dev->type != USB || libusb_get_bus_number(dev->usb_dev) != filt->bus)
            goto Lreject;
    }
    if(filt->mask & ADDR)
    {
        dbg(" with ADDR %u", filt->addr);
        if(dev->type != USB || libusb_get_device_address(dev->usb_dev) != filt->addr)
            goto Lreject;
    }
    dbg("\n");
    return 1;
Lreject:
    dbg(" reject\n");
    return 0;
#undef dbg
#undef _dbg
}

static int parse_path_filter(FILE *f, char *path, struct filter_t *filt)
{
    filt->mask = 0;
    if(strcmp(path, "") == 0 || strcmp(path, "all") == 0)
        return 0;
    char *sep = strpbrk(path, ":.");
    if(sep == NULL)
    {
        fprintf(f, "Invalid URI: invalid path '%s'\n", path);
        return -1;
    }
    if(*sep == ':')
    {
        *sep++ = 0;
        char *p;
        filt->mask = VID | PID;
        filt->vid = strtoul(path, &p, 16);
        if(*p)
        {
            fprintf(f, "Invalid URI: invalid VID in path '%s'\n", path);
            return -1;
        }
        filt->pid = strtoul(sep, &p, 16);
        if(*p)
        {
            fprintf(f, "Invalid URI: invalid PID in path '%s'\n", sep);
            return -1;
        }
    }
    else
    {
        *sep++ = 0;
        char *p;
        filt->mask = BUS | ADDR;
        filt->bus = strtoul(path, &p, 0);
        if(*p)
        {
            fprintf(f, "Invalid URI: invalid bus in path '%s'\n", path);
            return -1;
        }
        filt->addr = strtoul(sep, &p, 0);
        if(*p)
        {
            fprintf(f, "Invalid URI: invalid address in path '%s'\n", sep);
            return -1;
        }
    }
    return 0;
}

static int apply_path(FILE *f, char *path, struct hwstub_device_list_entry_t *list,
    ssize_t *nr)
{
    struct filter_t filt;
    if(parse_path_filter(f, path, &filt) != 0)
        return -1;
    filt.f = f;
    filter_dev_list(list, nr, apply_filter, &filt);
    return 0;
}

static int parse_query_filter(FILE *f, char *query, struct filter_t *filt)
{
    filt->mask = 0;
    char *sep = strchr(query, '=');
    if(sep == NULL)
    {
        fprintf(f, "Invalid URI: no '=' in query attribute '%s'\n", query);
        return -1;
    }
    *sep++ = 0;
    if(strcmp(query, "vid") == 0)
    {
        char *p;
        filt->mask |= VID;
        filt->vid = strtoul(sep, &p, 16);
        if(*p)
        {
            fprintf(f, "Invalid URI: invalid VID in query attribute '%s'\n", sep);
            return -1;
        }
    }
    else if(strcmp(query, "pid") == 0)
    {
        char *p;
        filt->mask |= PID;
        filt->pid = strtoul(sep, &p, 16);
        if(*p)
        {
            fprintf(f, "Invalid URI: invalid PID in query attribute '%s'\n", sep);
            return -1;
        }
    }
    else if(strcmp(query, "bus") == 0)
    {
        char *p;
        filt->mask |= BUS;
        filt->bus = strtoul(sep, &p, 0);
        if(*p)
        {
            fprintf(f, "Invalid URI: invalid bus in query attribute '%s'\n", sep);
            return -1;
        }
    }
    else if(strcmp(query, "addr") == 0)
    {
        char *p;
        filt->mask |= ADDR;
        filt->addr = strtoul(sep, &p, 0);
        if(*p)
        {
            fprintf(f, "Invalid URI: invalid address in query attribute '%s'\n", sep);
            return -1;
        }
    }
    else
    {
        fprintf(f, "Invalid URI: unknown query attribute '%s'\n", query);
        return -1;
    }
    return 0;
}

static int apply_query_attr(FILE *f, char *attr, struct hwstub_device_list_entry_t *list,
    ssize_t *nr)
{
    struct filter_t filt;
    if(parse_query_filter(f, attr, &filt) != 0)
        return -1;
    filt.f = f;
    filter_dev_list(list, nr, apply_filter, &filt);
    return 0;
}

struct hwstub_device_t *open_dev(struct hwstub_device_list_entry_t *dev)
{
    if(dev->type == USB)
        return hwstub_open_usb(dev->usb_dev);
    if(dev->type == TCP)
        return hwstub_open_tcp(dev->tcp_domain, dev->tcp_port);
    return NULL;
}

struct hwstub_device_t *hwstub_open_uri(libusb_context *ctx, FILE *f, const char *_uri)
{
    char *uri = strdup(_uri);
    char *scheme = NULL, *domain = NULL, *port = NULL;
    char *p = uri;
    if(parse_scheme_domain_port(f, &p, &scheme, &domain, &port) != 0)
        goto Lerr;
    struct hwstub_device_list_entry_t *list;
    ssize_t nr = get_dev_list(ctx, f, scheme, domain, port, &list);
    if(nr < 0)
        goto Lerr;
    char *path = NULL;
    char *query = NULL;
    if(parse_path_query(f, &p, &path, &query) != 0)
        goto Lerr2;
    if(apply_path(f, path, list, &nr))
        goto Lerr2;
    char *query_attr;
    if(parse_query_attr(f, &query, &query_attr) != 0)
        goto Lerr2;
    while(query_attr)
    {
        if(apply_query_attr(f, query_attr, list, &nr) != 0)
            goto Lerr2;
        if(parse_query_attr(f, &query, &query_attr) != 0)
            goto Lerr2;
    }
    if(nr == 0)
    {
        fprintf(f, "No device match for URI. Device list for scheme:\n");
        free_dev_list(list, nr);
        ssize_t nr = get_dev_list(ctx, f, scheme, domain, port, &list);
        print_dev_list(f, list, nr);
        goto Lerr2;
    }
    else if(nr > 1)
    {
        fprintf(f, "Several device match for URI. Match list:\n");
        print_dev_list(f, list, nr);
        goto Lerr2;
    }
    struct hwstub_device_t *dev = open_dev(list);
    free_dev_list(list, nr);
    return dev;
Lerr2:
    free_dev_list(list, nr);
Lerr:
    free(uri);
    return NULL;
#undef error
}
