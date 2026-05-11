#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cpucycles.h"
#include "speed_print.h"
#include "intt.h"
#include "poly.h"

enum {
  RANDOM_CASES = 10000,
  WARMUP_ITERS = 1000,
  NUMBER_OF_CYCLE_SAMPLES = 100000,
};

#define N_CYCLE_TIMESTAMPS (NUMBER_OF_CYCLE_SAMPLES + 1)

static int32_t rrlwr_zetas1[RRLWR_N] = RRLWR_SIGN_ZETAS1;
static int32_t rrlwr_zetas2[RRLWR_N] = RRLWR_SIGN_ZETAS2;
static uint64_t rng_state = 0x2718281828459045ULL;

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

static void fill_random(poly *p, int32_t prime)
{
  int64_t bound = (int64_t)2 * prime;
  uint64_t span = (uint64_t)(2 * bound + 1);
  for (unsigned int i = 0; i < RRLWR_N; ++i) {
    p->coeffs[i] = (int32_t)((int64_t)(rng_u64() % span) - bound);
  }
}

static int32_t diff_mod_prime(int32_t a, int32_t b, int32_t prime)
{
  int64_t r = ((int64_t)a - b) % prime;
  if (r < 0) {
    r += prime;
  }
  return (int32_t)r;
}

static void checksum_add(const poly *p, int64_t *checksum)
{
  for (unsigned int i = 0; i < RRLWR_N; ++i) {
    *checksum += p->coeffs[i];
  }
}

static int compare_mod_equiv(const char *label,
                             int case_id,
                             const poly *input,
                             const poly *ref,
                             const poly *avx,
                             int32_t prime)
{
  (void)input;
  for (unsigned int i = 0; i < RRLWR_N; ++i) {
    int32_t dmod = diff_mod_prime(ref->coeffs[i], avx->coeffs[i], prime);
    if (dmod != 0) {
      fprintf(stderr,
              "%s fused mismatch case=%d coeff=%u ref=%d avx=%d diff=%lld diff_mod=%d\n",
              label, case_id, i, ref->coeffs[i], avx->coeffs[i],
              (long long)((int64_t)ref->coeffs[i] - avx->coeffs[i]), dmod);
      return 0;
    }
  }
  return 1;
}

static int correctness_one(const char *label,
                           int32_t prime,
                           int32_t primeinv,
                           int32_t finalconst,
                           int32_t *zetas_ref,
                           const int32_t *intt_param)
{
  poly input, ref, avx;
  for (int tc = 0; tc < RANDOM_CASES; ++tc) {
    fill_random(&input, prime);
    copy_poly(&ref, &input);
    copy_poly(&avx, &input);
    poly_invntt32(&ref, prime, primeinv, finalconst, zetas_ref);
    intt_avx(avx.coeffs, prime, intt_param);
    if (!compare_mod_equiv(label, tc, &input, &ref, &avx, prime)) {
      return 0;
    }
  }
  return 1;
}

static void bench_poly_invntt32_cycles(const poly *input,
                                          int32_t prime,
                                          int32_t primeinv,
                                          int32_t finalconst,
                                          int32_t *zetas_ref,
                                          uint64_t *timestamps,
                                          size_t n_timestamps,
                                          int64_t *checksum)
{
  poly work;
  int64_t acc = 0;
  size_t i;
  size_t iters;

  if (n_timestamps < 2) {
    *checksum = 0;
    return;
  }

  iters = n_timestamps - 1;
  *checksum = 0;

  for (i = 0; i < WARMUP_ITERS; i++) {
    copy_poly(&work, input);
    poly_invntt32(&work, prime, primeinv, finalconst, zetas_ref);
    checksum_add(&work, &acc);
  }

  timestamps[0] = cpucycles();
  for (i = 0; i < iters; i++) {
    copy_poly(&work, input);
    poly_invntt32(&work, prime, primeinv, finalconst, zetas_ref);
    checksum_add(&work, &acc);
    timestamps[i + 1] = cpucycles();
  }

  *checksum = acc;
}

