#!/usr/bin/python3
import glob
import os
import re
import subprocess

# parse models.txt
g_models = []
with open('models.txt') as fp:
    for line in fp:
        # we unpack and repack 1) to make the format obvious 2) to catch errors
        mid,name = line.rstrip().split(",")
        g_models.append({'mid': int(mid, 0), 'name': name})
# parse series.txt
g_series = []
g_series_codename = set()
with open('series.txt') as fp:
    for line in fp:
        # we unpack and repack 1) to make the format obvious 2) to catch errors
        arr = line.rstrip().split(",")
        codename = arr[0]
        name = arr[1]
        models = arr[2:]
        # handle empty list
        if len(models) == 1 and models[0] == "":
            models = []
        models = [int(mid,0) for mid in models]
        g_series.append({'codename': codename, 'name': name, 'models': models})
        g_series_codename.add(codename)
# parse all maps in nvp/
# since most nvps are the same, what we actually do is to compute the md5sum hash
# of all files, to identify groups and then each entry in the name is in fact the
# hash, and we only parse one file per hash group
g_hash_nvp = dict() # hash -> nvp
g_nvp_hash = dict() # codename -> hash
HASH_SIZE=6
map_files = glob.glob('nvp/nw*.txt')
for line in subprocess.run(["md5sum"] + map_files, stdout = subprocess.PIPE).stdout.decode("utf-8").split("\n"):
    if len(line.rstrip()) == 0:
        continue
    hash, file = line.rstrip().split()
    codename = re.search('nvp/(.*)\.txt', file).group(1)
    # sanity check
    if not (codename in g_series_codename):
        print("Warning: file %s does not have a match series in series.txt" % file)
    hash = hash[:HASH_SIZE]
    # only keep one file
    if not (hash in g_hash_nvp):
        g_hash_nvp[hash] = set()
    g_hash_nvp[hash].add(codename);
    g_nvp_hash[codename] = hash
# we have some file nodes (nodes-*) but not necessarily for all series
# so for each hash group, try to find at least one
for hash in g_hash_nvp:
    # look at all codename and see if we can find one with a node file
    node_codename = ""
    for codename in g_hash_nvp[hash]:
        if os.path.isfile("nvp/nodes-%s.txt" % codename):
            node_codename = codename
            break
    # if we didn't find one, we just keep the first one
    # otherwise keep the one we found
    if node_codename == "":
        node_codename = g_hash_nvp[hash].pop()
    g_hash_nvp[hash] = node_codename
# for each entry in g_hash_nvp, replace the file name by the actual table
# that we parse, and compute all nvp names at the same time
g_nvp_names = set() # set of all nvp names
g_nvp_desc = dict() # name -> set of all description of a node
g_nvp_size = dict() # name -> set of all possible sizes of a node
for hash in g_hash_nvp:
    codename = g_hash_nvp[hash]
    # extract codename from file
    # parse file
    map = dict()
    with open("nvp/%s.txt" % codename) as fp:
        for line in fp:
            # we unpack and repack 1) to make the format obvious 2) to catch errors
            name,index = line.rstrip().split(",")
            # convert node to integer but be careful of leading 0 (ie 010 is actually
            # 10 in decimal, it is not in octal)
            index = int(index, 10)
            map[index] = name
            g_nvp_names.add(name)
    # parse node map if any
    node_map = dict()
    if os.path.isfile("nvp/nodes-%s.txt" % codename):
        with open("nvp/nodes-%s.txt" % codename) as fp:
            for line in fp:
                # we unpack and repack 1) to make the format obvious 2) to catch errors
                index,size,desc = line.rstrip().split(",")
                # convert node to integer but be careful of leading 0 (ie 010 is actually
                # 10 in decimal, it is not in octal)
                index = int(index, 10)
                desc = desc.rstrip()
                node_map[index] = {'size': size, 'desc': desc}
    # compute final nvp
    nvp = dict()
    for index in map:
        size = 0
        desc = ""
        name = map[index]
        if index in node_map:
            size = node_map[index]["size"]
            desc = node_map[index]["desc"]
        nvp[name] = index
        if not (name in g_nvp_desc):
            g_nvp_desc[name] = set()
        if len(desc) != 0:
            g_nvp_desc[name].add(desc)
        if not (name in g_nvp_size):
            g_nvp_size[name] = set()
        if size != 0:
            g_nvp_size[name].add(size)
    g_hash_nvp[hash] = nvp

