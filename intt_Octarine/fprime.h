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

#ifndef FPRIME_H
#define FPRIME_H

#include "parameters.h"

#ifdef __cplusplus
extern "C"
{
#endif

  int32_t montgomery_mul32(int32_t a, int32_t b, int32_t prime, int32_t primeinv);
  int32_t add32(int32_t a, int32_t b, int32_t prime);
  int32_t sub32(int32_t a, int32_t b, int32_t prime);
  int32_t conditional_reduce32(int32_t r, int32_t prime);
  int32_t conditional_final_reduce32(int32_t r, int32_t prime);

#ifdef __cplusplus
}
#endif

#endif