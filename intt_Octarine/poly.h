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

#ifndef POLY_H
#define POLY_H

#include "fprime.h"

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct{
    int32_t coeffs[RRLWR_N];
  } poly;

  void poly_invntt32(poly *f, int32_t prime, int32_t primeinv, int32_t finalconst, int32_t fp_zetas[RRLWR_N]);
  void poly_basemul32(poly *r, poly *f, poly *g, int32_t prime, int32_t primeinv);
  void poly_add32(poly *r, poly *f, poly *g, int32_t prime);
  void poly_add(poly *r, poly *f, poly *g);
  void poly_sub32(poly *r, poly *f, poly *g, int32_t prime);
  void poly_sub(poly *r, poly *f, poly *g);
  void poly_reduce_pow2(poly *r, poly *f, int32_t d);
  void poly_conditional_final_reduce32(poly *r, int32_t prime);
  void poly_round_xtoy(poly *r, const poly *f, int32_t x, int32_t y);
  void poly_compress(poly *r, int32_t x);
  void poly_decompress(poly *r, int32_t x);

#ifdef __cplusplus
}
#endif

#endif
