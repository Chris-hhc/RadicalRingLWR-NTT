#include <immintrin.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { N = 256, Q = 8380417 };
enum {
  RANDOM_CASES = 10000,
  WARMUP_ITERS = 1000,
  NUMBER_OF_CYCLE_SAMPLES = 100000,
};

#define N_CYCLE_TIMESTAMPS (NUMBER_OF_CYCLE_SAMPLES + 1)

#define REF(sym) pqcrystals_dilithium2_ref_##sym
#define AVX(sym) pqcrystals_dilithium2_avx2_##sym

typedef union {
  int32_t coeffs[N];
  __m256i vec[N / 8];
} poly_t;

typedef union {
  int32_t coeffs[624];
  __m256i vec[624 / 8];
} qdata_t;

extern void REF(ntt)(int32_t a[N]);
extern void AVX(ntt_avx)(__m256i *a, const __m256i *qdata);
extern void AVX(nttunpack_avx)(__m256i *a);
extern const qdata_t AVX(qdata);

static uint64_t rng_state = 0x123456789abcdef0ULL;
static uint64_t ts_avx[N_CYCLE_TIMESTAMPS];
static uint64_t ts_ref[N_CYCLE_TIMESTAMPS];

static uint64_t rng_next(void) {
  uint64_t x = rng_state;
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  rng_state = x;
  return x * 2685821657736338717ULL;
}

static int32_t rand_range(int32_t lo, int32_t hi) {
  uint64_t span = (uint64_t)((int64_t)hi - (int64_t)lo + 1);
  return (int32_t)(lo + (int32_t)(rng_next() % span));
}

static void fill_poly(poly_t *p) {
  for (int i = 0; i < N; ++i) {
    p->coeffs[i] = rand_range(-2 * Q, 2 * Q);
  }
}

static void checksum_add(const poly_t *p, int64_t *checksum) {
  for (int i = 0; i < N; ++i) {
    *checksum += p->coeffs[i];
  }
}

static uint64_t cpucycles(void) {
  uint64_t result;

  __asm__ volatile("rdtsc; shlq $32,%%rdx; orq %%rdx,%%rax"
                   : "=a"(result)
                   :
                   : "%rdx");

  return result;
}

static uint64_t cpucycles_overhead(void) {
  uint64_t overhead = UINT64_MAX;

  for (int i = 0; i < 100000; i++) {
    uint64_t t0 = cpucycles();
    __asm__ volatile("");
    uint64_t t1 = cpucycles();
    if (t1 - t0 < overhead) {
      overhead = t1 - t0;
    }
  }

  return overhead;
}

static int cmp_uint64(const void *a, const void *b) {
  const uint64_t va = *(const uint64_t *)a;
  const uint64_t vb = *(const uint64_t *)b;
  if (va < vb) return -1;
  if (va > vb) return 1;
  return 0;
}

static uint64_t average(uint64_t *t, size_t tlen) {
  uint64_t acc = 0;

  for (size_t i = 0; i < tlen; i++) {
    acc += t[i];
  }

  return acc / tlen;
}

static uint64_t median(uint64_t *t, size_t tlen) {
  qsort(t, tlen, sizeof(uint64_t), cmp_uint64);

  if (tlen & 1) {
    return t[tlen / 2];
  }
  return (t[tlen / 2 - 1] + t[tlen / 2]) / 2;
}

static uint64_t print_results_ex(const char *label, uint64_t *t, size_t tlen, uint64_t *out_avg) {
  static uint64_t overhead = UINT64_MAX;
  uint64_t avg;
  uint64_t med;

  if (tlen < 2) {
    fprintf(stderr, "ERROR: Need at least two cycle counts!\n");
    return 0;
  }

  if (overhead == UINT64_MAX) {
    overhead = cpucycles_overhead();
  }

  tlen--;
  for (size_t i = 0; i < tlen; ++i) {
    t[i] = t[i + 1] - t[i] - overhead;
  }

  avg = average(t, tlen);
  med = median(t, tlen);
  if (out_avg != NULL) {
    *out_avg = avg;
  }

  printf("%s\n", label);
  printf("median: %" PRIu64 " cycles/ticks\n", med);
  printf("average: %" PRIu64 " cycles/ticks\n\n", avg);
  return med;
}

