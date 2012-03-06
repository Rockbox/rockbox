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
import string
import gitscraper

# modules that are not part of python itself.
cpus = 1
try:
    import multiprocessing
    cpus = multiprocessing.cpu_count()
    print "Info: %s cores found." % cpus
except ImportError:
    print "Warning: multiprocessing module not found. Assuming 1 core."

# == Global stuff ==
# DLL files to ignore when searching for required DLL files.
systemdlls = ['advapi32.dll',
        'comdlg32.dll',
        'gdi32.dll',
        'imm32.dll',
        'kernel32.dll',
        'msvcrt.dll',
        'msvcrt.dll',
        'netapi32.dll',
        'ole32.dll',
        'oleaut32.dll',
        'setupapi.dll',
        'shell32.dll',
        'user32.dll',
        'winmm.dll',
        'winspool.drv',
        'ws2_32.dll']

gitrepo = os.path.abspath(os.path.join(os.path.dirname(__file__), "../.."))


# == Functions ==
def usage(myself):
    print "Usage: %s [options]" % myself
    print "       -q, --qmake=<qmake>   path to qmake"
    print "       -p, --project=<pro>   path to .pro file for building with local tree"
    print "       -t, --tag=<tag>       use specified tag from svn"
    print "       -a, --add=<file>      add file to build folder before building"
    print "       -s, --source-only     only create source archive"
    print "       -b, --binary-only     only create binary archive"
    if nsisscript != "":
        print "       -n, --makensis=<file> path to makensis for building Windows setup program."
    if sys.platform != "darwin":
        print "       -d, --dynamic         link dynamically instead of static"
    if sys.platform != "win32":
        print "       -x, --cross=          prefix to cross compile for win32"
    print "       -k, --keep-temp       keep temporary folder on build failure"
    print "       -h, --help            this help"
    print "  If neither a project file nor tag is specified trunk will get downloaded"
    print "  from svn."


def which(executable):
    path = os.environ.get("PATH", "").split(os.pathsep)
    for p in path:
        fullpath = p + "/" + executable
        if os.path.exists(fullpath):
            return fullpath
    print "which: could not find " + executable
    return ""


def getsources(treehash, filelist, dest):
    '''Get the files listed in filelist from svnsrv and put it at dest.'''
    gitscraper.scrape_files(gitrepo, treehash, filelist, dest)
    return 0


def getfolderrev(svnsrv):
    '''Get the most recent revision for svnsrv'''
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


def findqt(cross=""):
    '''Search for Qt4 installation. Return path to qmake.'''
    print "Searching for Qt"
    bins = [cross + "qmake", cross + "qmake-qt4"]
    for binary in bins:
        try:
            q = which(binary)
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


def qmake(qmake, projfile, platform=sys.platform, wd=".", static=True, cross=""):
    print "Running qmake in %s..." % wd
    command = [qmake, "-config", "release", "-config", "noccache"]
    if static == True:
        command.extend(["-config", "-static"])
    # special spec required?
    if len(qmakespec[platform]) > 0:
        command.extend(["-spec", qmakespec[platform]])
    # cross compiling prefix set?
    if len(cross) > 0:
        command.extend(["-config", "cross"])
    command.append(projfile)
    output = subprocess.Popen(command, stdout=subprocess.PIPE, cwd=wd)
    output.communicate()
    if not output.returncode == 0:
        print "qmake returned an error!"
        return -1
    return 0


def build(wd=".", platform=sys.platform, cross=""):
    # make
    print "Building ..."
    # use the current platforms make here, cross compiling uses the native make.
    command = [make[sys.platform]]
    if cpus > 1:
        command.append("-j")
        command.append(str(cpus))
    output = subprocess.Popen(command, stdout=subprocess.PIPE, cwd=wd)
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
    if platform != "darwin":
        # strip. OS X handles this via macdeployqt.
        print "Stripping binary."
        output = subprocess.Popen([cross + "strip", progexe[platform]], \
                                  stdout=subprocess.PIPE, cwd=wd)
        output.communicate()
        if not output.returncode == 0:
            print "Stripping failed!"
            return -1
    return 0


