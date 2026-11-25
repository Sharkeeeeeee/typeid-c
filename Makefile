CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic
PYTHON ?= python3

ifeq ($(shell uname -s),Darwin)
  SHARED = -dynamiclib
  EXT = dylib
else
  SHARED = -shared
  EXT = so
endif

.PHONY: all test test-c test-sqlite bench vectors clean

all: typeid typeid.$(EXT)

typeid: examples/typeid_cli.c typeid.h
	$(CC) $(CFLAGS) -o $@ examples/typeid_cli.c

typeid.$(EXT): sqlite/typeid_sqlite.c typeid.h
	$(CC) $(CFLAGS) -fPIC $(SHARED) -o $@ sqlite/typeid_sqlite.c

test_typeid: tests/test_typeid.c tests/vectors.h typeid.h
	$(CC) $(CFLAGS) -o $@ tests/test_typeid.c

test: test-c test-sqlite

test-c: test_typeid
	./test_typeid

test-sqlite: typeid.$(EXT)
	$(PYTHON) tests/test_sqlite.py ./typeid.$(EXT) tests/spec

bench_typeid: bench/bench.c typeid.h
	$(CC) -O2 -o $@ bench/bench.c

bench: bench_typeid
	./bench_typeid

# Regenerate tests/vectors.h after updating tests/spec/*.json
vectors:
	$(PYTHON) tools/gen_vectors.py tests/spec tests/vectors.h

clean:
	rm -f typeid typeid.so typeid.dylib test_typeid bench_typeid
