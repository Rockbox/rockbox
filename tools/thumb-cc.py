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

import sys
import os
import subprocess
import tempfile


def run_gcc(args):
    os.execv(args[0], args) # run real gcc

def get_output(args):
    output = False
    for i in args:
        if output == True:
            return i
        elif i == '-o':
            output = True


def try_thumb(args, output):
    thumb_args = args + ['-mthumb']
    thumb_args[thumb_args.index('-o') + 1] = output
    thumb_gcc = subprocess.Popen(thumb_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout, stderr) = thumb_gcc.communicate()

    if thumb_gcc.returncode != 0: # building failed
        return False

    # building with thumb succeeded, show our output
    #sys.stderr.write(bytes.decode(stderr))
    #sys.stdout.write(bytes.decode(stdout))
    sys.stderr.write(stderr)
    sys.stdout.write(stdout)
    return True

##### main


args=sys.argv[1:]   # remove script path

for opt in ['-E', '-MM', '-v', '--version']:
    if opt in args:
        run_gcc(args)

output = get_output(args)
split = output.rsplit('.o', 1)

if len(split) == 1: # output doesn't end in .o
    run_gcc(args)

dirname = os.path.dirname(output)
thumb_output = tempfile.mktemp(suffix='.o', prefix=split[0], dir=dirname)

args.append('-mthumb-interwork')

if try_thumb(args, thumb_output):
    os.rename(thumb_output, output)
    sys.exit(0)
else:
    #sys.stderr.write('skipped ' + os.path.basename(output) + '\n')
    run_gcc(args)