def upxfile(wd=".", platform=sys.platform):
    # run upx on binary
    print "UPX'ing binary ..."
    output = subprocess.Popen(["upx", progexe[platform]], \
                              stdout=subprocess.PIPE, cwd=wd)
    output.communicate()
    if not output.returncode == 0:
        print "UPX'ing failed!"
        return -1
    return 0


def runnsis(versionstring, nsis, script, srcfolder):
    # run script through nsis to create installer.
    print "Running NSIS ..."

    # Assume the generated installer gets placed in the same folder the nsi
    # script lives in.  This seems to be a valid assumption unless the nsi
    # script specifies a path. NSIS expects files relative to source folder so
    # copy progexe. Additional files are injected into the nsis script.

    # FIXME: instead of copying binaries around copy the NSI file and inject
    # the correct paths.
    # Only win32 supported as target platform so hard coded.
    b = srcfolder + "/" + os.path.dirname(script) + "/" \
        + os.path.dirname(progexe["win32"])
    if not os.path.exists(b):
        os.mkdir(b)
    shutil.copy(srcfolder + "/" + progexe["win32"], b)
    output = subprocess.Popen([nsis, srcfolder + "/" + script], \
                              stdout=subprocess.PIPE)
    output.communicate()
    if not output.returncode == 0:
        print "NSIS failed!"
        return -1
    setupfile = program + "-" + versionstring + "-setup.exe"
    # find output filename in nsis script file
    nsissetup = ""
    for line in open(srcfolder + "/" + script):
        if re.match(r'^[^;]*OutFile\s+', line) != None:
            nsissetup = re.sub(r'^[^;]*OutFile\s+"(.+)"', r'\1', line).rstrip()
    if nsissetup == "":
        print "Could not retrieve output file name!"
        return -1
    shutil.copy(srcfolder + "/" + os.path.dirname(script) + "/" + nsissetup, \
                setupfile)
    return 0


def nsisfileinject(nsis, outscript, filelist):
    '''Inject files in filelist into NSIS script file after the File line
       containing the main binary. This assumes that the main binary is present
       in the NSIS script and that all additiona files (dlls etc) to get placed
       into $INSTDIR.'''
    output = open(outscript, "w")
    for line in open(nsis, "r"):
        output.write(line)
        # inject files after the progexe binary.
        # Match the basename only to avoid path mismatches.
        if re.match(r'^\s*File\s*.*' + os.path.basename(progexe["win32"]), \
                    line, re.IGNORECASE):
            for f in filelist:
                injection = "    File /oname=$INSTDIR\\" + os.path.basename(f) \
                             + " " + os.path.normcase(f) + "\n"
                output.write(injection)
            output.write("    ; end of injected files\n")
    output.close()


def finddlls(program, extrapaths=[], cross=""):
    '''Check program for required DLLs. Find all required DLLs except ignored
       ones and return a list of DLL filenames (including path).'''
    # ask objdump about dependencies.
    output = subprocess.Popen([cross + "objdump", "-x", program], \
                              stdout=subprocess.PIPE)
    cmdout = output.communicate()

    # create list of used DLLs. Store as lower case as W32 is case-insensitive.
    dlls = []
    for line in cmdout[0].split('\n'):
        if re.match(r'\s*DLL Name', line) != None:
            dll = re.sub(r'^\s*DLL Name:\s+([a-zA-Z_\-0-9\.\+]+).*$', r'\1', line)
            dlls.append(dll.lower())

    # find DLLs in extrapaths and PATH environment variable.
    dllpaths = []
    for file in dlls:
        if file in systemdlls:
            print "System DLL:  " + file
            continue
        dllpath = ""
        for path in extrapaths:
            if os.path.exists(path + "/" + file):
                dllpath = re.sub(r"\\", r"/", path + "/" + file)
                print file + ": found at " + dllpath
                dllpaths.append(dllpath)
                break
        if dllpath == "":
            try:
                dllpath = re.sub(r"\\", r"/", which(file))
                print file + ": found at " + dllpath
                dllpaths.append(dllpath)
            except:
                print "MISSING DLL: " + file
    return dllpaths


