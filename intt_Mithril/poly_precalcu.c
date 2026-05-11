/*
 * Copyright 2026 NXP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>

#include "poly.h"

static inline int32_t montgomery_mul32_precalcu(int32_t a, int32_t b, int32_t b_pre, int32_t prime)
{
  int64_t prod = (int64_t)a * b;
  uint32_t t_u = (uint32_t)a * (uint32_t)b_pre;
  int32_t t = (int32_t)t_u;
  int64_t s = prod + (int64_t)t * prime;
  return (int32_t)((uint64_t)s >> 32);
}

void poly_invntt32(poly *f,
                   int32_t prime,
                   int32_t primeinv,
                   int32_t finalconst,
                   const int32_t fp_zetas[RRLWR_N])
{
  unsigned int start, len, j, k;
  int32_t t, zeta;
  int32_t *fc = f->coeffs;

  k = RRLWR_N - 1;
  for (len = 1; len <= (RRLWR_N >> 1); len <<= 1) {
    for (start = 0; start < RRLWR_N; start = j + len) {
      zeta = fp_zetas[k--];
      for (j = start; j < start + len; j++) {
        t = fc[j];
        fc[j] = t + fc[j + len];
        fc[j + len] = fc[j + len] - t;
        fc[j + len] = montgomery_mul32(fc[j + len], zeta, prime, primeinv);
        fc[j] = conditional_reduce32(fc[j], prime); // Reduce back to [-p, p]
      }
    }
  }

  for (j = 0; j < RRLWR_N; j++) {
    fc[j] = montgomery_mul32(fc[j], finalconst, prime, primeinv);
  }
}

void poly_invntt32_precalcu_finalinlast(poly *f,
                                        int32_t prime,
                                        int32_t primeinv,
                                        int32_t finalconst,
                                        const int32_t fp_zetas_intt[RRLWR_N],
                                        const int32_t fp_zetas_intt_pre[RRLWR_N])
{
  unsigned int start, len, j, k;
  int32_t t, zeta, zeta_pre;
  int32_t sum, diff, low, high;
  int32_t finalconst_pre = (int32_t)((uint32_t)finalconst * (uint32_t)primeinv);
  int32_t *fc = f->coeffs;

  k = 0;
  for (len = 1; len <= (RRLWR_N >> 1); len <<= 1) {
    for (start = 0; start < RRLWR_N; start = j + len) {
      zeta = fp_zetas_intt[k];
      zeta_pre = fp_zetas_intt_pre[k];
      ++k;
      for (j = start; j < start + len; j++) {
        t = fc[j];
        if (len != (RRLWR_N >> 1)) {
          fc[j] = t + fc[j + len];
          fc[j + len] = fc[j + len] - t;
          fc[j + len] = montgomery_mul32_precalcu(fc[j + len], zeta, zeta_pre, prime);
          fc[j] = conditional_reduce32(fc[j], prime);
        } else {
          sum = t + fc[j + len];
          diff = fc[j + len] - t;
          high = montgomery_mul32_precalcu(diff, zeta, zeta_pre, prime);
          low = conditional_reduce32(sum, prime);
          fc[j] = montgomery_mul32_precalcu(low, finalconst, finalconst_pre, prime);
          fc[j + len] = montgomery_mul32_precalcu(high, finalconst, finalconst_pre, prime);
        }
      }
    }
  }
  assert(k == RRLWR_N - 1);
}

void poly_invntt32_precalcu_finalinlast_fusedzeta(poly *f,
                                                  int32_t prime,
                                                  int32_t primeinv,
                                                  int32_t finalconst,
                                                  const int32_t fp_zetas_intt_fused[RRLWR_N],
                                                  const int32_t fp_zetas_intt_fused_pre[RRLWR_N])
{
  unsigned int start, len, j, k;
  int32_t t, zeta, zeta_pre;
  int32_t sum, diff, low;
  int32_t finalconst_pre = (int32_t)((uint32_t)finalconst * (uint32_t)primeinv);
  int32_t *fc = f->coeffs;

  k = 0;
  for (len = 1; len <= (RRLWR_N >> 1); len <<= 1) {
    for (start = 0; start < RRLWR_N; start = j + len) {
      zeta = fp_zetas_intt_fused[k];
      zeta_pre = fp_zetas_intt_fused_pre[k];
      ++k;
      for (j = start; j < start + len; j++) {
        t = fc[j];
        if (len != (RRLWR_N >> 1)) {
          fc[j] = t + fc[j + len];
          fc[j + len] = fc[j + len] - t;
          fc[j + len] = montgomery_mul32_precalcu(fc[j + len], zeta, zeta_pre, prime);
          fc[j] = conditional_reduce32(fc[j], prime);
        } else {
          sum = t + fc[j + len];
          diff = fc[j + len] - t;
          low = conditional_reduce32(sum, prime);
          fc[j] = montgomery_mul32_precalcu(low, finalconst, finalconst_pre, prime);
          fc[j + len] = montgomery_mul32_precalcu(diff, zeta, zeta_pre, prime);
        }
      }
    }
  }
  assert(k == RRLWR_N - 1);
}
