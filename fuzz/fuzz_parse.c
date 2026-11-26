/* libFuzzer harness: parse arbitrary input and check invariants.
 *
 *   clang -g -O1 -fsanitize=fuzzer,address,undefined fuzz/fuzz_parse.c -o fuzz_parse
 *   ./fuzz_parse -max_total_time=60
 */
#define TYPEID_IMPLEMENTATION
#include "../typeid.h"

#include <stdlib.h>
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    typeid_t tid, again;
    char buf[TYPEID_STR_SIZE];
    size_t len;

    if (typeid_parse_n(&tid, (const char *)data, size) != TYPEID_OK) return 0;

    /* Anything that parses must be canonical: re-encoding gives the input. */
    if (typeid_to_string(&tid, buf, sizeof buf, &len) != TYPEID_OK) abort();
    if (len != size || memcmp(buf, data, size) != 0) abort();
    if (typeid_parse(&again, buf) != TYPEID_OK) abort();
    if (!typeid_equal(&tid, &again)) abort();
    if (!typeid_prefix_valid(tid.prefix, strlen(tid.prefix))) abort();
    return 0;
}
