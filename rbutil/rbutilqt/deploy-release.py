#!/usr/bin/python
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
# Copyright (c) 2009 Dominik Riebeling
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
#
# Automate building releases for deployment.
# Run from source folder. Error checking / handling rather is limited.
# If the required Qt installation isn't in PATH use --qmake option.
# Tested on Linux and MinGW / W32
#
# requires python which package (http://code.google.com/p/which/)
# requires upx.exe in PATH on Windows.
#

import re
import os
import sys
import tarfile
import zipfile
import shutil
import subprocess
import getopt
import which
import time

# == Global stuff ==
# Windows nees some special treatment. Differentiate between program name
# and executable filename.
program = "rbutilqt"
if sys.platform == "win32":
    progexe = "Release/rbutilqt.exe"
else:
    progexe = program

programfiles = [ progexe ]


# == Functions ==
def usage(myself):
    print "Usage: %s [options]" % myself
    print "       -q, --qmake=<qmake>   path to qmake"
    print "       -h, --help            this help"


def findversion(versionfile):
    '''figure most recent program version from version.h,
    returns version string.'''
    h = open(versionfile, "r")
    c = h.read()
    h.close()
    r = re.compile("#define +VERSION +\"(.[0-9\.a-z]+)\"")
    m = re.search(r, c)
    s = re.compile("\$Revision: +([0-9]+)")
    n = re.search(s, c)
    if n == None:
        print "WARNING: Revision not found!"
    return m.group(1)


def findqt():
    '''Search for Qt4 installation. Return path to qmake.'''
    print "Searching for Qt"
    bins = ["qmake", "qmake-qt4"]
    for binary in bins:
        try:
            q = which.which(binary)
            if len(q) > 0:
                result = checkqt(q)
                if not result == "":
                    return result
        except:
            print sys.exc_value

    return ""


def checkqt(qmakebin):
    '''Check if given path to qmake exists and is a suitable version.'''
    result = ""
    # check if binary exists
    if not os.path.exists(qmakebin):
        print "Specified qmake path does not exist!"
        return result
    # check version
    output = subprocess.Popen([qmakebin, "-version"], stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
    cmdout = output.communicate()
    # don't check the qmake return code here, Qt3 doesn't return 0 on -version.
    for ou in cmdout:
        r = re.compile("Qt[^0-9]+([0-9\.]+[a-z]*)")
        m = re.search(r, ou)
        if not m == None:
            print "Qt found: %s" % m.group(1)
            s = re.compile("4\..*")
            n = re.search(s, m.group(1))
            if not n == None:
                result = qmakebin
    return result


def removedir(folder):
    # remove output folder
    for root, dirs, files in os.walk(folder, topdown=False):
        for name in files:
            os.remove(os.path.join(root, name))
        for name in dirs:
            os.rmdir(os.path.join(root, name))
    os.rmdir(folder)


def qmake(qmake="qmake"):
    print "Running qmake ..."
    output = subprocess.Popen([qmake, "-config", "static", "-config", "release"],
                stdout=subprocess.PIPE)
    output.communicate()
    if not output.returncode == 0:
        print "qmake returned an error!"
        return -1
    return 0


def build():
    # make
    print "Building ..."
    output = subprocess.Popen(["make"], stdout=subprocess.PIPE)
    output.communicate()
    if not output.returncode == 0:
        print "Build failed!"
        return -1
    # strip
    print "Stripping binary."
    output = subprocess.Popen(["strip", progexe], stdout=subprocess.PIPE)
    output.communicate()
    if not output.returncode == 0:
        print "Stripping failed!"
        return -1
    return 0


def upxfile():
    # run upx on binary
    print "UPX'ing binary ..."
    output = subprocess.Popen(["upx", progexe], stdout=subprocess.PIPE)
    output.communicate()
    if not output.returncode == 0:
        print "UPX'ing failed!"
        return -1
    return 0


def zipball(versionstring):
    '''package created binary'''
    print "Creating binary zipball."
    outfolder = program + "-v" + versionstring
    archivename = outfolder + ".zip"
    # create output folder
    os.mkdir(outfolder)
    # move program files to output folder
    for f in programfiles:
        shutil.copy(f, outfolder)
    # create zipball from output folder
    zf = zipfile.ZipFile(archivename, mode='w', compression=zipfile.ZIP_DEFLATED)
    for root, dirs, files in os.walk(outfolder):
        for name in files:
            zf.write(os.path.join(root, name))
        for name in dirs:
            zf.write(os.path.join(root, name))
    zf.close()
    # remove output folder
    removedir(outfolder)
    st = os.stat(archivename)
    print "done: %s, %i bytes" % (archivename, st.st_size)
    return archivename


def tarball(versionstring):
    '''package created binary'''
    print "Creating binary tarball."
    outfolder = program + "-v" + versionstring
    archivename = outfolder + ".tar.bz2"
    # create output folder
    os.mkdir(outfolder)
    # move program files to output folder
    for f in programfiles:
        shutil.copy(f, outfolder)
    # create tarball from output folder
    tf = tarfile.open(archivename, mode='w:bz2')
    tf.add(outfolder)
    tf.close()
    # remove output folder
    removedir(outfolder)
    st = os.stat(archivename)
    print "done: %s, %i bytes" % (archivename, st.st_size)
    return archivename


def main():
    startup = time.time()
    try:
        opts, args = getopt.getopt(sys.argv[1:], "q:h", ["qmake=", "help"])
    except getopt.GetoptError, err:
        print str(err)
        usage(sys.argv[0])
        sys.exit(1)
    qt = ""
    for o, a in opts:
        if o in ("-q", "--qmake"):
            qt = a
        if o in ("-h", "--help"):
            usage(sys.argv[0])
            sys.exit(0)

    # qmake path
    if qt == "":
        qm = findqt()
    else:
        qm = checkqt(qt)
    if qm == "":
        print "ERROR: No suitable Qt installation found."
        sys.exit(1)

    # figure version from sources
    ver = findversion("version.h")
    header = "Building %s %s" % (program, ver)
    print header
    print len(header) * "="

    # build it.
    if not qmake(qm) == 0:
        os.exit(1)
    if not build() == 0:
        sys.exit(1)
    if sys.platform == "win32":
        if not upxfile() == 0:
            sys.exit(1)
        zipball(ver)
    else:
        tarball(ver)
    print "done."
    duration = time.time() - startup
    durmins = (int)(duration / 60)
    dursecs = (int)(duration % 60)
    print "Building took %smin %ssec." % (durmins, dursecs)


if __name__ == "__main__":
    main()


