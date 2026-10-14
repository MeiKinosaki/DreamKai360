CC ?= cc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra
.PHONY: test clean
test: build/test_core
	./build/test_core
build/test_core: src/dreamkai360.c src/dreamkai360.h tests/test_core.c
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc src/dreamkai360.c tests/test_core.c -o $@
clean:
	rm -rf build
