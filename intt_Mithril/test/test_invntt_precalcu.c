#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpucycles.h"
#include "intt.h"
#include "poly.h"
#include "speed_print.h"
#include "parameters.h"
#include "intt_precalcu_tables.h"
#include "intt_final_fused_tables.h"

enum {
  RANDOM_CASES = 10000,
  WARMUP_ITERS = 1000,
  NUMBER_OF_CYCLE_SAMPLES = 100000,
};

#define N_CYCLE_TIMESTAMPS (NUMBER_OF_CYCLE_SAMPLES + 1)

static const int32_t reference_zetas[RRLWR_N] = RRLWR_KEM_ZETAS;

static uint64_t rng_state = 0x23456789abcdef01ULL;

static uint64_t rng_u64(void)
{
  uint64_t x = rng_state;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  rng_state = x;
  return x;
}

static void copy_poly(poly *dst, const poly *src)
{
  memcpy(dst->coeffs, src->coeffs, sizeof(dst->coeffs));
}

static void fill_constant_poly(poly *p, int32_t value)
{
  for (unsigned int i = 0; i < RRLWR_N; ++i) {
    p->coeffs[i] = value;
  }
}

static void fill_alternating_poly(poly *p)
{
  for (unsigned int i = 0; i < RRLWR_N; ++i) {
    p->coeffs[i] = (i & 1) ? -1 : 1;
  }
}

static void fill_increasing_poly(poly *p)
{
  for (unsigned int i = 0; i < RRLWR_N; ++i) {
    p->coeffs[i] = (int32_t)i;
  }
}

static void fill_qedge_poly(poly *p, int32_t prime)
{
  for (unsigned int i = 0; i < RRLWR_N; ++i) {
    p->coeffs[i] = (i & 1) ? -(prime - 1) : (prime - 1);
  }
}

static void fill_random_poly(poly *p, int32_t prime, int multiplier)
{
  int64_t bound = (int64_t)multiplier * prime;
  uint64_t span = (uint64_t)(2 * bound + 1);
  for (unsigned int i = 0; i < RRLWR_N; ++i) {
    p->coeffs[i] = (int32_t)((int64_t)(rng_u64() % span) - bound);
  }
}

static int64_t diff_i64(int32_t a, int32_t b)
{
  return (int64_t)a - b;
}

static int32_t diff_mod_prime(int32_t a, int32_t b, int32_t prime)
{
  int64_t r = diff_i64(a, b) % prime;
  if (r < 0) {
    r += prime;
  }
  return (int32_t)r;
}

static void compare_fused(const char *case_label,
                          int case_id,
                          const poly *input,
                          const poly *baseline,
                          const poly *fused,
                          int32_t prime,
                          int *fused_bitexact,
                          int *fused_modequiv)
{
  unsigned int first_mismatch = 0;
  unsigned int first_coeff = 0;
  int32_t first_baseline = 0;
  int32_t first_fused = 0;
  int32_t first_dmod = 0;

  for (unsigned int i = 0; i < RRLWR_N; ++i) {
    if (baseline->coeffs[i] != fused->coeffs[i]) {
      int32_t dmod = diff_mod_prime(baseline->coeffs[i], fused->coeffs[i], prime);
      if (*fused_bitexact && !first_mismatch) {
        first_mismatch = 1;
        first_coeff = i;
        first_baseline = baseline->coeffs[i];
        first_fused = fused->coeffs[i];
        first_dmod = dmod;
      }
      *fused_bitexact = 0;
      if (dmod != 0) {
        *fused_modequiv = 0;
      }
    }
  }

  if (!*fused_modequiv && first_mismatch) {
    fprintf(stderr,
            "fusedzeta first mismatch case=%s/%d coeff=%u input=%d baseline=%d fused=%d diff=%lld diff_mod_prime=%d\n",
            case_label, case_id, first_coeff, input->coeffs[first_coeff], first_baseline, first_fused,
            (long long)diff_i64(first_baseline, first_fused), first_dmod);
  }
}

static int run_case(const char *case_label,
                    int case_id,
                    const poly *input,
                    int32_t prime,
                    int32_t primeinv,
                    int32_t finalconst,
                    const int32_t *zetas_ref,
                    const int32_t *intt_fused_param,
                    int *fused_bitexact,
                    int *fused_modequiv)
{
  poly baseline, fused;

  copy_poly(&baseline, input);
  copy_poly(&fused, input);

  poly_invntt32(&baseline, prime, primeinv, finalconst, zetas_ref);
  intt_avx(fused.coeffs, prime, intt_fused_param);
  compare_fused(case_label, case_id, input, &baseline, &fused, prime, fused_bitexact, fused_modequiv);
  return 1;
}

