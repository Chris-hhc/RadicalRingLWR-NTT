#define _POSIX_C_SOURCE 200809L

#include <immintrin.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum { N = 256, Q = 8380417 };
enum { QDATA_WORDS = 624 / 8 };

#define CAT2(a, b) a##b
#define CAT(a, b) CAT2(a, b)
#define SYM(prefix, name) CAT(prefix, _##name)

typedef union {
  int32_t coeffs[N];
  __m256i vec[N / 8];
} poly_t;

typedef union {
  int32_t coeffs[624];
  __m256i vec[QDATA_WORDS];
} qdata_t;

extern void SYM(REF_PREFIX, ntt)(int32_t a[N]);
extern void SYM(AVX_PREFIX, ntt_avx)(__m256i *a, const __m256i *qdata);
extern void SYM(AVX_PREFIX, nttunpack_avx)(__m256i *a);
extern const qdata_t SYM(AVX_PREFIX, qdata);

static uint64_t rng_state = 0x4d595df4d0f33173ULL;

static uint64_t rng_next(void) {
  uint64_t x = rng_state;
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  rng_state = x;
  return x * 2685821657736338717ULL;
}

static void fill_poly(int32_t *a) {
  for (int i = 0; i < N; ++i) {
    a[i] = (int32_t)(rng_next() % Q);
  }
}

static uint64_t now_ns(void) {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
    perror("clock_gettime");
    exit(1);
  }
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint64_t checksum_poly(const int32_t *a) {
  uint64_t x = 0;
  for (int i = 0; i < N; ++i) {
    x ^= (uint64_t)(uint32_t)a[i] + 0x9e3779b97f4a7c15ULL * (uint64_t)(i + 1);
    x = (x << 7) | (x >> 57);
  }
  return x;
}

static int compare_once(void) {
  poly_t ref_in, avx_in;
  fill_poly(ref_in.coeffs);
  memcpy(avx_in.coeffs, ref_in.coeffs, sizeof(ref_in.coeffs));

  SYM(REF_PREFIX, ntt)(ref_in.coeffs);
  SYM(AVX_PREFIX, ntt_avx)(avx_in.vec, SYM(AVX_PREFIX, qdata).vec);

  if (memcmp(ref_in.coeffs, avx_in.coeffs, sizeof(ref_in.coeffs)) == 0) {
    return 0;
  }

  poly_t ref_unpack = ref_in;
  poly_t avx_unpack = avx_in;

  SYM(AVX_PREFIX, nttunpack_avx)(ref_unpack.vec);
  SYM(AVX_PREFIX, nttunpack_avx)(avx_unpack.vec);

  if (memcmp(ref_unpack.coeffs, avx_in.coeffs, sizeof(ref_unpack.coeffs)) == 0 ||
      memcmp(ref_in.coeffs, avx_unpack.coeffs, sizeof(avx_unpack.coeffs)) == 0 ||
      memcmp(ref_unpack.coeffs, avx_unpack.coeffs, sizeof(ref_unpack.coeffs)) == 0) {
    return 1;
  }

  fprintf(stderr,
          "mismatch after raw compare and unpack compare: ref=%" PRId32
          " avx2=%" PRId32 "\n",
          ref_in.coeffs[1], avx_in.coeffs[1]);
  for (int k = 0; k < 16; ++k) {
    fprintf(stderr, "[%d] ref=%" PRId32 " avx2=%" PRId32
                    " ref_unpack=%" PRId32 " avx_unpack=%" PRId32 "\n",
            k, ref_in.coeffs[k], avx_in.coeffs[k],
            ref_unpack.coeffs[k], avx_unpack.coeffs[k]);
  }
  return -1;
}

static double bench_ref(const poly_t *template) {
  poly_t work;
  const int rounds = 50000;
  volatile uint64_t sink = 0;
  uint64_t t0 = now_ns();
  for (int i = 0; i < rounds; ++i) {
    memcpy(work.coeffs, template->coeffs, sizeof(work.coeffs));
    SYM(REF_PREFIX, ntt)(work.coeffs);
    sink ^= checksum_poly(work.coeffs);
  }
  uint64_t t1 = now_ns();
  (void)sink;
  return (double)(t1 - t0) / (double)rounds;
}

static double bench_avx(const poly_t *template) {
  poly_t work;
  const int rounds = 50000;
  volatile uint64_t sink = 0;
  uint64_t t0 = now_ns();
  for (int i = 0; i < rounds; ++i) {
    memcpy(work.coeffs, template->coeffs, sizeof(work.coeffs));
    SYM(AVX_PREFIX, ntt_avx)(work.vec, SYM(AVX_PREFIX, qdata).vec);
    sink ^= checksum_poly(work.coeffs);
  }
  uint64_t t1 = now_ns();
  (void)sink;
  return (double)(t1 - t0) / (double)rounds;
}

int main(void) {
  int match_kind = 0;
  for (int i = 0; i < 1000; ++i) {
    int r = compare_once();
    if (r < 0) {
      return 1;
    }
    if (r > match_kind) {
      match_kind = r;
    }
  }

  poly_t template;
  fill_poly(template.coeffs);

  const double ref_ns = bench_ref(&template);
  const double avx_ns = bench_avx(&template);
  const double speedup = ref_ns / avx_ns;

  if (match_kind == 0) {
    printf("correctness: ok (raw layout)\n");
  } else {
    printf("correctness: ok (matches after AVX2 layout normalization)\n");
  }
  printf("benchmark: ref %.1f ns/op, avx2 %.1f ns/op, speedup %.2fx\n",
         ref_ns, avx_ns, speedup);
  return 0;
}
