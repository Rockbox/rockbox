#!/usr/bin/python
# -*- coding: utf8 -*-
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Copyright © 2010 Daniel Dalton <daniel.dalton10@gmail.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#

import os
import sys
# The following lines provide variables which you can modify to adjust
# settings... See comments next to each line.
# Start opts:
espeak = '/usr/bin/espeak' # location of easpeak binary
rbspeexenc = './rbspeexenc' # path to rbspeexenc binary (default currentdir)
VOPTS=espeak+" -s 320 -z" # Your espeak opts
ROPTS=rbspeexenc+" -q 4 -c 10" # rbspeex opts
logfile="/tmp/talkclips.log" # a file where output should be logged
# End opts
# Don't touch the below settings. Unless you know what your doing.
log=open(logfile, 'w') # logging leave this var alone.
USAGE="Usage: %s <directory>" % (sys.argv[0]) # usage prompt don't touch
if not os.path.exists(rbspeexenc):
  print ("%s not found, please change your rbspeexenc path appropriately,\n"\
      "or place the binary in %s\n"\
      % (rbspeexenc, os.path.realpath(rbspeexenc)))
  print (USAGE)
  exit (-1) # Rbspeexenc not found
if not os.path.exists(espeak):
  print ("Espeak not found, please install espeak, or adjust the path of\n"\
      'the "espeak" variable appropriately.\n')
  print (USAGE)
  exit (-1) # espeak not found

if len(sys.argv) != 2:
  print (USAGE)
  exit (-1) # user failed to supply enough arguments

RBDIR=sys.argv[1] # grab user input on the command line (don't touch)
if not os.path.exists(sys.argv[1]):
  print ("The path %s doesn't exist, please try again.\n\n%s"\
      % (sys.argv[1], USAGE))
  exit(-1) # path doesn't exist
else: # check if it's a dir
  if not os.path.isdir(sys.argv[1]): # a file
    print ("This script only currently works for directories.\n\n%s" % (USAGE))
    exit (-1) # not a dir

def gentalkclip(clipname, fullpath, isdir):
  """Generate an individual talk clip.

  Based on the file name structure of talk clips, run them through the
  synth, and encoder, and save accordingly."""

  if isdir: # directory
    output=os.path.join(fullpath, "_dirname.talk") # dir clip name
    if os.path.exists(output):
      return True # no need to create again
    try: # Don't let the script stop if bash raises filename errors
      os.system('%s "%s" -w "%s"' % (VOPTS, clipname, output+".tmp"))
      os.system('%s "%s" "%s"' % (ROPTS, output+".tmp", output))
      os.remove(output+".tmp") # delete the old wav file
    except OSError:
      log.write('Failed to create clip for directory: "%s"\n' % (clipname))
      return False
    log.write( 'Created clip for directory: "%s"\n' % (clipname)) # log
    return True
  else: # file
    output=fullpath+".talk"
    if os.path.exists(output):
      return True # no need to create again
    try:
      os.system('%s "%s" -w "%s"' % (VOPTS, clipname, output+".tmp"))
      os.system('%s "%s" "%s"' % (ROPTS, output+".tmp", output))
      os.remove (output+".tmp")
    except OSError: # don't let bash errors stop us
      log.write('Failed to create clip for file: "%s"\n' % (clipname))
      return False # something failed, so continue with next file
    log.write('Created clip for file: "%s"\n' % (clipname)) # logging
  return True # clips created

def istalkclip(file):
  """Is file a talkclip?

  Returns True if file is a .talk clip for rockbox, otherwise returns
  False."""

  if '_dirname.talk' in file or '.talk' in file:
    return True # is talk clip
  else: # Not a talk clip
    return False

def walker(directory):
  """Walk through a directory.

  Walk through a directory and subdirs, and operate on it, passing files
  through to the correct functions to generate talk clips."""
  for item in os.listdir(directory): # get subdirs and files
    if os.path.isdir(os.path.join(directory, item)):
      gentalkclip (item, os.path.join(directory, item), True) # its a dir
      walker(os.path.join(directory, item)) # go down into this sub dir
      continue
    else: # a file
      if istalkclip (item):
        continue # is a talk clip
      else: # create clip
        gentalkclip(item, os.path.join(directory, item), False) # a file
        continue

walker(RBDIR) # start the program:)
log.close() # close the log and finish
