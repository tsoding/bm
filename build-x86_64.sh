#!/bin/sh

set -xe

./build.sh

mkdir -p build/examples/unix

./build/bin/basm2nasm -felf64 ./examples/123i.basm > ./build/examples/123i.asm
nasm -felf64 -F dwarf -g ./build/examples/123i.asm -o ./build/examples/unix/123i.o
ld -o ./build/examples/unix/123i.exe ./build/examples/unix/123i.o

./build/bin/basm2nasm -felf64 ./examples/fib.basm > ./build/examples/fib.asm
nasm -felf64 -F dwarf -g ./build/examples/fib.asm -o ./build/examples/unix/fib.o
ld -o ./build/examples/unix/fib.exe ./build/examples/unix/fib.o

# TODO(#102): not all of the examples are translatable with basm2nasm
