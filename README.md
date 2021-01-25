# BM

![birch](./assets/birch-296x328.png)

Simple Virtual Machine with its own Bytecode and Assembly language.

## Quick Start

```console
$ ./build.sh                  # or ./build_msvc.bat if you are on Windows
$ ./build/bin/bme -i ./build/examples/hello.bm
$ ./build/bin/bme -i ./build/examples/fib.bm
$ ./build/bin/bme -i ./build/examples/e.bm
$ ./build/bin/bme -i ./build/examples/pi.bm
```

## Toolchain

### basm

Assembly language for the Virtual Machine. For examples see [./examples/](./examples) folder.

### bme

BM emulator. Used to run programs generated by [basm](#basm).

### bdb

BM debuger. Used to step debug programs generated by [basm](#basm).

### debasm

Disassembler for the binary files generated by [basm](#basm)

### bmr

BM recorder. Used to record the output of binary files generated by [basm](#basm) and comparing those output to the expected ones. We use this tool for Integration Testing.

### basm2nasm

An experimental tool that translates BM files generated by [basm](#basm) to an assembly files in [NASM](https://www.nasm.us/) dialect for x86_64 Linux.

## Build Scripts

### build.sh

- Builds the entire [Toolchain](#Toolchain) and puts the executables to `./build/bin/`.
- Translates all the [examples](./examples/) with [basm](#basm) and puts the BM files to `./build/examples/`.

### build_msvc.bat

Same as [build.sh](#build.sh) but for MSVC compiler on Windows. Has to be run from the `vcvarsall.bat`.

### build-x86_64.sh

Runs [build.sh](#build.sh) and builds some of the [examples](./examples/) with an experimental [basm2nasm](#basm2nasm) translator. Puts the results to the `./build/examples/unix` folder.

### build_msvc-x86_64.bat

Runs [build_msvc.bat](#build_msvc.bat) and builds some of the [examples](./examples/) with an experimental [basm2nasm](#basm2nasm) translator. Puts the results to the `./build/examples/win` folder.

### record-tests.sh

Runs [build.sh](#build.sh) and records the expected outputs of the [examples](./examples/) with [bmr](#bmr) tool. Puts the expected outputs to the [./test/examples/](./test/examples/) folder.

### run-tests.sh

Runs [build.sh](#build.sh) and compares the actual outputs of the examples with expected ones in [./test/examples/](./test/examples/) folder using [bmr](#bmr) tool.

## Editor Support

### Emacs

Emacs mode available in [./tools/basm-mode.el](./tools/basm-mode.el). Until the language stabilized and we upload the mode on [MELPA](https://melpa.org/) you need to install this mode manually.

Add the following lines to your `.emacs` file:

```emacs-lisp
(add-to-list 'load-path "/path/to/basm-mode/")
(require 'basm-mode)
```

### Vim

Copy [./tools/basm.vim](./tools/basm.vim) in `.vim/syntax/basm.vim`. Add the following line to your `.vimrc` file:

```vimscript
autocmd BufRead,BufNewFile *.basm set filetype=basm
```
