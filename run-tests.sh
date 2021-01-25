#!/bin/sh

set -xe

./build.sh

for example in `find examples/ -name \*.basm | sed "s/\.basm//"`; do
    ./build/bin/bmr -p "build/$example.bm" -eo "test/$example.expected.out"
done

# TODO(#151): there is no script to run tests on Windows
