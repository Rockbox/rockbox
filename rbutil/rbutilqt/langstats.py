#!/usr/bin/python
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
# Copyright (c) 2010 Dominik Riebeling
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
#
# lrelease all rbutil translations and create a nice table from the output
# suited to paste in the wiki.
#

import subprocess
import re
import sys
import string
import pysvn
import tempfile
import os
import shutil
from datetime import date


langs = {
    'cs'        : 'Czech',
    'de'        : 'German',
    'fi'        : 'Finnish',
    'fr'        : 'French',
    'gr'        : 'Greek',
    'he'        : 'Hebrew',
    'ja'        : 'Japanese',
    'nl'        : 'Dutch',
    'pl'        : 'Polish',
    'pt'        : 'Portuguese',
    'pt_BR'     : 'Portuguese (Brasileiro)',
    'ru'        : 'Russian',
    'tr'        : 'Turkish',
    'zh_CN'     : 'Chinese',
    'zh_TW'     : 'Chinese (trad)'
}

# modules that are not part of python itself.
try:
    import pysvn
except ImportError:
    print "Fatal: This script requires the pysvn package to run."
    print "       See http://pysvn.tigris.org/."
    sys.exit(-5)


svnserver = "svn://svn.rockbox.org/rockbox/trunk/"
langbase  = "rbutil/rbutilqt/"
# Paths and files to retrieve from svn.
# This is a mixed list, holding both paths and filenames.
# Get cpp sources as well for lupdate to work.
svnpaths = [ langbase ]

def printhelp():
    print "Usage:", sys.argv[0], "[options]"
    print "Print translation statistics suitable for pasting in the wiki."
    print "Options:"
    print "    --pretty: display pretty output instead of wiki-style"
    print "    --help: show this help"


def gettrunkrev(svnsrv):
    '''Get the revision of trunk for svnsrv'''
    client = pysvn.Client()
    entries = client.info2(svnsrv, recurse=False)
    return entries[0][1].rev.number


def getsources(svnsrv, filelist, dest):
    '''Get the files listed in filelist from svnsrv and put it at dest.'''
    client = pysvn.Client()
    print "Checking out sources from %s, please wait." % svnsrv

    for elem in filelist:
        url = re.subn('/$', '', svnsrv + elem)[0]
        destpath = re.subn('/$', '', dest + elem)[0]
        # make sure the destination path does exist
        d = os.path.dirname(destpath)
        if not os.path.exists(d):
            os.makedirs(d)
        # get from svn
        try:
            client.export(url, destpath)
        except:
            print "SVN client error: %s" % sys.exc_value
            print "URL: %s, destination: %s" % (url, destpath)
            return -1
    print "Checkout finished."
    return 0


def main():
    if len(sys.argv) > 1:
        if sys.argv[1] == '--help':
            printhelp()
            sys.exit(0)
    if len(sys.argv) > 1:
        if sys.argv[1] == '--pretty':
            pretty = 1
    else:
        pretty = 0

    # get svnpaths to temporary folder
    workfolder = tempfile.mkdtemp() + "/"
    getsources(svnserver, svnpaths, workfolder)

    projectfolder = workfolder + langbase
    # lupdate translations and drop all obsolete translations
    subprocess.Popen(["lupdate-qt4", "-no-obsolete", "rbutilqt.pro"], \
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=projectfolder).communicate()
    # lrelease translations to get status
    output = subprocess.Popen(["lrelease-qt4", "rbutilqt.pro"], stdout=subprocess.PIPE, \
            stderr=subprocess.PIPE, cwd=projectfolder).communicate()
    lines = re.split(r"\n", output[0])

    re_updating = re.compile(r"^Updating.*")
    re_generated = re.compile(r"Generated.*")
    re_ignored = re.compile(r"Ignored.*")
    re_qmlang = re.compile(r"'.*/rbutil_(.*)\.qm'")
    re_qmbase = re.compile(r"'.*/(rbutil_.*)\.qm'")
    re_genout = re.compile(r"[^0-9]([0-9]+) .*[^0-9]([0-9]+) .*[^0-9]([0-9]+) ")
    re_ignout = re.compile(r"([0-9]+) ")

    # print header
    titlemax = 0
    for l in langs:
        cur = len(langs[l])
        if titlemax < cur:
            titlemax = cur

    if pretty == 1:
        delim = "+-" + titlemax * "-" \
                + "-+-------+-----+-----+-----+-----+--------------------+-----------------+"
        head = "| Language" + (titlemax - 8) * " " \
                + " |  Code |Trans| Fin |Unfin| Untr|       Updated      |       Done      |"
        print delim
        print "|" + " " * (len(head) / 2 - 3) + str(gettrunkrev(svnserver)) \
                + " " * (len(head) / 2 - 4) + "|"
        print delim
        print head
        print delim
    else:
        print "|  *Translation status as of revision " + str(gettrunkrev(svnserver)) + "*  ||||||||"
        print "| *Language* | *Language Code* | *Translations* | *Finished* | " \
        "*Unfinished* | *Untranslated* | *Updated* | *Done* |"

    client = pysvn.Client()
    # scan output
    i = 0
    while i < len(lines):
        line = lines[i]
        if re_updating.search(line):
            lang = re_qmlang.findall(line)
            tsfile = "lang/" + re_qmbase.findall(line)[0] + ".ts"
            fileinfo = client.info2(svnserver + langbase + tsfile)[0][1]
            tsrev = fileinfo.last_changed_rev.number
            tsdate = date.fromtimestamp(fileinfo.last_changed_date).isoformat()

            line = lines[i + 1]
            if re_generated.search(line):
                values = re_genout.findall(line)
                translations = string.atoi(values[0][0])
                finished = string.atoi(values[0][1])
                unfinished = string.atoi(values[0][2])
                line = lines[i + 2]
                if not line.strip():
                    line = lines[i + 3]
                if re_ignored.search(line):
                    ignored = string.atoi(re_ignout.findall(line)[0])
                else:
                    ignored = 0
            if langs.has_key(lang[0]):
                name = langs[lang[0]].strip()
            else:
                name = '(unknown)'

            percent = (float(finished + unfinished) * 100 / float(translations + ignored))
            bar = "#" * int(percent / 10)
            if (percent % 10) > 5:
                bar += "+"
            bar += " " * (10 - len(bar))
            if pretty == 1:
                fancylang = lang[0] + " " * (5 - len(lang[0]))
            else:
                fancylang = lang[0]
            tsversion = str(tsrev) + " (" + tsdate + ")"
            status = [fancylang, translations, finished, unfinished, ignored, tsversion, percent, bar]
            if pretty == 1:
                thisname = name + (titlemax - len(name)) * " "
                print "| " + thisname + " | %5s | %3s | %3s | %3s | %3s | %6s | %3i%% %s |" % tuple(status)
            else:
                if percent > 90:
                    color = '%%GREEN%%'
                else:
                    if percent > 50:
                        color = '%%ORANGE%%'
                    else:
                        color = '%%RED%%'

                text = "| " + name + " | %s | %s | %s | %s | %s | %s | " + color + "%3i%%%%ENDCOLOR%% %s |"
                print text % tuple(status)
        i += 1

    if pretty == 1:
        print delim

    shutil.rmtree(workfolder)


if __name__ == "__main__":
    main()

