#! /bin/sh

# autogen.sh - bootstrap the fetchmail build after a fresh git checkout
# (C) 2004, 2010  Matthias Andree -- GNU GPL V2 or newer

set -e

# sanity checks:
test -f fetchmail.h
test -f fetchmail.c
test -f Makefile.am

# kill junk:
rm -rf autom4te.cache

echo
echo "Please stand by while generating files,"
echo "this may take a minute or two..."
echo

# do not use -s here, Eric S. Raymond (ESR) writes they don't
# work well in Debian's build system
${AUTORECONF:=autoreconf} -iv

echo
echo "You can now run ./configure and make as usual. See INSTALL for details."
echo
