#!/usr/bin/python3
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
import tempfile
import os
import shutil
import argparse

# extend search path for gitscraper
sys.path.append(os.path.abspath(os.path.join(
    os.path.dirname(os.path.realpath(__file__)), "../../utils/common")))
import gitscraper


LANGS = {
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


LANGBASE = "utils/rbutilqt"
# Paths and files to retrieve from svn.
# This is a mixed list, holding both paths and filenames.
# Get cpp sources as well for lupdate to work.
GITPATHS = [LANGBASE]


def main():
    parser = argparse.ArgumentParser(
        description='Print translation statistics for pasting in the wiki.')
    parser.add_argument('-p', '--pretty', action='store_true',
                        help='Display pretty output instead of wiki-style')
    parser.add_argument('-c', '--commit', nargs='?', help='Git commit hash')

    args = parser.parse_args()

    langstat(args.pretty, args.commit)


def langstat(pretty=True, tree=None):
    '''Get translation stats and print to stdout.'''
    # get gitpaths to temporary folder
    workfolder = tempfile.mkdtemp()
    repo = os.path.abspath(os.path.join(os.path.dirname(__file__), "../.."))
    if tree is None:
        tree = gitscraper.get_refs(repo)['HEAD']
    filesprops = gitscraper.scrape_files(
        repo, tree, GITPATHS, dest=workfolder,
        timestamp_files=[f"{LANGBASE}/lang"])

    projectfolder = os.path.join(workfolder, LANGBASE)
    # lupdate translations and drop all obsolete translations
    subprocess.Popen(["lupdate", "-no-obsolete", projectfolder, "-ts"]
                     + [f"lang/rbutil_{l}.ts" for l in LANGS],
                     stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                     cwd=projectfolder).communicate()
    # lrelease translations to get status
    output = subprocess.Popen(["lrelease"]
                              + [f"lang/rbutil_{l}.ts" for l in LANGS],
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              cwd=projectfolder).communicate()
    lines = output[0].decode().split("\n")

    re_updating = re.compile(r"^Updating.*")
    re_generated = re.compile(r"Generated.*")
    re_ignored = re.compile(r"Ignored.*")
    re_qmlang = re.compile(r"'.*/rbutil_(.*)\.qm'")
    re_qmbase = re.compile(r"'.*/(rbutil_.*)\.qm'")
    re_genout = re.compile(
        r"[^0-9]([0-9]+) .*[^0-9]([0-9]+) .*[^0-9]([0-9]+) ")
    re_ignout = re.compile(r"([0-9]+) ")

    # print header
    titlemax = 0
    for lang in LANGS:
        cur = len(LANGS[lang])
        if titlemax < cur:
            titlemax = cur

    if pretty:
        delim = "+--" + titlemax * "-"
        for spc in [7, 5, 5, 5, 5, 27, 17]:
            delim += "+" + "-" * spc
        delim += "+"
        head = ("| {:%s} | {:6}|{:5}|{:5}|{:5}|{:5}| {:26}| {:16}|"
                % titlemax).format("Language", "Code", "Trans", "Fin", "Unfin",
                                   "Untr", "Updated", "Done")
        print(delim)
        print(("| {:^%s} |" % (len(head) - 4)).format(tree))
        print(delim)
        print(head)
        print(delim)
    else:
        rev = f"{tree} {gitscraper.get_file_timestamp(repo, tree, '.')}"
        print(f"|  *Translation status as of revision {rev}*  ||||||||")
        print("| *Language* | *Language Code* | *Translations* "
              "| *Finished* | *Unfinished* | *Untranslated* | *Updated* "
              "| *Done* |")

    # scan output
    for i, line in enumerate(lines):
        if re_updating.search(line):
            lang = re_qmlang.findall(line)
            tsfile = f"{LANGBASE}/lang/{re_qmbase.findall(line)[0]}.ts"
            tsdate = filesprops[1][tsfile]

            line = lines[i + 1]
            if re_generated.search(line):
                values = re_genout.findall(line)
                translations = int(values[0][0])
                finished = int(values[0][1])
                unfinished = int(values[0][2])
                line = lines[i + 2]
                if not line.strip():
                    line = lines[i + 3]
                if re_ignored.search(line):
                    ignored = int(re_ignout.findall(line)[0])
                else:
                    ignored = 0
            if lang[0] in LANGS:
                name = LANGS[lang[0]].strip()
            else:
                name = '(unknown)'

            percent = (finished + unfinished) * 100. / (translations + ignored)
            progress = "#" * int(percent / 10)
            if (percent % 10) > 5:
                progress += "+"
            progress += " " * (10 - len(progress))
            if pretty:
                fancylang = lang[0] + " " * (5 - len(lang[0]))
            else:
                fancylang = lang[0]
            if pretty:
                print(f"| {name:{titlemax}} | {fancylang:5} | "
                      f"{translations:3} | {finished:3} | {unfinished:3} | "
                      f"{ignored:3} | {tsdate:25} | "
                      f"{int(percent):3}% {progress} |")
            else:
                if percent > 90:
                    color = r'%GREEN%'
                else:
                    if percent > 50:
                        color = r'%ORANGE%'
                    else:
                        color = r'%RED%'

                print(f"| {name} | {fancylang} | {translations} | {finished} "
                      f"| {unfinished} | {ignored} | {tsdate} | {color} "
                      f"{percent:.1f}% %ENDCOLOR% {progress} |")

    if pretty:
        print(delim)

    shutil.rmtree(workfolder)


if __name__ == "__main__":
    main()
