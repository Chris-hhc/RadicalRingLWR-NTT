#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "cpucycles.h"
#include "speed_print.h"

static int cmp_uint64(const void *a, const void *b) {
  if(*(uint64_t *)a < *(uint64_t *)b) return -1;
  if(*(uint64_t *)a > *(uint64_t *)b) return 1;
  return 0;
}

static uint64_t median(uint64_t *l, size_t llen) {
  qsort(l,llen,sizeof(uint64_t),cmp_uint64);

  if(llen%2) return l[llen/2];
  else return (l[llen/2-1]+l[llen/2])/2;
}

static uint64_t average(uint64_t *t, size_t tlen) {
  size_t i;
  uint64_t acc=0;

  for(i=0;i<tlen;i++)
    acc += t[i];

  return acc/tlen;
}

uint64_t print_results_ex(const char *s, uint64_t *t, size_t tlen, uint64_t *out_avg) {
  size_t i;
  static uint64_t overhead = -1;
  uint64_t avg;
  uint64_t med;

  if(tlen < 2) {
    fprintf(stderr, "ERROR: Need a least two cycle counts!\n");
    return 0;
  }

  if(overhead  == (uint64_t)-1)
    overhead = cpucycles_overhead();

  tlen--;
  for(i=0;i<tlen;++i)
    t[i] = t[i+1] - t[i] - overhead;

  avg = average(t, tlen);
  med = median(t, tlen);
  if(out_avg != NULL)
    *out_avg = avg;

  printf("%s\n", s);
  printf("median: %llu cycles/ticks\n", (unsigned long long)med);
  printf("average: %llu cycles/ticks\n", (unsigned long long)avg);
  printf("\n");
  return med;
}

void print_results(const char *s, uint64_t *t, size_t tlen) {
  (void)print_results_ex(s, t, tlen, NULL);
}
