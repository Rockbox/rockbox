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
# Run from any folder to build
# - trunk
# - any tag (using the -t option)
# - any local folder (using the -p option)
# Will build a binary archive (tar.bz2 / zip) and source archive.
# The source archive won't be built for local builds. Trunk and
# tag builds will retrieve the sources directly from svn and build
# below the systems temporary folder.
#
# If the required Qt installation isn't in PATH use --qmake option.
# Tested on Linux and MinGW / W32
#
# requires python which package (http://code.google.com/p/which/)
# requires pysvn package.
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
import time
import hashlib
import tempfile

# modules that are not part of python itself.
try:
    import pysvn
except ImportError:
    print "Fatal: This script requires the pysvn package to run."
    print "       See http://pysvn.tigris.org/."
    sys.exit(-5)
try:
    import which
except ImportError:
    print "Fatal: This script requires the which package to run."
    print "       See http://code.google.com/p/which/."
    sys.exit(-5)

# == Global stuff ==
# Windows nees some special treatment. Differentiate between program name
# and executable filename.
program = "rbutilqt"
project = "rbutil/rbutilqt/rbutilqt.pro"
if sys.platform == "win32":
    progexe = "Release/rbutilqt.exe"
    make = "mingw32-make"
else:
    progexe = program
    make = "make"

programfiles = [ progexe ]

svnserver = "svn://svn.rockbox.org/rockbox/"
# Paths and files to retrieve from svn when creating a tarball.
# This is a mixed list, holding both paths and filenames.
svnpaths = [ "rbutil/",
             "tools/ucl",
             "tools/rbspeex",
             "apps/codecs/libspeex",
             "docs/COPYING",
             "tools/iriver.c",
             "tools/Makefile",
             "tools/mkboot.h",
             "tools/voicefont.c",
             "tools/VOICE_PAUSE.wav",
             "tools/wavtrim.h",
             "tools/iriver.h",
             "tools/mkboot.c",
             "tools/telechips.c",
             "tools/telechips.h",
             "tools/voicefont.h",
             "tools/wavtrim.c",
             "tools/sapi_voice.vbs" ]

# == Functions ==
def usage(myself):
    print "Usage: %s [options]" % myself
    print "       -q, --qmake=<qmake>   path to qmake"
    print "       -p, --project=<pro>   path to .pro file for building with local tree"
    print "       -t, --tag=<tag>       use specified tag from svn"
    print "       -a, --add=<file>      add file to build folder before building"
    print "       -s, --source-only     only create source archive"
    print "       -b, --binary-only     only create binary archive"
    print "       -d, --dynamic         link dynamically instead of static"
    print "       -h, --help            this help"
    print "  If neither a project file nor tag is specified trunk will get downloaded"
    print "  from svn."

def getsources(svnsrv, filelist, dest):
    '''Get the files listed in filelist from svnsrv and put it at dest.'''
    client = pysvn.Client()
    print "Checking out sources from %s, please wait." % svnsrv

    for elem in filelist:
        url = re.subn('/$', '', svnsrv + elem)[0]
        destpath = re.subn('/$', '', dest + elem)[0]
        # make sure the destination path does exist
        d = os.path.dirname(destpath)
        print d
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


def gettrunkrev(svnsrv):
    '''Get the revision of trunk for svnsrv'''
    client = pysvn.Client()
    entries = client.info2(svnsrv, recurse=False)
    return entries[0][1].rev.number


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


def qmake(qmake="qmake", projfile=project, wd=".", static=True):
    print "Running qmake in %s..." % wd
    command = [qmake, "-config", "release", "-config", "noccache"]
    if static == True:
        command.append("-config")
        command.append("static")
    command.append(projfile)
    output = subprocess.Popen(command, stdout=subprocess.PIPE, cwd=wd)
    output.communicate()
    if not output.returncode == 0:
        print "qmake returned an error!"
        return -1
    return 0


