#!/bin/sh

set -xe

./build.sh

rm -r test/
mkdir -p test/examples

for example in `find examples/ -name \*.basm | sed "s/\.basm//"`; do
    ./build/bin/bmr -p "build/$example.bm" -ao "test/$example.expected.out"
done
