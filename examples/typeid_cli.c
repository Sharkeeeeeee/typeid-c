/* A tiny command-line tool built on typeid.h.
 *
 *   $ ./typeid new user
 *   user_01m89p3fzde1ftq481qavnfwaz
 *   $ ./typeid decode user_01m89p3fzde1ftq481qavnfwaz
 *   type: user
 *   uuid: 01a21361-bfed-705f-ab91-01bab757f15f
 *   time: 1795621502957
 *   $ ./typeid encode user 01a21361-bfed-705f-ab91-01bab757f15f
 *   user_01m89p3fzde1ftq481qavnfwaz
 */
#define TYPEID_IMPLEMENTATION
#include "../typeid.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int usage(void) {
    fprintf(stderr,
            "usage:\n"
            "  typeid new [prefix] [count]\n"
            "  typeid decode <typeid>\n"
            "  typeid encode <prefix> <uuid>\n");
    return 2;
}

static int fail(typeid_err e) {
    fprintf(stderr, "error: %s\n", typeid_strerror(e));
    return 1;
}

int main(int argc, char **argv) {
    typeid_t tid;
    char buf[TYPEID_STR_SIZE];
    char uuid[TYPEID_UUID_STR_SIZE];
    typeid_err e;

    if (argc < 2) return usage();

    if (strcmp(argv[1], "new") == 0) {
        const char *prefix = argc > 2 ? argv[2] : "";
        long n = argc > 3 ? strtol(argv[3], NULL, 10) : 1, i;
        for (i = 0; i < n; i++) {
            if ((e = typeid_new(&tid, prefix)) != TYPEID_OK) return fail(e);
            typeid_to_string(&tid, buf, sizeof buf, NULL);
            puts(buf);
        }
        return 0;
    }
    if (strcmp(argv[1], "decode") == 0 && argc == 3) {
        if ((e = typeid_parse(&tid, argv[2])) != TYPEID_OK) return fail(e);
        typeid_uuid_string(&tid, uuid);
        printf("type: %s\nuuid: %s\ntime: %llu\n", tid.prefix, uuid,
               (unsigned long long)typeid_timestamp_ms(&tid));
        return 0;
    }
    if (strcmp(argv[1], "encode") == 0 && argc == 4) {
        if ((e = typeid_from_uuid_str(&tid, argv[2], argv[3])) != TYPEID_OK) return fail(e);
        typeid_to_string(&tid, buf, sizeof buf, NULL);
        puts(buf);
        return 0;
    }
    return usage();
}
