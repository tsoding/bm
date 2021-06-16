# bm

Simple compiler ecosystem. Includes the backend and frontend for a couple of simple languages.

![logo](./logo/birch-296x328.png)

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