def build(wd="."):
    # make
    print "Building ..."
    output = subprocess.Popen([make], stdout=subprocess.PIPE, cwd=wd)
    while True:
        c = output.stdout.readline()
        sys.stdout.write(".")
        sys.stdout.flush()
        if not output.poll() == None:
            sys.stdout.write("\n")
            sys.stdout.flush()
            if not output.returncode == 0:
                print "Build failed!"
                return -1
            break
    # strip
    print "Stripping binary."
    output = subprocess.Popen(["strip", progexe], stdout=subprocess.PIPE, cwd=wd)
    output.communicate()
    if not output.returncode == 0:
        print "Stripping failed!"
        return -1
    return 0


def upxfile(wd="."):
    # run upx on binary
    print "UPX'ing binary ..."
    output = subprocess.Popen(["upx", progexe], stdout=subprocess.PIPE, cwd=wd)
    output.communicate()
    if not output.returncode == 0:
        print "UPX'ing failed!"
        return -1
    return 0


def zipball(versionstring, buildfolder):
    '''package created binary'''
    print "Creating binary zipball."
    archivebase = program + "-" + versionstring
    outfolder = buildfolder + "/" + archivebase
    archivename = archivebase + ".zip"
    # create output folder
    os.mkdir(outfolder)
    # move program files to output folder
    for f in programfiles:
        shutil.copy(buildfolder + "/" + f, outfolder)
    # create zipball from output folder
    zf = zipfile.ZipFile(archivename, mode='w', compression=zipfile.ZIP_DEFLATED)
    for root, dirs, files in os.walk(outfolder):
        for name in files:
            physname = os.path.join(root, name)
            filename = re.sub("^" + buildfolder, "", physname)
            zf.write(physname, filename)
        for name in dirs:
            physname = os.path.join(root, name)
            filename = re.sub("^" + buildfolder, "", physname)
            zf.write(physname, filename)
    zf.close()
    # remove output folder
    shutil.rmtree(outfolder)
    return archivename


def tarball(versionstring, buildfolder):
    '''package created binary'''
    print "Creating binary tarball."
    archivebase = program + "-" + versionstring
    outfolder = buildfolder + "/" + archivebase
    archivename = archivebase + ".tar.bz2"
    # create output folder
    os.mkdir(outfolder)
    # move program files to output folder
    for f in programfiles:
        shutil.copy(buildfolder + "/" + f, outfolder)
    # create tarball from output folder
    tf = tarfile.open(archivename, mode='w:bz2')
    tf.add(outfolder, archivebase)
    tf.close()
    # remove output folder
    shutil.rmtree(outfolder)
    return archivename


def filehashes(filename):
    '''Calculate md5 and sha1 hashes for a given file.'''
    if not os.path.exists(filename):
        return ["", ""]
    m = hashlib.md5()
    s = hashlib.sha1()
    f = open(filename, 'rb')
    while True:
        d = f.read(65536)
        if d == "":
            break
        m.update(d)
        s.update(d)
    return [m.hexdigest(), s.hexdigest()]


def filestats(filename):
    if not os.path.exists(filename):
        return
    st = os.stat(filename)
    print filename, "\n", "-" * len(filename)
    print "Size:    %i bytes" % st.st_size
    h = filehashes(filename)
    print "md5sum:  %s" % h[0]
    print "sha1sum: %s" % h[1]
    print "-" * len(filename), "\n"


