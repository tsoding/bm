# bm

Simple compiler ecosystem. Includes the backend and frontend for a couple of languages.

## Subprojects

- [bm](./bm) — VM Bytecode Definitions and Emulators.
- [basm](./basm) — Assembler for the VM Bytecode.
- [debasm](./debasm) — Disassembler for the VM Bytecode.
- [bdb](./bdb) — Debugger for the VM Bytecode.
- [bang ![logo](./bang/logo/logo-32.png)](./bang)  — Procedural Programming Language that Compiles to the VM Bytecode.

## Quick Start

To build and test the entire project bootstrap the root [nobuild](https://github.com/tsoding/nobuild) and run it like so:

```console
$ cc -o nobuild nobuild.c
$ ./nobuild test
```

For more info do `./nobuild help`

