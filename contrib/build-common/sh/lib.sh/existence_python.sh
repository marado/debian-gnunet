#!/bin/sh

# This file is part of TALER
# (C) 2019 GNUnet e.V.
#
# This is very simple POSIX sh script which
# identifies the first matching
# python3 identifier in $PATH and produces
# configure.py from configure.py.in, and then
# calls the new executable configure.py.
#
# It should be portable on Unices. Report bugs on
# the bugtracker if you discover that it isn't
# working as intended.
#
# Authors:
# Author: ng0 <ng0@taler.net>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE
# LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
# OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
# ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
# THIS SOFTWARE.
#
# SPDX-License-Identifier: 0BSD

existence_python()
{
    # We have to check every possible variant of the
    # executable name because there is a PEP which
    # defines the executable to be like this.
    if existence python3; then
        if test ! -z "`python3 --version | awk '$2 ~ /3.7/ { print }'`"; then
            python="python3"
        else
            echo "*** At least python 3.7 is required for the buildsystem"
            exit 1
        fi
        # elif existence python3.1; then
        #     python="python3.1"
        # elif existence python3.2; then
        #     python="python3.2"
        # elif existence python3.3; then
        #     python="python3.3"
        # elif existence python3.4; then
        #     python="python3.4"
        # elif existence python3.5; then
        #     python="python3.5"
        # elif existence python3.6; then
        #     python="python3.6"
    elif existence python3.7; then
        python="python3.7"
    elif existence python3.8; then
        python="python3.8"
    else
        echo "*** No known python3 executable found in path ***"
        echo "*** falling back to env(1) python             ***"
        # Unreliable, but if env finds no python, we still can
        # not assume python in a fixed location.
        # TODO: Check this in a clean chroot!
        python="env python"
    fi

    # we could check the return value here via || echo "blafoo"
    # or fail anyway once configure.py is invoked because we
    # don't have python if we reach the point to fail.
    PYTHON=$($python -c 'import sys; print(sys.executable)')
}

existence_python