def main():
    startup = time.time()
    try:
        opts, args = getopt.getopt(sys.argv[1:], "q:p:t:a:sbdh",
            ["qmake=", "project=", "tag=", "add=", "source-only", "binary-only", "dynamic", "help"])
    except getopt.GetoptError, err:
        print str(err)
        usage(sys.argv[0])
        sys.exit(1)
    qt = ""
    proj = ""
    svnbase = svnserver + "trunk/"
    tag = ""
    addfiles = []
    cleanup = True
    binary = True
    source = True
    static = True
    for o, a in opts:
        if o in ("-q", "--qmake"):
            qt = a
        if o in ("-p", "--project"):
            proj = a
            cleanup = False
        if o in ("-t", "--tag"):
            tag = a
            svnbase = svnserver + "tags/" + tag + "/"
        if o in ("-a", "--add"):
            addfiles.append(a)
        if o in ("-s", "--source-only"):
            binary = False
        if o in ("-b", "--binary-only"):
            source = False
        if o in ("-d", "--dynamic"):
            static = False
        if o in ("-h", "--help"):
            usage(sys.argv[0])
            sys.exit(0)

    if source == False and binary == False:
        print "Building build neither source nor binary means nothing to do. Exiting."
        sys.exit(1)

    # search for qmake
    if qt == "":
        qm = findqt()
    else:
        qm = checkqt(qt)
    if qm == "":
        print "ERROR: No suitable Qt installation found."
        sys.exit(1)

    # create working folder. Use current directory if -p option used.
    if proj == "":
        w = tempfile.mkdtemp()
        # make sure the path doesn't contain backslashes to prevent issues
        # later when running on windows.
        workfolder = re.sub(r'\\', '/', w)
        if not tag == "":
            sourcefolder = workfolder + "/" + tag + "/"
            archivename = tag + "-src.tar.bz2"
            # get numeric version part from tag
            ver = "v" + re.sub('^[^\d]+', '', tag)
        else:
            trunk = gettrunkrev(svnbase)
            sourcefolder = workfolder + "/rbutil-r" + str(trunk) + "/"
            archivename = "rbutil-r" + str(trunk) + "-src.tar.bz2"
            ver = "r" + str(trunk)
        os.mkdir(sourcefolder)
    else:
        workfolder = "."
        sourcefolder = "."
        archivename = ""
    # check if project file explicitly given. If yes, don't get sources from svn
    if proj == "":
        proj = sourcefolder + project
        # get sources and pack source tarball
        if not getsources(svnbase, svnpaths, sourcefolder) == 0:
            sys.exit(1)

        if source == True:
            tf = tarfile.open(archivename, mode='w:bz2')
            tf.add(sourcefolder, os.path.basename(re.subn('/$', '', sourcefolder)[0]))
            tf.close()
            if binary == False:
                shutil.rmtree(workfolder)
                sys.exit(0)
    else:
        # figure version from sources. Need to take path to project file into account.
        versionfile = re.subn('[\w\.]+$', "version.h", proj)[0]
        ver = findversion(versionfile)

    # check project file
    if not os.path.exists(proj):
        print "ERROR: path to project file wrong."
        sys.exit(1)

    # copy specified (--add) files to working folder
    for f in addfiles:
        shutil.copy(f, sourcefolder)
    buildstart = time.time()
    header = "Building %s %s" % (program, ver)
    print header
    print len(header) * "="

    # build it.
    if not qmake(qm, proj, sourcefolder, static) == 0:
        sys.exit(1)
    if not build(sourcefolder) == 0:
        sys.exit(1)
    if sys.platform == "win32":
        if not upxfile(sourcefolder) == 0:
            sys.exit(1)
        archive = zipball(ver, sourcefolder)
    else:
        archive = tarball(ver, sourcefolder)

    # remove temporary files
    print "Cleaning up working folder %s" % workfolder
    if cleanup == True:
        shutil.rmtree(workfolder)
    else:
        print "Project file specified, not cleaning up!"

    # display summary
    headline = "Build Summary for %s" % program
    print "\n", headline, "\n", "=" * len(headline)
    if not archivename == "":
        filestats(archivename)
    filestats(archive)
    duration = time.time() - startup
    building = time.time() - buildstart
    durmins = (int)(duration / 60)
    dursecs = (int)(duration % 60)
    buildmins = (int)(building / 60)
    buildsecs = (int)(building % 60)
    print "Overall time %smin %ssec, building took %smin %ssec." % \
        (durmins, dursecs, buildmins, buildsecs)


if __name__ == "__main__":
    main()

