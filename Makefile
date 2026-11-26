CC ?= cc
CXX ?= c++
CXXFLAGS ?= -O2 -Wall -Wextra -Wpedantic
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic
PYTHON ?= python3

ifeq ($(shell uname -s),Darwin)
  SHARED = -dynamiclib
  EXT = dylib
else
  SHARED = -shared
  EXT = so
endif

.PHONY: all test test-c test-cpp test-cli test-sqlite bench fuzz vectors clean

all: typeid typeid.$(EXT)

typeid: examples/typeid_cli.c typeid.h
	$(CC) $(CFLAGS) -o $@ examples/typeid_cli.c

typeid.$(EXT): sqlite/typeid_sqlite.c typeid.h
	$(CC) $(CFLAGS) -fPIC $(SHARED) -o $@ sqlite/typeid_sqlite.c

test_typeid: tests/test_typeid.c tests/vectors.h typeid.h
	$(CC) $(CFLAGS) -o $@ tests/test_typeid.c

test: test-c test-cpp test-cli test-sqlite

test-c: test_typeid
	./test_typeid

test_typeid_cpp: tests/test_typeid.cpp tests/vectors.h typeid.h typeid.hpp
	$(CXX) -std=c++17 $(CXXFLAGS) -pthread -o $@ tests/test_typeid.cpp

test-cpp: test_typeid_cpp
	./test_typeid_cpp

test-cli: typeid
	sh tests/test_cli.sh ./typeid

test-sqlite: typeid.$(EXT)
	$(PYTHON) tests/test_sqlite.py ./typeid.$(EXT) tests/spec

bench_typeid: bench/bench.c typeid.h
	$(CC) -O2 -o $@ bench/bench.c

bench: bench_typeid
	./bench_typeid

# Requires clang with libFuzzer
fuzz_parse: fuzz/fuzz_parse.c typeid.h
	clang -g -O1 -fsanitize=fuzzer,address,undefined -o $@ fuzz/fuzz_parse.c

fuzz: fuzz_parse
	./fuzz_parse -max_total_time=60 -max_len=128 fuzz/corpus

# Regenerate tests/vectors.h after updating tests/spec/*.json
vectors:
	$(PYTHON) tools/gen_vectors.py tests/spec tests/vectors.h

clean:
	rm -f typeid typeid.so typeid.dylib test_typeid test_typeid_cpp bench_typeid fuzz_parse
