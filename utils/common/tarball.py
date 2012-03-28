#!/usr/bin/python

import gitscraper
import tempfile
import subprocess
import os
import shutil

version = "v3.11"
repository = "."
basename = "rockbox-" + version
tree = gitscraper.get_refs(repository)["refs/remotes/origin/" + version]
tmpbase = tempfile.mkdtemp()
tmpdir = tmpbase + "/" + basename

gitscraper.archive_files(repository, tree, [], basename, tmpdir)

print "7z-ing files"
output = subprocess.Popen(["7z", "a",
        os.path.join(os.getcwd(), basename + ".7z"), basename], cwd=tmpbase)
cmdout = output.communicate()

shutil.rmtree(tmpbase)
