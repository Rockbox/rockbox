#!/usr/bin/python
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Copyright (c) 2012 Dominik Riebeling
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#

'''Scrape files from a git repository.

This module provides functions to get a subset of files from a git repository.
The files to retrieve can be specified, and the git tree to work on can be
specified. That way arbitrary trees can be retrieved (like a subset of files
for a given tag).

Retrieved files can be packaged into a bzip2 compressed tarball or stored in a
given folder for processing afterwards.

Calls git commands directly for maximum compatibility.
'''

import re
import subprocess
import os
import tarfile
import tempfile
import shutil


def get_refs(repo):
    '''Get dict matching refs to hashes from repository pointed to by repo.
    @param repo Path to repository root.
    @return Dict matching hashes to each ref.
    '''
    print("Getting list of refs")
    output = subprocess.Popen(["git", "show-ref", "--abbrev"],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=repo)
    cmdout = output.communicate()
    refs = {}

    if len(cmdout[1]) > 0:
        print("An error occured!\n")
        print(cmdout[1])
        return refs

    for line in cmdout:
        regex = re.findall(b'([a-f0-9]+)\s+(\S+)', line)
        for r in regex:
            # ref is the key, hash its value.
            refs[r[1].decode()] = r[0].decode()

    return refs


def get_lstree(repo, start, filterlist=[]):
    '''Get recursive list of tree objects for a given tree.
    @param repo Path to repository root.
    @param start Hash identifying the tree.
    @param filterlist List of paths to retrieve objecs hashes for.
                      An empty list will retrieve all paths.
    @return Dict mapping filename to blob hash
    '''
    output = subprocess.Popen(["git", "ls-tree", "-r", start],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=repo)
    cmdout = output.communicate()
    objects = {}

    if len(cmdout[1]) > 0:
        print("An error occured!\n")
        print(cmdout[1])
        return objects

    for line in cmdout[0].decode().split('\n'):
        regex = re.findall(b'([0-9]+)\s+([a-z]+)\s+([0-9a-f]+)\s+(\S+)',
                line.encode())
        for rf in regex:
            # filter
            add = False
            for f in filterlist:
                if rf[3].decode().find(f) == 0:
                    add = True

            # If two files have the same content they have the same hash, so
            # the filename has to be used as key.
            if len(filterlist) == 0 or add == True:
                if rf[3] in objects:
                    print("FATAL: key already exists in dict!")
                    return {}
                objects[rf[3]] = rf[2]
    return objects


def get_file_timestamp(repo, tree, filename):
    '''Get timestamp for a file.
    @param repo Path to repository root.
    @param tree Hash of tree to use.
    @param filename Filename in tree
    @return Timestamp as string.
    '''
    output = subprocess.Popen(
            ["git", "log", "--format=%ai", "-n", "1", tree, filename],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=repo)
    cmdout = output.communicate()

    return cmdout[0].decode().rstrip()


def get_object(repo, blob, destfile):
    '''Get an identified object from the repository.
    @param repo Path to repository root.
    @param blob hash for blob to retrieve.
    @param destfile filename for blob output.
    @return True if file was successfully written, False on error.
    '''
    output = subprocess.Popen(["git", "cat-file", "-p", blob],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=repo)
    cmdout = output.communicate()
    # make sure output path exists
    if len(cmdout[1]) > 0:
        print("An error occured!\n")
        print(cmdout[1])
        return False
    if not os.path.exists(os.path.dirname(destfile)):
        os.makedirs(os.path.dirname(destfile))
    f = open(destfile, 'wb')
    f.write(cmdout[0])
    f.close()
    return True


def describe_treehash(repo, treehash):
    '''Retrieve output of git-describe for a given hash.
    @param repo Path to repository root.
    @param treehash Hash identifying the tree / commit to describe.
    @return Description string.
    '''
    output = subprocess.Popen(["git", "describe", treehash],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=repo)
    cmdout = output.communicate()
    if len(cmdout[1]) > 0:
        print("An error occured!\n")
        print(cmdout[1])
        return ""
    return cmdout[0].rstrip()


def scrape_files(repo, treehash, filelist, dest="", timestamp_files=[]):
    '''Scrape list of files from repository.
    @param repo Path to repository root.
    @param treehash Hash identifying the tree.
    @param filelist List of files to get from repository.
    @param dest Destination path for files. Files will get retrieved with full
                path from the repository, and the folder structure will get
                created below dest as necessary.
    @param timestamp_files List of files to also get the last modified date.
                           WARNING: this is SLOW!
    @return Destination path, filename:timestamp dict.
    '''
    print("Scraping files from repository")

    if dest == "":
        dest = tempfile.mkdtemp()
    treeobjects = get_lstree(repo, treehash, filelist)
    timestamps = {}
    for obj in treeobjects:
        get_object(repo, treeobjects[obj], os.path.join(dest.encode(), obj))
        for f in timestamp_files:
            if obj.find(f) == 0:
                timestamps[obj] = get_file_timestamp(repo, treehash, obj)

    return [dest, timestamps]


def archive_files(repo, treehash, filelist, basename, tmpfolder="",
        archive="tbz"):
    '''Archive list of files into tarball.
    @param repo Path to repository root.
    @param treehash Hash identifying the tree.
    @param filelist List of files to archive. All files in the archive if left
                    empty.
    @param basename Basename (including path) of output file. Will get used as
                    basename inside of the archive as well (i.e. no tarbomb).
    @param tmpfolder Folder to put intermediate files in. If no folder is given
                     a temporary one will get used.
    @param archive Type of archive to create. Supported values are "tbz" and
                   "7z". The latter requires the 7z binary available in the
                   system's path.
    @return Output filename.
    '''

    if tmpfolder == "":
        temp_remove = True
        tmpfolder = tempfile.mkdtemp()
    else:
        temp_remove = False
    workfolder = scrape_files(repo, treehash, filelist,
            os.path.join(tmpfolder, basename))[0]
    if basename is "":
        return ""
    print("Archiving files from repository")
    if archive == "7z":
        outfile = basename + ".7z"
        output = subprocess.Popen(["7z", "a",
            os.path.join(os.getcwd(), basename + ".7z"), basename],
            cwd=tmpfolder, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        output.communicate()
    elif archive == "tbz":
        outfile = basename + ".tar.bz2"
        tf = tarfile.open(outfile, "w:bz2")
        tf.add(workfolder, basename)
        tf.close()
    else:
        print("Files not archived")
    if tmpfolder != workfolder:
        shutil.rmtree(workfolder)
    if temp_remove:
        shutil.rmtree(tmpfolder)
    return outfile
