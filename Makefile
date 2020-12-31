CFLAGS=-Wall -Wextra -Wswitch-enum -Wmissing-prototypes -pedantic -std=c11
LIBS=

EXAMPLES=./examples/fib.bm ./examples/123i.bm ./examples/123f.bm ./examples/e.bm

.PHONY: all
all: basm bme debasm

basm: ./src/basm.c ./src/bm.h
	$(CC) $(CFLAGS) -o basm ./src/basm.c $(LIBS)

bme: ./src/bme.c ./src/bm.h
	$(CC) $(CFLAGS) -o bme ./src/bme.c $(LIBS)

debasm: ./src/debasm.c ./src/bm.h
	$(CC) $(CFLAGS) -o debasm ./src/debasm.c $(LIBS)

.PHONY: examples
examples: $(EXAMPLES)

%.bm: %.basm basm
	./basm $< $@

.PHONY: clean
clean:
	rm -fr basm bme debasm

.PHONY: clean-examples
clean-examples:
	rm -f examples/*.bm
