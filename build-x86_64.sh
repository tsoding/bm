#!/bin/sh

set -xe

./build.sh

./build/bin/basm2nasm ./examples/123i.basm > ./build/examples/123i.asm
nasm -felf64 -g ./build/examples/123i.asm -o ./build/examples/123i.o
ld -o ./build/examples/123i.exe ./build/examples/123i.o

./build/bin/basm2nasm ./examples/fib.basm > ./build/examples/fib.asm
nasm -felf64 -g ./build/examples/fib.asm -o ./build/examples/fib.o
ld -o ./build/examples/fib.exe ./build/examples/fib.o

# TODO(#102): not all of the examples are translatable with basm2nasm
