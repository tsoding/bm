#!/bin/sh

set -xe

./build.sh

./basm2nasm ./examples/123i.basm > ./examples/123i.asm
nasm -felf64 -g ./examples/123i.asm -o ./examples/123i.o
ld -o ./examples/123i.exe ./examples/123i.o

./basm2nasm ./examples/fib.basm > ./examples/fib.asm
nasm -felf64 -g ./examples/fib.asm -o ./examples/fib.o
ld -o ./examples/fib.exe ./examples/fib.o

# TODO: not all of the examples are translatable with basm2nasm
