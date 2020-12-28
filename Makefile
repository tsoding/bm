CFLAGS=-Wall -Wextra -Wswitch-enum -std=c11 -pedantic
LIBS=

.PHONY: all
all: ebasm bmi

ebasm: ./src/ebasm.c ./src/bm.c
	$(CC) $(CFLAGS) -o ebasm ./src/ebasm.c $(LIBS)

bmi: ./src/bmi.c ./src/bm.c
	$(CC) $(CFLAGS) -o bmi ./src/bmi.c $(LIBS)


.PHONY: examples
examples: ./examples/fib.bm ./examples/123.bm

./examples/fib.bm: ebasm ./examples/fib.ebasm
	./ebasm ./examples/fib.ebasm ./examples/fib.bm

./examples/123.bm: ebasm ./examples/123.ebasm
	./ebasm ./examples/123.ebasm ./examples/123.bm
