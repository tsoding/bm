#!/bin/sh

set -xe

CC=${CC:=/usr/bin/cc}
CFLAGS="-Wall -Wextra -Wswitch-enum -Wmissing-prototypes -Wconversion -pedantic -fno-strict-aliasing -ggdb -std=c11"
LIBS=

$CC $CFLAGS -o basm ./src/basm.c $LIBS
$CC $CFLAGS -o bme ./src/bme.c $LIBS
$CC $CFLAGS -o debasm ./src/debasm.c $LIBS
$CC $CFLAGS -o bdb ./src/bdb.c $LIBS
$CC $CFLAGS -o basm2amd64 ./src/basm2amd64.c $LIBS

for example in `find examples/ -name \*.basm | sed "s/\.basm//"`; do
    ./basm -g "$example.basm" "$example.bm"
done
