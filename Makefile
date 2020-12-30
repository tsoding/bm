CFLAGS=-Wall -Wextra -Wswitch-enum -Wmissing-prototypes -pedantic -std=c11
LIBS=

.PHONY: all
all: basm bme debasm

basm: ./src/basm.c ./src/bm.h
	$(CC) $(CFLAGS) -o basm ./src/basm.c $(LIBS)

bme: ./src/bme.c ./src/bm.h
	$(CC) $(CFLAGS) -o bme ./src/bme.c $(LIBS)

debasm: ./src/debasm.c ./src/bm.h
	$(CC) $(CFLAGS) -o debasm ./src/debasm.c $(LIBS)

.PHONY: examples
examples: ./examples/fib.bm ./examples/123i.bm ./examples/123f.bm ./examples/e.bm

./examples/fib.bm: basm ./examples/fib.basm
	./basm ./examples/fib.basm ./examples/fib.bm

./examples/123i.bm: basm ./examples/123i.basm
	./basm ./examples/123i.basm ./examples/123i.bm

./examples/123f.bm: basm ./examples/123f.basm
	./basm ./examples/123f.basm ./examples/123f.bm

./examples/e.bm: basm ./examples/e.basm
	./basm ./examples/e.basm ./examples/e.bm
