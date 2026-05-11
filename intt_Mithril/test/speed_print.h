#ifndef PRINT_SPEED_H
#define PRINT_SPEED_H

#include <stddef.h>
#include <stdint.h>

void print_results(const char *s, uint64_t *t, size_t tlen);
uint64_t print_results_ex(const char *s, uint64_t *t, size_t tlen, uint64_t *out_avg);

#endif