static int run_tests_for_prime(int32_t prime,
                               int32_t primeinv,
                               int32_t finalconst,
                               const int32_t *zetas_ref,
                               const int32_t *intt_fused_param,
                               int *fused_bitexact,
                               int *fused_modequiv)
{
  poly input;

  fill_constant_poly(&input, 0);
  if (!run_case("zero", 0, &input, prime, primeinv, finalconst, zetas_ref, intt_fused_param, fused_bitexact, fused_modequiv)) return 0;
  fill_constant_poly(&input, 1);
  if (!run_case("one", 1, &input, prime, primeinv, finalconst, zetas_ref, intt_fused_param, fused_bitexact, fused_modequiv)) return 0;
  fill_alternating_poly(&input);
  if (!run_case("alternating", 2, &input, prime, primeinv, finalconst, zetas_ref, intt_fused_param, fused_bitexact, fused_modequiv)) return 0;
  fill_increasing_poly(&input);
  if (!run_case("increasing", 3, &input, prime, primeinv, finalconst, zetas_ref, intt_fused_param, fused_bitexact, fused_modequiv)) return 0;
  fill_qedge_poly(&input, prime);
  if (!run_case("qedge", 4, &input, prime, primeinv, finalconst, zetas_ref, intt_fused_param, fused_bitexact, fused_modequiv)) return 0;

  for (int case_id = 0; case_id < RANDOM_CASES; ++case_id) {
    fill_random_poly(&input, prime, (case_id & 1) ? 2 : 1);
    if (!run_case("random", 100 + case_id, &input, prime, primeinv, finalconst, zetas_ref, intt_fused_param, fused_bitexact, fused_modequiv)) {
      return 0;
    }
  }

  return 1;
}

static int64_t bench_poly_invntt32_cycles(const poly *input,
                                          int32_t prime,
                                          int32_t primeinv,
                                          int32_t finalconst,
                                          const int32_t *zetas_ref,
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
    copy_poly(&work, input);
    poly_invntt32(&work, prime, primeinv, finalconst, zetas_ref);
    acc ^= work.coeffs[i & (RRLWR_N - 1)];
  }

  timestamps[0] = cpucycles();
  for (i = 0; i < iters; i++) {
    copy_poly(&work, input);
    poly_invntt32(&work, prime, primeinv, finalconst, zetas_ref);
    acc ^= work.coeffs[(i * 17) & (RRLWR_N - 1)];
    timestamps[i + 1] = cpucycles();
  }

  return acc;
}

static int64_t bench_intt_avx_cycles(const poly *input,
                                     int32_t prime,
                                     const int32_t *intt_param,
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
    copy_poly(&work, input);
    intt_avx(work.coeffs, prime, intt_param);
    acc ^= work.coeffs[i & (RRLWR_N - 1)];
  }

  timestamps[0] = cpucycles();
  for (i = 0; i < iters; i++) {
    copy_poly(&work, input);
    intt_avx(work.coeffs, prime, intt_param);
    acc ^= work.coeffs[(i * 17) & (RRLWR_N - 1)];
    timestamps[i + 1] = cpucycles();
  }

  return acc;
}

static void run_benchmark(int32_t prime,
                          int32_t primeinv,
                          int32_t finalconst,
                          const int32_t *zetas_ref,
                          const int32_t *intt_fused_param,
                          const char *label)
{
  poly input;
  uint64_t ts_base[N_CYCLE_TIMESTAMPS];
  uint64_t ts_fused[N_CYCLE_TIMESTAMPS];
  uint64_t med_base, med_fused;
  uint64_t avg_base = 0, avg_fused = 0;
  int64_t chk_base, chk_fused;

  fill_random_poly(&input, prime, 2);

  printf("%s benchmark (rdtsc, %d samples each, copy+transform per sample)\n",
         label, NUMBER_OF_CYCLE_SAMPLES);

  chk_base = bench_poly_invntt32_cycles(&input, prime, primeinv, finalconst, zetas_ref,
                                        ts_base, N_CYCLE_TIMESTAMPS);
  med_base = print_results_ex("  poly_invntt32", ts_base, N_CYCLE_TIMESTAMPS, &avg_base);
  printf("  checksum=%lld\n\n", (long long)chk_base);

  chk_fused = bench_intt_avx_cycles(&input, prime, intt_fused_param, ts_fused, N_CYCLE_TIMESTAMPS);
  med_fused = print_results_ex("  intt_avx fused", ts_fused, N_CYCLE_TIMESTAMPS, &avg_fused);
  printf("  checksum=%lld\n\n", (long long)chk_fused);

  if (med_fused > 0) {
    printf("  speedup (median) poly_invntt32 / intt_avx fused: %.4f\n",
           (double)med_base / (double)med_fused);
    printf("  speedup (average) poly_invntt32 / intt_avx fused: %.4f\n",
           (double)avg_base / (double)avg_fused);
  }
}

int main(void)
{
  int fused_bitexact = 1;
  int fused_modequiv = 1;

  if (!run_tests_for_prime(RRLWR_PKE_PRIME,
                           RRLWR_PKE_PRIMEINV,
                           RRLWR_NTTINV_FINALCONST,
                           reference_zetas,
                           RRLWR_KEM_INTT_FUSED_AVX_PARAMS,
                           &fused_bitexact,
                           &fused_modequiv)) {
    return 1;
  }

  if (fused_bitexact) {
    puts("OK: intt_avx fused matches poly_invntt32 bit-exact");
  } else if (fused_modequiv) {
    puts("intt_avx fused is mod-equivalent but not bit-exact");
  } else {
    puts("intt_avx fused is not mod-equivalent");
    return 1;
  }

  run_benchmark(RRLWR_PKE_PRIME,
                RRLWR_PKE_PRIMEINV,
                RRLWR_NTTINV_FINALCONST,
                reference_zetas,
                RRLWR_KEM_INTT_FUSED_AVX_PARAMS,
                "KEM inverse NTT");

  puts("test_invntt_precalcu ok");
  return 0;
}
