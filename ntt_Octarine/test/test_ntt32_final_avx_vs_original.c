#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cpucycles.h"
#include "speed_print.h"
#include "ntt.h"
#include "poly.h"

static int32_t rrlwr_sign_zetas1[RRLWR_N] = RRLWR_SIGN_ZETAS1;
static int32_t rrlwr_sign_zetas2[RRLWR_N] = RRLWR_SIGN_ZETAS2;

enum {
  RANDOM_CASES = 10000,
  WARMUP_ITERS = 1000,
  NUMBER_OF_CYCLE_SAMPLES = 100000,
};

#define N_CYCLE_TIMESTAMPS (NUMBER_OF_CYCLE_SAMPLES + 1)

static uint64_t rng_state = 0x9e3779b97f4a7c15ULL;

static uint64_t rng_u64(void)
{
  uint64_t x = rng_state;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  rng_state = x;
  return x;
}

static void poly_copy(poly *dst, const poly *src)
{
  memcpy(dst->coeffs, src->coeffs, sizeof(dst->coeffs));
}

static void fill_random_poly(poly *p, int32_t prime)
{
  uint64_t span = 2ULL * (uint64_t)prime + 1ULL;

  for (unsigned int i = 0; i < RRLWR_N; ++i) {
    p->coeffs[i] = (int32_t)(rng_u64() % span) - prime;
  }
}

static void final_reduce_poly(poly *p, int32_t prime)
{
  for (unsigned int i = 0; i < RRLWR_N; ++i) {
    p->coeffs[i] = conditional_reduce32(p->coeffs[i], prime);
  }
}

static void final_normalize_poly(poly *p, int32_t prime)
{
  for (unsigned int i = 0; i < RRLWR_N; ++i) {
    p->coeffs[i] = conditional_final_reduce32(p->coeffs[i], prime);
  }
}

static int64_t checksum_poly(const poly *p)
{
  int64_t acc = 0;

  for (unsigned int i = 0; i < RRLWR_N; ++i) {
    acc ^= (int64_t)p->coeffs[i] * (int64_t)(i + 1);
  }

  return acc;
}

static void select_ntt_tables(int32_t prime,
                              const int32_t **ntt_param)
{
  if (prime == RRLWR_SIGN_PRIME1) {
    *ntt_param = RRLWR_SIGN_NTT_AVX_PARAMS1;
    return;
  }

  *ntt_param = RRLWR_SIGN_NTT_AVX_PARAMS2;
}

static int compare_random_cases(const char *label,
                                int32_t prime,
                                int32_t primeinv,
                                const int32_t fp_zetas[RRLWR_N])
{
  poly input, scalar, avx;
  const int32_t *ntt_param;

  select_ntt_tables(prime, &ntt_param);

  for (int case_id = 0; case_id < RANDOM_CASES; ++case_id) {
    fill_random_poly(&input, prime);
    poly_copy(&scalar, &input);
    poly_copy(&avx, &input);

    poly_ntt32(&scalar, prime, primeinv, fp_zetas);
    ntt_avx(avx.coeffs, prime, ntt_param);
    final_reduce_poly(&scalar, prime);
    final_reduce_poly(&avx, prime);
    final_normalize_poly(&scalar, prime);
    final_normalize_poly(&avx, prime);

    for (unsigned int i = 0; i < RRLWR_N; ++i) {
      if (scalar.coeffs[i] != avx.coeffs[i]) {
        int32_t diff = scalar.coeffs[i] - avx.coeffs[i];
        int32_t mod = diff % prime;
        fprintf(stderr,
                "%s case %d mismatch at %u: input=%d scalar=%d avx=%d diff=%d mod=%d\n",
                label, case_id, i, input.coeffs[i], scalar.coeffs[i], avx.coeffs[i], diff, mod);
        return 0;
      }
    }
  }

  return 1;
}

