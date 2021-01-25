#!/bin/sh

set -xe

CC=${CC:=/usr/bin/cc}
CFLAGS="-Wall -Wextra -Wswitch-enum -Wmissing-prototypes -Wconversion -pedantic -fno-strict-aliasing -ggdb -std=c11"
LIBS=

mkdir -p build/bin
mkdir -p build/examples

$CC $CFLAGS -o build/bin/basm ./src/basm.c $LIBS
$CC $CFLAGS -o build/bin/bme ./src/bme.c $LIBS
$CC $CFLAGS -o build/bin/bmr ./src/bmr.c $LIBS
$CC $CFLAGS -o build/bin/debasm ./src/debasm.c $LIBS
$CC $CFLAGS -o build/bin/bdb ./src/bdb.c $LIBS
$CC $CFLAGS -o build/bin/basm2nasm ./src/basm2nasm.c $LIBS

for example in `find examples/ -name \*.basm | sed "s/\.basm//"`; do
    ./build/bin/basm -g "$example.basm" "build/$example.bm"
done
