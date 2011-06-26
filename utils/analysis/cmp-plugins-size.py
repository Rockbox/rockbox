#!/usr/bin/python
# -*- coding: utf8 -*-
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Copyright © 2010 Rafaël Carré <rafael.carre@gmail>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#

# TODO: iram

import sys
import os
import re
import stat
import fnmatch


def percent_diff(old, new):
    if old == 0:
        return '?'
    diff = 100.0*(new-old)/old
    return format(diff, '+2.2f') + '%'


def find_map(dir):
    dirs = []
    for file in os.listdir(dir):
        path = os.path.join(dir, file)
        if stat.S_ISDIR(os.stat(path).st_mode) != 0:
            dirs += find_map(path)
        elif fnmatch.fnmatch(file, '*.map'):
            dirs += [path]
    return dirs


def rb_version(dir):
    info = os.path.join(dir, 'rockbox-info.txt')
    if not os.path.lexists(info):
        return 'unknown'
    info = open(info).read()
    s = re.search('^Version: .*', info, re.MULTILINE)
    if not s:
        return 'unknown'
    return re.sub('^Version: ', '', info[s.start():s.end()])


def map_info(map):
    file = os.path.basename(map)
    name = file.rsplit('.map', 1)[0]

    # ignore ape-pre map, used to fill IRAM
    if name == 'ape-pre':
        return None

    # ignore overlays
    ovlmap = os.path.join(os.path.dirname(map), name, file)
    if os.path.lexists(ovlmap):
        return None

    f = open(map).read() # read map content

    s = re.search('^PLUGIN_RAM *0x(\d|[abcdef])*', f, re.MULTILINE)
    if not s: return (name, 0)
    plugin_start = re.sub('^PLUGIN_RAM *0x0*', '', f[s.start():s.end()])

    s = re.search('^\.pluginend *0x(\d|[abcdef])*', f, re.MULTILINE)
    if not s: return (name, 0)
    plugin_end   = re.sub('^\.pluginend *0x0*', '', f[s.start():s.end()])

    size = int(plugin_end, 16) - int(plugin_start, 16)
    return (name, size)


def get_new(oldinfo, newinfo, name):
    i = 0
    while i < len(oldinfo) and i < len(newinfo):
        if newinfo[i][0] == name:
            return newinfo[i]
        i += 1
    return None


def compare(olddir, newdir, oldver, newer):
    oldinfo = []
    for map in find_map(olddir):
        info = map_info(map)
        if info:
            oldinfo += [info]

    newinfo = []
    for map in find_map(newdir):
        info = map_info(map)
        if info:
            newinfo += [info]

    oldinfo.sort()
    newinfo.sort()

    diff = []
    longest_name = 0
    for (name, old_size) in oldinfo:
        new = get_new(oldinfo, newinfo, name)
        if not new:
            continue
        (name, new_size) = new
        if len(name) > longest_name:
            longest_name = len(name)
        diff += [(name, new_size - old_size, old_size)]

    spacelen = (longest_name + 3)

    print(' ' * spacelen + oldver + '\t\t' + newver + '\n')

    for (name, diff, old_size) in diff:
        space = ' ' * (longest_name - len(name) + 3)
        new_size = old_size + diff
        pdiff = percent_diff(old_size, new_size)
        diff = str(diff)
        if diff[0] != '-':
            diff = '+' + diff

        print(name + space + str(old_size) + '\t' + diff + \
            '\t=\t' + str(new_size) + '\t-->\t' + pdiff)




### main


if len(sys.argv) != 3:
    print('Usage: ' + sys.argv[0] + ' build-old build-new')
    sys.exit(1)

oldver = rb_version(sys.argv[1])
newver = rb_version(sys.argv[2])

oldplugindir = sys.argv[1] + '/apps/plugins'
newplugindir = sys.argv[2] + '/apps/plugins'
oldcodecsdir = sys.argv[1] + '/lib/rbcodec/codecs'
newcodecsdir = sys.argv[2] + '/lib/rbcodec/codecs'

if os.path.lexists(oldplugindir) and os.path.lexists(newplugindir):
    compare(oldplugindir, newplugindir, oldver, newver)

print('\n\n\n')

if os.path.lexists(oldcodecsdir) and os.path.lexists(newcodecsdir):
    compare(oldcodecsdir, newcodecsdir, oldver, newver)
