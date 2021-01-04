CFLAGS=	-Wall -Wextra -Wswitch-enum -Wmissing-prototypes -pedantic -std=c11
LIBS=
RM?=	rm -f

EXAMPLES!=	find examples/ -name \*.basm | sed "s/\.basm/\.bm/"
BINARIES=	basm \
		bme  \
		debasm

.SUFFIXES: .basm .bm

.basm.bm:
	./basm $< $@

.PHONY: all
all: ${BINARIES}

basm: ./src/basm.c ./src/bm.h
	$(CC) $(CFLAGS) -o basm ./src/basm.c $(LIBS)

bme: ./src/bme.c ./src/bm.h
	$(CC) $(CFLAGS) -o bme ./src/bme.c $(LIBS)

debasm: ./src/debasm.c ./src/bm.h
	$(CC) $(CFLAGS) -o debasm ./src/debasm.c $(LIBS)

.PHONY: examples
examples: basm ${EXAMPLES}

.PHONY: clean
clean:
	${RM} ${EXAMPLES}
	${RM} ${BINARIES}