def zipball(programfiles, versionstring, buildfolder, platform=sys.platform):
    '''package created binary'''
    print "Creating binary zipball."
    archivebase = program + "-" + versionstring
    outfolder = buildfolder + "/" + archivebase
    archivename = archivebase + ".zip"
    # create output folder
    os.mkdir(outfolder)
    # move program files to output folder
    for f in programfiles:
        if re.match(r'^(/|[a-zA-Z]:)', f) != None:
            shutil.copy(f, outfolder)
        else:
            shutil.copy(buildfolder + "/" + f, outfolder)
    # create zipball from output folder
    zf = zipfile.ZipFile(archivename, mode='w', compression=zipfile.ZIP_DEFLATED)
    for root, dirs, files in os.walk(outfolder):
        for name in files:
            physname = os.path.normpath(os.path.join(root, name))
            filename = string.replace(physname, os.path.normpath(buildfolder), "")
            zf.write(physname, filename)
        for name in dirs:
            physname = os.path.normpath(os.path.join(root, name))
            filename = string.replace(physname, os.path.normpath(buildfolder), "")
            zf.write(physname, filename)
    zf.close()
    # remove output folder
    shutil.rmtree(outfolder)
    return archivename


def tarball(programfiles, versionstring, buildfolder):
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


def macdeploy(versionstring, buildfolder, platform=sys.platform):
    '''package created binary to dmg'''
    dmgfile = program + "-" + versionstring + ".dmg"
    appbundle = buildfolder + "/" + progexe[platform]

    # workaround to Qt issues when building out-of-tree. Copy files into bundle.
    sourcebase = buildfolder + re.sub('[^/]+.pro$', '', project) + "/"
    print sourcebase
    for src in bundlecopy:
        shutil.copy(sourcebase + src, appbundle + "/" + bundlecopy[src])
    # end of Qt workaround

    output = subprocess.Popen(["macdeployqt", progexe[platform], "-dmg"], \
                              stdout=subprocess.PIPE, cwd=buildfolder)
    output.communicate()
    if not output.returncode == 0:
        print "macdeployqt failed!"
        return -1
    # copy dmg to output folder
    shutil.copy(buildfolder + "/" + program + ".dmg", dmgfile)
    return dmgfile


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


def tempclean(workfolder, nopro):
    if nopro == True:
        print "Cleaning up working folder %s" % workfolder
        shutil.rmtree(workfolder)
    else:
        print "Project file specified or cleanup disabled!"
        print "Temporary files kept at %s" % workfolder


