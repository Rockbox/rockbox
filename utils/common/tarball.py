#!/usr/bin/python

import gitscraper
import os
import sys

if len(sys.argv) < 2:
    print("Usage: %s <version|hash>" % sys.argv[0])
    sys.exit()

repository = os.path.abspath(os.path.dirname(os.path.abspath(__file__)) + "/../..")
if '.' in sys.argv[1]:
    version = sys.argv[1]
    basename = "rockbox-" + version
    ref = "refs/tags/v" + version + "-final"
    refs = gitscraper.get_refs(repository)
    if ref in refs:
        tree = refs[ref]
    else:
        print("Could not find hash for version!")
        sys.exit()
else:
    tree = sys.argv[1]
    basename = "rockbox-" + tree

gitscraper.archive_files(repository, tree, [], basename, archive="7z")

print("done.")


