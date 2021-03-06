#!/usr/bin/python3

import os
import sys
import difflib
import filecmp


def getdiff(old, new):
    diff = []
    with open(old) as a:
        with open(new) as b:
            for l in difflib.unified_diff(
                a.read().splitlines(),
                b.read().splitlines()
            ):
                diff.append(l)
    return diff


def dc_getdiff(dc, old, new):
    diff = []
    for f in dc.left_only:
        diff.append("Only in {}: {}".format(old, f))
    for f in dc.right_only:
        diff.append("Only in {}: {}".format(new, f))
    for f in dc.diff_files:
        r = getdiff(os.path.join(old, f), os.path.join(new, f))
        diff.extend(r)
    for dn, dc in list(dc.subdirs.items()):
        r = dc_getdiff(dc, os.path.join(old, dn), os.path.join(new, dn))
        diff.extend(r)
    return diff


def dcdiff(old, new):
    dc = filecmp.dircmp(old, new)
    diff = dc_getdiff(dc, old, new)
    return diff


def main():
    for l in dcdiff(sys.argv[1], sys.argv[2]):
        print(l)


if __name__ == '__main__':
    main()