static int correctness_random(void) {
  poly_t input, ref, avx, avx_unpack;
  int match_kind = 0;

  for (int tc = 0; tc < RANDOM_CASES; ++tc) {
    fill_poly(&input);
    ref = input;
    avx = input;

    REF(ntt)(ref.coeffs);
    AVX(ntt_avx)(avx.vec, AVX(qdata).vec);

    if (memcmp(ref.coeffs, avx.coeffs, sizeof(ref.coeffs)) == 0) {
      continue;
    }

    avx_unpack = avx;
    AVX(nttunpack_avx)(avx_unpack.vec);
    if (memcmp(ref.coeffs, avx_unpack.coeffs, sizeof(ref.coeffs)) == 0) {
      match_kind = 1;
      continue;
    }

    for (int i = 0; i < N; ++i) {
      if (ref.coeffs[i] != avx.coeffs[i] && ref.coeffs[i] != avx_unpack.coeffs[i]) {
        fprintf(stderr, "original ntt mismatch case=%d coeff=%d\n", tc, i);
        fprintf(stderr,
                "input=%" PRId32 " ref=%" PRId32 " avx=%" PRId32
                " avx_unpack=%" PRId32 "\n",
                input.coeffs[i], ref.coeffs[i], avx.coeffs[i], avx_unpack.coeffs[i]);
        return 0;
      }
    }

    fprintf(stderr, "original ntt mismatch case=%d (layout compare failed)\n", tc);
    return 0;
  }

  if (match_kind == 0) {
    printf("  correctness: OK, %d random cases (raw original ntt_avx vs ntt_ref)\n",
           RANDOM_CASES);
  } else {
    printf("  correctness: OK, %d random cases (ntt_avx matches after layout normalization)\n",
           RANDOM_CASES);
  }
  return 1;
}

static void bench_ref_cycles(const poly_t *input,
                             uint64_t *timestamps,
                             size_t n_timestamps,
                             int64_t *checksum) {
  poly_t work;
  int64_t acc = 0;
  size_t iters;

  if (n_timestamps < 2) {
    *checksum = 0;
    return;
  }

  iters = n_timestamps - 1;

  for (size_t i = 0; i < WARMUP_ITERS; i++) {
    memcpy(work.coeffs, input->coeffs, sizeof(work.coeffs));
    REF(ntt)(work.coeffs);
    checksum_add(&work, &acc);
  }

  timestamps[0] = cpucycles();
  for (size_t i = 0; i < iters; i++) {
    memcpy(work.coeffs, input->coeffs, sizeof(work.coeffs));
    REF(ntt)(work.coeffs);
    checksum_add(&work, &acc);
    timestamps[i + 1] = cpucycles();
  }

  *checksum = acc;
}

static void bench_avx_cycles(const poly_t *input,
                             uint64_t *timestamps,
                             size_t n_timestamps,
                             int64_t *checksum) {
  poly_t work;
  int64_t acc = 0;
  size_t iters;

  if (n_timestamps < 2) {
    *checksum = 0;
    return;
  }

  iters = n_timestamps - 1;

  for (size_t i = 0; i < WARMUP_ITERS; i++) {
    memcpy(work.coeffs, input->coeffs, sizeof(work.coeffs));
    AVX(ntt_avx)(work.vec, AVX(qdata).vec);
    checksum_add(&work, &acc);
  }

  timestamps[0] = cpucycles();
  for (size_t i = 0; i < iters; i++) {
    memcpy(work.coeffs, input->coeffs, sizeof(work.coeffs));
    AVX(ntt_avx)(work.vec, AVX(qdata).vec);
    checksum_add(&work, &acc);
    timestamps[i + 1] = cpucycles();
  }

  *checksum = acc;
}

int main(void) {
  poly_t input;
  uint64_t med_avx, med_ref;
  uint64_t avg_avx = 0, avg_ref = 0;
  int64_t chk_avx = 0, chk_ref = 0;

  printf("original ntt\n");

  if (!correctness_random()) {
    return 1;
  }

  fill_poly(&input);

  printf("  benchmark (rdtsc, %d samples each, copy+transform per sample)\n",
         NUMBER_OF_CYCLE_SAMPLES);

  bench_avx_cycles(&input, ts_avx, N_CYCLE_TIMESTAMPS, &chk_avx);
  med_avx = print_results_ex("  original ntt_avx", ts_avx, N_CYCLE_TIMESTAMPS, &avg_avx);
  printf("  checksum=%" PRId64 "\n\n", chk_avx);

  bench_ref_cycles(&input, ts_ref, N_CYCLE_TIMESTAMPS, &chk_ref);
  med_ref = print_results_ex("  original ntt_ref", ts_ref, N_CYCLE_TIMESTAMPS, &avg_ref);
  printf("  checksum=%" PRId64 "\n\n", chk_ref);

  if (med_avx > 0) {
    printf("  speedup (median) original_ntt_ref / original_ntt_avx: %.4f\n",
           (double)med_ref / (double)med_avx);
    printf("  speedup (average) original_ntt_ref / original_ntt_avx: %.4f\n",
           (double)avg_ref / (double)avg_avx);
  }
  return 0;
}
