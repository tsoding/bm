# bm

Simple compiler ecosystem. Includes the backend and frontend for a couple of simple languages.

![logo](./logo/birch-296x328.png)

## Subprojects

- [bm](./bm) -- VM Bytecode Definitions and Emulators.
- [basm](./basm) -- Assembler for the VM Bytecode.
- [debasm](./debasm) -- Disassembler for the VM Bytecode.
- [bdb](./bdb) -- Debugger for the VM Bytecode.
- [bang](./bang) -- Procedural Programming Language that Compiles to the VM Bytecode.

## Quick Start

To build the entire project bootstrap the root [nobuild](https://github.com/tsoding/nobuild) and run it:

```console
$ cc -o nobuild nobuild.c
$ ./nobuild
```

But this is not very useful for the devs and usually reserved for CI.

When developing the project it is generally advised to just `cd` to the corresponding subfolder and use its nobuild instead:

```console
$ cd bm
$ cc -o nobuild nobuild.c
$ ./nobuild
```
