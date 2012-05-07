#!/usr/bin/python
# -*- coding: utf8 -*-
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Copyright © 2010-2011 Rafaël Carré <rafael.carre@gmail>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.

from sys        import argv, stderr, stdout
from subprocess import Popen, PIPE
from os         import execv

args = argv[1:] # remove script path

for opt in ['-E', '-MM', '-v', '--version']:
    if opt in args:
        execv(args[0], args) # not actually compiling

if '-o' in args and args.index('-o') < len(args) - 1:
    if len(args[args.index('-o') + 1].rsplit('.o', 1)) == 1:
        execv(args[0], args) # output doesn't end in .o

args.append('-mthumb-interwork') # thumb-interwork is required
gcc = Popen(args + ['-mthumb'], stdout=PIPE, stderr=PIPE)
(out, err) = gcc.communicate()

if gcc.returncode != 0: # thumb failed, try outputting arm
    execv(args[0], args)

stdout.write(out.decode("utf-8"))
stderr.write(err.decode("utf-8"))