static void bench_intt_avx_cycles(const poly *input,
                                     int32_t prime,
                                     const int32_t *intt_param,
                                     uint64_t *timestamps,
                                     size_t n_timestamps,
                                     int64_t *checksum)
{
  poly work;
  int64_t acc = 0;
  size_t i;
  size_t iters;

  if (n_timestamps < 2) {
    *checksum = 0;
    return;
  }

  iters = n_timestamps - 1;
  *checksum = 0;

  for (i = 0; i < WARMUP_ITERS; i++) {
    copy_poly(&work, input);
    intt_avx(work.coeffs, prime, intt_param);
    checksum_add(&work, &acc);
  }

  timestamps[0] = cpucycles();
  for (i = 0; i < iters; i++) {
    copy_poly(&work, input);
    intt_avx(work.coeffs, prime, intt_param);
    checksum_add(&work, &acc);
    timestamps[i + 1] = cpucycles();
  }

  *checksum = acc;
}

static int bench_one(const char *label,
                     int32_t prime,
                     int32_t primeinv,
                     int32_t finalconst,
                     int32_t *zetas_ref,
                     const int32_t *intt_param)
{
  poly input;
  uint64_t ts_ref[N_CYCLE_TIMESTAMPS];
  uint64_t ts_avx[N_CYCLE_TIMESTAMPS];
  uint64_t med_ref, med_avx;
  uint64_t avg_ref = 0, avg_avx = 0;
  int64_t chk_ref, chk_avx;

  fill_random(&input, prime);

  printf("%s\n", label);
  printf("  correctness: OK, %d random cases (fused intt_avx vs poly_invntt32)\n", RANDOM_CASES);
  printf("  benchmark (rdtsc, %d samples each, copy+transform per sample)\n",
         NUMBER_OF_CYCLE_SAMPLES);

  bench_poly_invntt32_cycles(&input, prime, primeinv, finalconst, zetas_ref, ts_ref,
                             N_CYCLE_TIMESTAMPS, &chk_ref);
  med_ref = print_results_ex("  poly_invntt32", ts_ref, N_CYCLE_TIMESTAMPS, &avg_ref);
  printf("  checksum=%lld\n\n", (long long)chk_ref);

  bench_intt_avx_cycles(&input, prime, intt_param, ts_avx, N_CYCLE_TIMESTAMPS, &chk_avx);
  med_avx = print_results_ex("  intt_avx fused", ts_avx, N_CYCLE_TIMESTAMPS, &avg_avx);
  printf("  checksum=%lld\n\n", (long long)chk_avx);

  if (med_avx > 0) {
    printf("  speedup (median) poly_invntt32 / intt_avx: %.4f\n",
           (double)med_ref / (double)med_avx);
    printf("  speedup (average) poly_invntt32 / intt_avx: %.4f\n",
           (double)avg_ref / (double)avg_avx);
  }
  return 1;
}

int main(void)
{
  if (!correctness_one("prime1", RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, RRLWR_SIGN_NTTINV_FINALCONST1,
                       rrlwr_zetas1, RRLWR_SIGN_INTT_FUSED_AVX_PARAMS1)) {
    return 1;
  }
  if (!correctness_one("prime2", RRLWR_SIGN_PRIME2, RRLWR_SIGN_PRIME2INV, RRLWR_SIGN_NTTINV_FINALCONST2,
                       rrlwr_zetas2, RRLWR_SIGN_INTT_FUSED_AVX_PARAMS2)) {
    return 1;
  }
  puts("OK: fused intt_avx is mod-equivalent to poly_invntt32 for 10000 random cases per prime");

  if (!bench_one("prime1", RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, RRLWR_SIGN_NTTINV_FINALCONST1,
                 rrlwr_zetas1, RRLWR_SIGN_INTT_FUSED_AVX_PARAMS1)) {
    return 1;
  }
  if (!bench_one("prime2", RRLWR_SIGN_PRIME2, RRLWR_SIGN_PRIME2INV, RRLWR_SIGN_NTTINV_FINALCONST2,
                 rrlwr_zetas2, RRLWR_SIGN_INTT_FUSED_AVX_PARAMS2)) {
    return 1;
  }
  return 0;
}
