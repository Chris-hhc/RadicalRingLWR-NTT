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

void poly_basemul32(poly *r, poly *f, poly *g, int32_t prime, int32_t primeinv) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = montgomery_mul32(f->coeffs[i], g->coeffs[i], prime, primeinv);
  }
}

void poly_add32(poly *r, poly *f, poly *g, int32_t prime) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = add32(f->coeffs[i], g->coeffs[i], prime);
  }
}

void poly_add(poly *r, poly *f, poly *g) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = f->coeffs[i] + g->coeffs[i];
  }
}

void poly_sub32(poly *r, poly *f, poly *g, int32_t prime) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = sub32(f->coeffs[i], g->coeffs[i], prime);
  }
}

void poly_sub(poly *r, poly *f, poly *g) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = f->coeffs[i] - g->coeffs[i];
  }
}

/// @brief Reduce the input modulo 2**d into signed interval [-2**d, 2**d-1]
void poly_reduce_pow2(poly *r, poly *f, int32_t d) {
  int32_t pow2div2 = (int32_t)1 << (d-1); // 2^d/2
  int32_t pow2 = (int32_t)1 << d;         // 2^d
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = ((f->coeffs[i] + pow2div2) & (pow2-1)) - pow2div2;
  }
}

/// @brief Reduce the input modulo prime into signed interval [-(prime-1)/2, (prime-1)/2-1]
void poly_conditional_final_reduce32(poly *r, int32_t prime) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = conditional_final_reduce32(r->coeffs[i], prime);
  }
}

void poly_round_xtoy(poly *r, const poly *f, int32_t x, int32_t y) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = f->coeffs[i] + ((int32_t)1 << (x-(y+1))); // Add constant x/(2*y)
    r->coeffs[i] >>= (x-y);                                  // Divide by x/y and floor
    r->coeffs[i] &= ((int32_t)1 << y)-1;                     // Reduce mod y
  }
}

void poly_compress(poly *r, int32_t x) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] >>= x;
  }
}

void poly_decompress(poly *r, int32_t x) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] <<= x;
  }
}
