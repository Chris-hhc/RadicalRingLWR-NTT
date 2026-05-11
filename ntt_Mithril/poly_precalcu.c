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

#include "poly.h"

static inline int32_t montgomery_mul32_precalcu(int32_t a, int32_t b, int32_t b_pre, int32_t prime)
{
  int64_t prod = (int64_t)a * b;
  uint32_t t_u = (uint32_t)a * (uint32_t)b_pre;
  int32_t t = (int32_t)t_u;
  int64_t s = prod + (int64_t)t * prime;
  return (int32_t)((uint64_t)s >> 32);
}

static void poly_ntt32_precalcu_range(poly *f,
                                      int32_t prime,
                                      const int32_t fp_zetas[RRLWR_N],
                                      const int32_t fp_zetas_pre[RRLWR_N],
                                      unsigned int len_begin,
                                      unsigned int len_end,
                                      unsigned int k_begin,
                                      int do_final_reduce)
{
  unsigned int len, start, j, k;
  int32_t t, fp_zeta, fp_zeta_pre;
  int32_t *fc = f->coeffs;

  k = k_begin;
  for(len = len_begin; len >= len_end; len >>= 1) {
    for(start = 0; start < RRLWR_N; start = j + len) {
      fp_zeta = fp_zetas[k];
      fp_zeta_pre = fp_zetas_pre[k++];
      for(j = start; j < start + len; j++) {
        t = montgomery_mul32_precalcu(fc[j + len], fp_zeta, fp_zeta_pre, prime);
        fc[j] = conditional_reduce32(fc[j], prime); // Reduce back to [-p, p]
        fc[j + len] = fc[j] - t;
        fc[j] = fc[j] + t;
      }
    }
  }

  if (do_final_reduce) {
    for(j = 0; j < RRLWR_N; j++) {
      fc[j] = conditional_reduce32(fc[j], prime); // Reduce back to [-p, p]
    }
  }
}

void poly_ntt32_first4_precalcu_c(poly *f,
                                  int32_t prime,
                                  const int32_t fp_zetas[RRLWR_N],
                                  const int32_t fp_zetas_pre[RRLWR_N])
{
  poly_ntt32_precalcu_range(f, prime, fp_zetas, fp_zetas_pre, RRLWR_N >> 1, RRLWR_N >> 4, 1, 0);
}

void poly_ntt32_top2_precalcu_c(poly *f,
                                int32_t prime,
                                const int32_t fp_zetas[RRLWR_N],
                                const int32_t fp_zetas_pre[RRLWR_N])
{
  poly_ntt32_precalcu_range(f, prime, fp_zetas, fp_zetas_pre, RRLWR_N >> 1, RRLWR_N >> 2, 1, 0);
}

void poly_ntt32_middle2_precalcu_c(poly *f,
                                   int32_t prime,
                                   const int32_t fp_zetas[RRLWR_N],
                                   const int32_t fp_zetas_pre[RRLWR_N])
{
  poly_ntt32_precalcu_range(f, prime, fp_zetas, fp_zetas_pre, RRLWR_N >> 3, RRLWR_N >> 4, 4, 0);
}

void poly_ntt32_bottom6_precalcu_c(poly *f,
                                   int32_t prime,
                                   const int32_t fp_zetas[RRLWR_N],
                                   const int32_t fp_zetas_pre[RRLWR_N])
{
  poly_ntt32_precalcu_range(f, prime, fp_zetas, fp_zetas_pre, RRLWR_N >> 5, 1, 16, 1);
}

void poly_ntt32_precalcu(poly *f, int32_t prime, const int32_t fp_zetas[RRLWR_N], const int32_t fp_zetas_pre[RRLWR_N]) {
  poly_ntt32_precalcu_range(f, prime, fp_zetas, fp_zetas_pre, RRLWR_N >> 1, 1, 1, 1);
}
