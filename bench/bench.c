/* Micro-benchmark: ns/op for new, encode (to_string) and parse. */
#define _POSIX_C_SOURCE 200809L
#define TYPEID_IMPLEMENTATION
#include "../typeid.h"

#include <stdio.h>
#include <time.h>

#define N 2000000

static double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

int main(void) {
    typeid_t tid;
    char buf[TYPEID_STR_SIZE];
    volatile unsigned sink = 0;
    double t;
    long i;

    t = now_ns();
    for (i = 0; i < N; i++) { typeid_new(&tid, "user"); sink += tid.uuid[15]; }
    printf("typeid_new        %6.1f ns/op\n", (now_ns() - t) / N);

    {
        typeid_gen_t gen;
        typeid_gen_init(&gen);
        t = now_ns();
        for (i = 0; i < N; i++) { typeid_gen_next(&gen, &tid, "user"); sink += tid.uuid[15]; }
        printf("typeid_gen_next   %6.1f ns/op\n", (now_ns() - t) / N);
    }

    t = now_ns();
    for (i = 0; i < N; i++) { tid.uuid[15] = (uint8_t)i; typeid_to_string(&tid, buf, sizeof buf, NULL); sink += (unsigned char)buf[30]; }
    printf("typeid_to_string  %6.1f ns/op\n", (now_ns() - t) / N);

    t = now_ns();
    for (i = 0; i < N; i++) { typeid_parse(&tid, "user_01h455vb4pex5vsknk084sn02q"); sink += tid.uuid[15]; }
    printf("typeid_parse      %6.1f ns/op\n", (now_ns() - t) / N);
    return (int)(sink & 0);
}
