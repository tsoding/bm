# Assembly

BM comes with its own flavor of Assembly that we call BASM. This document describes the syntax and semantics of this language.

## Overview

A [Hello World](https://en.wikipedia.org/wiki/%22Hello,_World!%22_program) in BASM would look like this:

```basm
%const hello "Hello, World!" ; Add a "Hello, World" string to the memory of the
                             ; Virtual Machine and create a new binding `hello`
                             ; that stores the address to the beginning of that
                             ; string in memory.

main:                        ; Create a new label called `main`.
    push hello               ; Push address of the hello string onto the stack.
    push len(hello)          ; Push the size of the hello string onto the stack.
    native write             ; Call the native `write` function.
    halt                     ; Halt the Virtual Machine.

%entry main                  ; Declare label `main` as the entry point of the program.
```

The translation of the program occurs line by line:
- Empty lines are ignored.
- Everything after symbol `;` up until the end of the line is ignored.
- Any leading whitespaces in the lines are ignored.
- The lines that start with `%` are [Translation Directives](#translation-directives).
- Any other line has the following syntax: `[label:] [<instruction> [operand]]`.

The specific instructions are documented in the [Instructions](#instructions) section. For more information on how labels work read the [Labels](#labels) section.

## Labels

TBD

## Instructions

TBD

## Translation Directives

TBD
