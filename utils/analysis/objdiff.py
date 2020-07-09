#!/usr/bin/python
import sys
from subprocess import Popen, PIPE

if len(sys.argv) != 3:
    print ("""%s usage:
    %s obj1 obj2
    Calculate per-symbol and total size differences between obj1 and obj2,
    which may be any files that nm can read""" % ((sys.argv[0],)*2))
    sys.exit(2)

obj1 = sys.argv[1]
obj2 = sys.argv[2]

def getsyms(obj):
    proc = Popen(args=['nm', '-S', '-t', 'd', obj], stdout=PIPE, stderr=PIPE)
    out, err = proc.communicate()
    if err:
        print ("nm reported an error:\n")
        print (err)
        sys.exit(1)
    d = {}
    for l in out.splitlines():
        l = l.strip().split()
        if len(l) == 4:
            d[l[3]] = int(l[1])
    return d

diff = 0

d1 = getsyms(obj1)
d2 = getsyms(obj2)
l = [(k,v) for k,v in sorted(d1.items()) if k not in d2]
if l:
    print ("only in %s" % obj1)
    print (''.join("  %6d %s\n" % (v,k)) for k,v in l)
    diff -= sum(v for k,v in l)

l = [(k,v) for k,v in sorted(d2.items()) if k not in d1]
if l:
    print ("only in %s" % obj2)
    print (''.join("%6d %s\n" % (v,k)) for k,v in l)
    diff += sum(v for k,v in l)

l = [(k,v,d2[k]) for k,v in sorted(d1.items()) if k in d2 and d2[k] != v]
if l:
    print ("different sizes in %s and %s:" %(obj1, obj2))
    print (''.join("  %6d %6d %s\n" % (v1,v2,k)) for k,v1,v2 in l)
    diff += sum(v2-v1 for k,v1,v2 in l)

if diff:
    print ("total size difference: %+d" % diff)
else:
    print ("total size difference:  0")
