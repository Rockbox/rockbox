#!/usr/bin/env python
import os
import sys
import hashlib
import argparse
from collections import OrderedDict

EQ = '='
BLK_START = '{'
BLK_END = '}'

#name: {key: value, key: value}
def parse(filename):
    blocks = OrderedDict()
    blk = None
    key = None
    value = None

    with open(filename) as f:
        # read all lines
        for line in f:
            # if line has '=' sign treat it
            # as split symbol
            if EQ in line:
                key, value = line.split(EQ, 1)
                key = key.strip()
                value = value.strip()

                if value == BLK_START:
                    # value on the right of '=' is '{'
                    # so this opens new block
                    blk = key
                    blocks[key] = OrderedDict()

                elif value == BLK_END:
                    # value on the right of '=' is '}'
                    # this terminates block
                    blk = None

                else:
                    # key = value inside block
                    blocks[blk][key] = value

    # return parsed structure as dictionary
    return blocks

# write back internal representation into file
def dump(tree, filename=None):
    with open(filename, 'w') as f:
        for blk in tree.keys():
            f.write('\n%s={\n' % blk)
            for key,value in tree[blk].items():
                f.write('\t%s=%s\n' % (key,value))
            f.write('}\n')

if __name__=='__main__':
    description = 'Update information in update.txt control file.'
    usage = 'update_update.py filepath'

    argp = argparse.ArgumentParser(usage=usage, description=description)
    argp.add_argument('filepath', help='update.txt filepath to update contents of.')

    if len(sys.argv) == 1:
        argp.print_help()
        sys.exit(1)

    args = argp.parse_args()

    # build config file representation
    tree = parse(args.filepath)

    dir = os.path.dirname(args.filepath)

    # update all md5 sums
    for blk in tree.keys():
        filename = os.path.join(dir, os.path.basename(tree[blk]['file_path']))
        with open(filename) as f:
            tree[blk]['md5'] = hashlib.md5(f.read()).hexdigest()

    # write back result
    dump(tree, args.filepath)