#
# generate header
#
header_begin = \
"""\
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \\
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
"""

header_end = \
"""\
    NWZ_NVP_COUNT /* Number of nvp nodes */
};

/* Invalid NVP index */
#define NWZ_NVP_INVALID     -1 /* Non-existent entry */
/* Number of models */
#define NWZ_MODEL_COUNT     %s
/* Number of series */
#define NWZ_SERIES_COUNT    %s

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
"""

with open("nwz_db.h", "w") as fp:
    fp.write(header_begin)
    # generate list of all nvp nodes
    for name in sorted(g_nvp_names):
        # create comment to explain the meaning, gather several meaning together
        # if there are more than one (sorted to keep a stable order when we update)
        explain = ""
        if name in g_nvp_desc:
            explain = " | ".join(sorted(list(g_nvp_desc[name])))
        # overwrite desc set with a single string for later
        g_nvp_desc[name] = explain
        fp.write("    NWZ_NVP_%s, /* %s */\n" % (name.upper(), explain))
    fp.write(header_end % (len(g_models), len(g_series)))

#
# generate tables
#
impl_begin = \
"""\
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \\
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

/** /!\ This file was automatically generated, DO NOT MODIFY IT DIRECTLY /!\ */

#include "nwz_db.h"

struct nwz_model_info_t nwz_model[NWZ_MODEL_COUNT] =
{
"""

def by_mid(model):
    return model["mid"]

def by_name(nvp_entry):
    return nvp_entry["name"]

def codename_to_c(codename):
    return re.sub('[^a-zA-Z0-9]', '_', codename, 0)

with open("nwz_db.c", "w") as fp:
    fp.write(impl_begin)
    # generate model list (sort by mid)
    for model in sorted(g_models, key = by_mid):
        fp.write("    { %s, \"%s\" },\n" % (hex(model["mid"]), model["name"]))
    fp.write("};\n")
    # generate nvps
    for hash in sorted(g_hash_nvp):
        nvp = g_hash_nvp[hash]
        fp.write("\nstatic int nvp_index_%s[NWZ_NVP_COUNT] =\n" % hash)
        fp.write("{\n")
        for name in sorted(g_nvp_names):
            index = "NWZ_NVP_INVALID"
            if name in nvp:
                index = nvp[name]
            fp.write("    [NWZ_NVP_%s] = %s,\n" % (name.upper(), index))
        fp.write("};\n")
    # generate nvp info
    fp.write("\nstruct nwz_nvp_info_t nwz_nvp[NWZ_NVP_COUNT] =\n")
    fp.write("{\n")
    for name in sorted(g_nvp_names):
        size = 0
        if name in g_nvp_size:
            size_set = g_nvp_size[name]
            if len(size_set) == 0:
                size = 0
            elif len(size_set) == 1:
                size = next(iter(size_set))
            else:
                print("Warning: nvp node \"%s\" has several possible sizes: %s"
                    % (name, size_set))
                size = 0
        desc = ""
        if name in g_nvp_desc:
            desc = g_nvp_desc[name]
        fp.write("    [NWZ_NVP_%s] = { \"%s\", %s, \"%s\" },\n" % (name.upper(),
            name, size, desc))
    fp.write("};\n")
    # generate list of models for each series
    for series in g_series:
        c_codename = codename_to_c(series["codename"])
        list = [hex(mid) for mid in series["models"]]
        limit = 3
        c_list = ""
        while len(list) != 0:
            if len(list) <= limit:
                c_list = c_list + ", ".join(list)
                list = []
            else:
                c_list = c_list + ", ".join(list[:limit]) + ",\n    "
                list = list[limit:]
                limit = 6
        fp.write("\nstatic unsigned long models_%s[] = { %s };\n" % (c_codename, c_list))
    # generate series list
    fp.write("\nstruct nwz_series_info_t nwz_series[NWZ_SERIES_COUNT] =\n{\n")
    for series in g_series:
        name = series["name"]
        codename = series["codename"]
        c_codename = codename_to_c(codename)
        nvp = "0"
        if codename in g_nvp_hash:
            nvp = "&nvp_index_%s" % g_nvp_hash[codename]
        fp.write("    { \"%s\", \"%s\", %s, models_%s, %s },\n" % (codename,
            name, len(series["models"]), c_codename, nvp))
    fp.write("};\n")