static int64_t bench_poly_ntt32_cycles(const poly *input,
                                       int32_t prime,
                                       int32_t primeinv,
                                       const int32_t fp_zetas[RRLWR_N],
                                       uint64_t *timestamps,
                                       size_t n_timestamps)
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
    poly_ntt32(&work, prime, primeinv, fp_zetas);
    acc ^= checksum_poly(&work);
  }

  timestamps[0] = cpucycles();
  for (i = 0; i < iters; i++) {
    poly_copy(&work, input);
    poly_ntt32(&work, prime, primeinv, fp_zetas);
    acc ^= checksum_poly(&work);
    timestamps[i + 1] = cpucycles();
  }

  return acc;
}

static int64_t bench_ntt_avx_cycles(const poly *input,
                                    int32_t prime,
                                    const int32_t fp_zetas[RRLWR_N],
                                    uint64_t *timestamps,
                                    size_t n_timestamps)
{
  poly work;
  int64_t acc = 0;
  const int32_t *ntt_param;
  size_t i;
  size_t iters;

  (void)fp_zetas;
  select_ntt_tables(prime, &ntt_param);

  if (n_timestamps < 2) {
    return 0;
  }

  iters = n_timestamps - 1;

  for (i = 0; i < WARMUP_ITERS; i++) {
    poly_copy(&work, input);
    ntt_avx(work.coeffs, prime, ntt_param);
    acc ^= checksum_poly(&work);
  }

  timestamps[0] = cpucycles();
  for (i = 0; i < iters; i++) {
    poly_copy(&work, input);
    ntt_avx(work.coeffs, prime, ntt_param);
    acc ^= checksum_poly(&work);
    timestamps[i + 1] = cpucycles();
  }

  return acc;
}

static int run_prime(const char *label,
                     int32_t prime,
                     int32_t primeinv,
                     const int32_t fp_zetas[RRLWR_N])
{
  poly input;
  uint64_t ts_scalar[N_CYCLE_TIMESTAMPS];
  uint64_t ts_avx[N_CYCLE_TIMESTAMPS];
  uint64_t med_scalar;
  uint64_t med_avx;
  uint64_t avg_scalar = 0;
  uint64_t avg_avx = 0;
  int64_t chk_orig;
  int64_t chk_avx;

  if (!compare_random_cases(label, prime, primeinv, fp_zetas)) {
    return 0;
  }

  fill_random_poly(&input, prime);

  printf("%s\n", label);
  printf("  correctness: OK, %d random cases\n", RANDOM_CASES);
  printf("  benchmark (rdtsc, %d samples each, copy+transform per sample)\n",
         NUMBER_OF_CYCLE_SAMPLES);

  chk_orig = bench_poly_ntt32_cycles(&input, prime, primeinv, fp_zetas, ts_scalar,
                                     N_CYCLE_TIMESTAMPS);
  med_scalar = print_results_ex("  poly_ntt32", ts_scalar, N_CYCLE_TIMESTAMPS, &avg_scalar);
  printf("  checksum=%lld\n\n", (long long)chk_orig);

  chk_avx = bench_ntt_avx_cycles(&input, prime, fp_zetas, ts_avx, N_CYCLE_TIMESTAMPS);
  med_avx = print_results_ex("  ntt_avx", ts_avx, N_CYCLE_TIMESTAMPS, &avg_avx);
  printf("  checksum=%lld\n\n", (long long)chk_avx);

  if (med_avx > 0) {
    printf("  speedup (median) poly_ntt32 / ntt_avx: %.4f\n",
           (double)med_scalar / (double)med_avx);
    printf("  speedup (average) poly_ntt32 / ntt_avx: %.4f\n",
           (double)avg_scalar / (double)avg_avx);
  }
  return 1;
}

int main(void)
{
  if (!run_prime("prime1", RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, rrlwr_sign_zetas1)) {
    return 1;
  }
  if (!run_prime("prime2", RRLWR_SIGN_PRIME2, RRLWR_SIGN_PRIME2INV, rrlwr_sign_zetas2)) {
    return 1;
  }
  puts("OK: final poly_ntt32 + external reduce matches ntt_avx");
  return 0;
}
