#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpucycles.h"
#include "speed_print.h"
#include "poly.h"
#include "ntt.h"

static const int32_t k_zetas[RRLWR_N] = RRLWR_KEM_ZETAS;

enum {
  WARMUP_ITERS = 1000,
  NUMBER_OF_CYCLE_SAMPLES = 100000,
  RANDOM_CASES = 100000
};

#define N_CYCLE_TIMESTAMPS (NUMBER_OF_CYCLE_SAMPLES + 1)

static void poly_fill_pattern(poly *p, int mode, int32_t prime, uint32_t seed)
{
  for (unsigned int i = 0; i < RRLWR_N; i++) {
    int32_t v;
    switch (mode) {
      case 0:
        v = 0;
        break;
      case 1:
        v = 1;
        break;
      case 2:
        v = (i & 1) ? 1 : -1;
        break;
      case 3:
        v = (int32_t)i - (int32_t)(RRLWR_N / 2);
        break;
      case 4:
        v = (i & 1) ? (prime - 1) : -(prime - 1);
        break;
      default:
        seed = seed * 1664525u + 1013904223u;
        {
          uint32_t span = 4u * (uint32_t)prime + 1u;
          v = (int32_t)(seed % span) - 2 * prime;
        }
        break;
    }
    p->coeffs[i] = v;
  }
}

static void poly_copy(poly *dst, const poly *src)
{
  memcpy(dst->coeffs, src->coeffs, sizeof(dst->coeffs));
}

static void expect_equal(const poly *a, const poly *b, const char *label, unsigned case_id)
{
  for (unsigned int i = 0; i < RRLWR_N; i++) {
    if (a->coeffs[i] != b->coeffs[i]) {
      fprintf(stderr,
              "%s mismatch case=%u coeff=%u got=%d expected=%d\n",
              label, case_id, i, a->coeffs[i], b->coeffs[i]);
      assert(a->coeffs[i] == b->coeffs[i]);
    }
  }
}

static void run_correctness(int32_t prime, int32_t primeinv, const char *label)
{
  const char *patterns[] = {"zero", "one", "alt", "increasing", "edge"};
  poly ref, avx;

  for (unsigned i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
    poly_fill_pattern(&ref, (int)i, prime, 123u + i);
    poly_copy(&avx, &ref);
    poly_ntt32(&ref, prime, primeinv, k_zetas);
    ntt_avx(avx.coeffs, prime, RRLWR_KEM_NTT_AVX_PARAMS);
    expect_equal(&avx, &ref, label, i);
  }

  for (unsigned case_id = 0; case_id < RANDOM_CASES; case_id++) {
    poly_fill_pattern(&ref, 5, prime, 0x9e3779b9u + 17u * case_id);
    poly_copy(&avx, &ref);
    poly_ntt32(&ref, prime, primeinv, k_zetas);
    ntt_avx(avx.coeffs, prime, RRLWR_KEM_NTT_AVX_PARAMS);
    expect_equal(&avx, &ref, label, case_id + 1000u);
  }
}

static int64_t bench_poly_ntt32_cycles(const poly *input, int32_t prime, int32_t primeinv,
                                       uint64_t *timestamps, size_t n_timestamps)
{
  poly work;
  int64_t acc = 0;
  size_t i;
  size_t iters;

  if (n_timestamps < 2) {
    return 0;
  }

  iters = n_timestamps - 1;

  for (i = 0; i < WARMUP_ITERS; i++) {
    poly_copy(&work, input);
    poly_ntt32(&work, prime, primeinv, k_zetas);
    acc ^= work.coeffs[i & (RRLWR_N - 1)];
  }

  timestamps[0] = cpucycles();
  for (i = 0; i < iters; i++) {
    poly_copy(&work, input);
    poly_ntt32(&work, prime, primeinv, k_zetas);
    acc ^= work.coeffs[(i * 17) & (RRLWR_N - 1)];
    timestamps[i + 1] = cpucycles();
  }

  return acc;
}

static int64_t bench_ntt_avx_cycles(const poly *input, int32_t prime,
                                    uint64_t *timestamps, size_t n_timestamps)
{
  poly work;
  int64_t acc = 0;
  size_t i;
  size_t iters;

  if (n_timestamps < 2) {
    return 0;
  }

  iters = n_timestamps - 1;

  for (i = 0; i < WARMUP_ITERS; i++) {
    poly_copy(&work, input);
    ntt_avx(work.coeffs, prime, RRLWR_KEM_NTT_AVX_PARAMS);
    acc ^= work.coeffs[i & (RRLWR_N - 1)];
  }

  timestamps[0] = cpucycles();
  for (i = 0; i < iters; i++) {
    poly_copy(&work, input);
    ntt_avx(work.coeffs, prime, RRLWR_KEM_NTT_AVX_PARAMS);
    acc ^= work.coeffs[(i * 17) & (RRLWR_N - 1)];
    timestamps[i + 1] = cpucycles();
  }

  return acc;
}

static void run_benchmark(int32_t prime, int32_t primeinv, const char *label)
{
  poly input;
  uint64_t ts_scalar[N_CYCLE_TIMESTAMPS];
  uint64_t ts_avx[N_CYCLE_TIMESTAMPS];
  uint64_t med_scalar;
  uint64_t med_avx;
  uint64_t avg_scalar = 0;
  uint64_t avg_avx = 0;
  int64_t scalar_checksum;
  int64_t avx_checksum;

  poly_fill_pattern(&input, 5, prime, 0x12345678u);

  printf("%s benchmark (poly_ntt32 vs ntt_avx, rdtsc, %d samples each, copy+ntt per sample)\n",
         label, NUMBER_OF_CYCLE_SAMPLES);

  scalar_checksum = bench_poly_ntt32_cycles(&input, prime, primeinv, ts_scalar,
                                              N_CYCLE_TIMESTAMPS);
  med_scalar = print_results_ex("  poly_ntt32", ts_scalar, N_CYCLE_TIMESTAMPS, &avg_scalar);
  printf("  checksum=%lld\n\n", (long long)scalar_checksum);

  avx_checksum = bench_ntt_avx_cycles(&input, prime, ts_avx, N_CYCLE_TIMESTAMPS);
  med_avx = print_results_ex("  ntt_avx", ts_avx, N_CYCLE_TIMESTAMPS, &avg_avx);
  printf("  checksum=%lld\n\n", (long long)avx_checksum);

  if (med_avx > 0) {
    printf("  speedup (median) poly_ntt32 / ntt_avx: %.4f\n",
           (double)med_scalar / (double)med_avx);
    printf("  speedup (average) poly_ntt32 / ntt_avx: %.4f\n",
           (double)avg_scalar / (double)avg_avx);
  }
}

int main(void)
{
  const int32_t prime = RRLWR_PKE_PRIME;
  const int32_t primeinv = RRLWR_PKE_PRIMEINV;

  run_correctness(prime, primeinv, "KEM NTT");
  run_benchmark(prime, primeinv, "KEM NTT");

  puts("ntt_mithril ok");
  return 0;
}