def deploy():
    startup = time.time()

    try:
        opts, args = getopt.getopt(sys.argv[1:], "q:p:t:a:n:sbdkx:i:h",
            ["qmake=", "project=", "tag=", "add=", "makensis=", "source-only",
             "binary-only", "dynamic", "keep-temp", "cross=", "buildid=", "help"])
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
    keeptemp = False
    makensis = ""
    cross = ""
    buildid = None
    platform = sys.platform
    treehash = gitscraper.get_refs(gitrepo)['refs/remotes/origin/HEAD']
    if sys.platform != "darwin":
        static = True
    else:
        static = False
    for o, a in opts:
        if o in ("-q", "--qmake"):
            qt = a
        if o in ("-p", "--project"):
            proj = a
            cleanup = False
        if o in ("-a", "--add"):
            addfiles.append(a)
        if o in ("-n", "--makensis"):
            makensis = a
        if o in ("-s", "--source-only"):
            binary = False
        if o in ("-b", "--binary-only"):
            source = False
        if o in ("-d", "--dynamic") and sys.platform != "darwin":
            static = False
        if o in ("-k", "--keep-temp"):
            keeptemp = True
        if o in ("-t", "--tree"):
            treehash = a
        if o in ("-x", "--cross") and sys.platform != "win32":
            cross = a
            platform = "win32"
        if o in ("-i", "--buildid"):
            buildid = a
        if o in ("-h", "--help"):
            usage(sys.argv[0])
            sys.exit(0)

    if source == False and binary == False:
        print "Building build neither source nor binary means nothing to do. Exiting."
        sys.exit(1)

    print "Building " + progexe[platform] + " for " + platform
    # search for qmake
    if qt == "":
        qm = findqt(cross)
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
        revision = gitscraper.describe_treehash(gitrepo, treehash)
        # try to find a version number from describe output.
        # WARNING: this is broken and just a temporary workaround!
        v = re.findall('([\d\.a-f]+)', revision)
        if v:
            if v[-1].find('.') >= 0:
                revision = "v" + v[-1]
            else:
                revision = v[-1]
        if buildid == None:
            versionextra = ""
        else:
            versionextra = "-" + buildid
        sourcefolder = workfolder + "/" + program + "-" + str(revision) + versionextra + "/"
        archivename = program + "-" + str(revision) + versionextra + "-src.tar.bz2"
        ver = str(revision)
        os.mkdir(sourcefolder)
    else:
        workfolder = "."
        sourcefolder = "."
        archivename = ""
    # check if project file explicitly given. If yes, don't get sources from svn
    print "Version: %s" % revision
    if proj == "":
        proj = sourcefolder + project
        # get sources and pack source tarball
        if not getsources(treehash, svnpaths, sourcefolder) == 0:
            tempclean(workfolder, cleanup and not keeptemp)
            sys.exit(1)

        # replace version strings.
        print "Updating version information in sources"
        for f in regreplace:
            infile = open(sourcefolder + "/" + f, "r")
            incontents = infile.readlines()
            infile.close()

            outfile = open(sourcefolder + "/" + f, "w")
            for line in incontents:
                newline = line
                for r in regreplace[f]:
                    # replacements made on the replacement string:
                    # %REVISION% is replaced with the revision number
                    replacement = re.sub("%REVISION%", str(revision), r[1])
                    newline = re.sub(r[0], replacement, newline)
                    # %BUILD% is replaced with buildid as passed on the command line
                    if buildid != None:
                        replacement = re.sub("%BUILDID%", "-" + str(buildid), replacement)
                    else:
                        replacement = re.sub("%BUILDID%", "", replacement)
                    newline = re.sub(r[0], replacement, newline)
                outfile.write(newline)
            outfile.close()

        if source == True:
            print "Creating source tarball %s\n" % archivename
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
    # append buildid if any.
    if buildid != None:
        ver += "-" + buildid

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
    if not qmake(qm, proj, platform, sourcefolder, static, cross) == 0:
        tempclean(workfolder, cleanup and not keeptemp)
        sys.exit(1)
    if not build(sourcefolder, platform, cross) == 0:
        tempclean(workfolder, cleanup and not keeptemp)
        sys.exit(1)
    buildtime = time.time() - buildstart
    progfiles = programfiles
    progfiles.append(progexe[platform])
    if platform == "win32":
        if useupx == True:
            if not upxfile(sourcefolder, platform) == 0:
                tempclean(workfolder, cleanup and not keeptemp)
                sys.exit(1)
        dllfiles = finddlls(sourcefolder + "/" + progexe[platform], \
                            [os.path.dirname(qm)], cross)
        if len(dllfiles) > 0:
            progfiles.extend(dllfiles)
        archive = zipball(progfiles, ver, sourcefolder, platform)
        # only when running native right now.
        if nsisscript != "" and makensis != "":
            nsisfileinject(sourcefolder + "/" + nsisscript, sourcefolder \
                           + "/" + nsisscript + ".tmp", dllfiles)
            runnsis(ver, makensis, nsisscript + ".tmp", sourcefolder)
    elif platform == "darwin":
        archive = macdeploy(ver, sourcefolder, platform)
    else:
        if platform == "linux2":
            for p in progfiles:
                prog = sourcefolder + "/" + p
                output = subprocess.Popen(["file", prog],
                        stdout=subprocess.PIPE)
                res = output.communicate()
                if re.findall("ELF 64-bit", res[0]):
                    ver += "-64bit"
                    break

        archive = tarball(progfiles, ver, sourcefolder)

    # remove temporary files
    tempclean(workfolder, cleanup)

    # display summary
    headline = "Build Summary for %s" % program
    print "\n", headline, "\n", "=" * len(headline)
    if not archivename == "":
        filestats(archivename)
    filestats(archive)
    duration = time.time() - startup
    durmins = (int)(duration / 60)
    dursecs = (int)(duration % 60)
    buildmins = (int)(buildtime / 60)
    buildsecs = (int)(buildtime % 60)
    print "Overall time %smin %ssec, building took %smin %ssec." % \
        (durmins, dursecs, buildmins, buildsecs)


if __name__ == "__main__":
    print "You cannot run this module directly!"
    print "Set required environment and call deploy()."
