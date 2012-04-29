#!/usr/bin/python
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
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
import tempfile
import os
import shutil
from datetime import date
import time

# extend search path for gitscraper
sys.path.append(os.path.abspath(os.path.dirname(os.path.realpath(__file__))
        + "/../../utils/common"))
import gitscraper


langs = {
    'cs':       'Czech',
    'de':       'German',
    'fi':       'Finnish',
    'fr':       'French',
    'gr':       'Greek',
    'he':       'Hebrew',
    'it':       'Italian',
    'ja':       'Japanese',
    'nl':       'Dutch',
    'pl':       'Polish',
    'pt':       'Portuguese',
    'pt_BR':    'Portuguese (Brasileiro)',
    'ru':       'Russian',
    'tr':       'Turkish',
    'zh_CN':    'Chinese',
    'zh_TW':    'Chinese (trad)'
}


langbase = "rbutil/rbutilqt/"
# Paths and files to retrieve from svn.
# This is a mixed list, holding both paths and filenames.
# Get cpp sources as well for lupdate to work.
gitpaths = [langbase]


def printhelp():
    print "Usage:", sys.argv[0], "[options]"
    print "Print translation statistics suitable for pasting in the wiki."
    print "Options:"
    print "    --pretty: display pretty output instead of wiki-style"
    print "    --help: show this help"


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

    # get gitpaths to temporary folder
    workfolder = tempfile.mkdtemp() + "/"
    repo = os.path.abspath(os.path.join(os.path.dirname(__file__), "../.."))
    tree = gitscraper.get_refs(repo)['refs/remotes/origin/master']
    filesprops = gitscraper.scrape_files(repo, tree, gitpaths, dest=workfolder,
            timestamp_files=["rbutil/rbutilqt/lang"])

    projectfolder = workfolder + langbase
    # lupdate translations and drop all obsolete translations
    subprocess.Popen(["lupdate-qt4", "-no-obsolete", "rbutilqt.pro"],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            cwd=projectfolder).communicate()
    # lrelease translations to get status
    output = subprocess.Popen(["lrelease-qt4", "rbutilqt.pro"],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            cwd=projectfolder).communicate()
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
        spaces = [7, 5, 5, 5, 5, 27, 17]
        delim = "+--" + titlemax * "-"
        for s in spaces:
            delim += "+" + "-" * s
        delim += "+"
        head = "| Language" + (titlemax - 8) * " " \
                + " |  Code |Trans| Fin |Unfin| Untr|           Updated         |       Done      |"
        print delim
        print "|" + " " * ((len(head) / 2 - len(tree) / 2) - 1) + str(tree) \
                + " " * ((len(head) / 2 - len(tree) / 2) - 1) + "|"
        print delim
        print head
        print delim
    else:
        r = str(tree) + " (" + gitscraper.get_file_timestamp(repo, tree, ".") + ")"
        print "|  *Translation status as of revision " + r + "*  ||||||||"
        print "| *Language* | *Language Code* | *Translations* | *Finished* | " \
        "*Unfinished* | *Untranslated* | *Updated* | *Done* |"

    # scan output
    i = 0
    while i < len(lines):
        line = lines[i]
        if re_updating.search(line):
            lang = re_qmlang.findall(line)
            tsfile = "rbutil/rbutilqt/lang/" + re_qmbase.findall(line)[0] + ".ts"
            tsdate = filesprops[1][tsfile]

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
            if lang[0] in langs:
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
            status = [fancylang, translations, finished, unfinished, ignored, tsdate, percent, bar]
            if pretty == 1:
                thisname = name + (titlemax - len(name)) * " "
                print "| " + thisname + " | %5s | %3s | %3s | %3s | %3s | %25s | %3i%% %s |" % tuple(status)
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
