#!/bin/sh

set -xe

CC=${CC:=/usr/bin/cc}
CFLAGS="-Wall -Wextra -Wswitch-enum -Wmissing-prototypes -pedantic -ggdb -std=c11"
LIBS=

$CC $CFLAGS -o basm ./src/basm.c $LIBS
$CC $CFLAGS -o bme ./src/bme.c $LIBS
$CC $CFLAGS -o debasm ./src/debasm.c $LIBS

for example in `find examples/ -name \*.basm | sed "s/\.basm//"`; do
    ./basm "$example.basm" "$example.bm"
done
