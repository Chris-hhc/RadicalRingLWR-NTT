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

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  #ifndef RRLWR_SECURITY_LEVEL
  #define RRLWR_SECURITY_LEVEL 128
  #endif

  // Algorithm parameters (should not be changed)
  #if (RRLWR_SECURITY_LEVEL == 128)
    #define RRLWR_N                   (128)
    #define RRLWR_K                   (5)
    #define RRLWR_PKE_ELL             (1)
    #define RRLWR_PKE_LOGQ            (13)
    #define RRLWR_PKE_LOGP            (11)
    #define RRLWR_PKE_LOGT            (3)
    #define RRLWR_PKE_LOG_ETA         (1)
  #elif (RRLWR_SECURITY_LEVEL == 256)
    #define RRLWR_N                   (128)
    #define RRLWR_K                   (9)
    #define RRLWR_PKE_ELL             (2)
    #define RRLWR_PKE_LOGQ            (13)
    #define RRLWR_PKE_LOGP            (11)
    #define RRLWR_PKE_LOGT            (3)
    #define RRLWR_PKE_LOG_ETA         (1)
  #elif (RRLWR_SECURITY_LEVEL == 512)
    #define RRLWR_N                   (128)
    #define RRLWR_K                   (17)
    #define RRLWR_PKE_ELL             (4)
    #define RRLWR_PKE_LOGQ            (13)
    #define RRLWR_PKE_LOGP            (11)
    #define RRLWR_PKE_LOGT            (8)
    #define RRLWR_PKE_LOG_ETA         (1)
  #endif

  #define RRLWR_PKE_SEED_A_LEN        (64)
  #define RRLWR_SEED_S_LEN            (64)
  #define RRLWR_PKE_P                 ((int32_t)1 << RRLWR_PKE_LOGP)
  #define RRLWR_PKE_PACKED_POLYQ_LEN  (RRLWR_PKE_LOGQ*RRLWR_N/8)
  #define RRLWR_PKE_PACKED_POLYP_LEN  (RRLWR_PKE_LOGP*RRLWR_N/8)
  #define RRLWR_PKE_PACKED_POLYT_LEN  (RRLWR_PKE_LOGT*RRLWR_N/8)
  #define RRLWR_PKE_PACKED_POLY1_LEN  (RRLWR_N/8)
  #define RRLWR_PACKED_POLY2_LEN      (2*RRLWR_N/8)
  #define RRLWR_PACKED_POLY11_LEN     (11*RRLWR_N/8)

  #define RRLWR_PKE_MESSAGE_LEN       (RRLWR_PKE_ELL*RRLWR_N/8)
  #define RRLWR_PKE_SK_LEN            (RRLWR_K*RRLWR_PACKED_POLY2_LEN)
  #define RRLWR_PKE_PK_LEN            (RRLWR_PKE_SEED_A_LEN + RRLWR_K*RRLWR_PKE_PACKED_POLYP_LEN)
  #define RRLWR_PKE_CT_LEN            (RRLWR_PKE_ELL*RRLWR_PKE_PACKED_POLYT_LEN + RRLWR_K*RRLWR_PKE_PACKED_POLYP_LEN)

  #define RRLWR_KEM_SEED_Z_LEN        (RRLWR_PKE_MESSAGE_LEN)
  #define RRLWR_KEM_HPK_LEN           (RRLWR_PKE_MESSAGE_LEN)
  #define RRLWR_KEM_SS_LEN            (RRLWR_PKE_MESSAGE_LEN)
  #define RRLWR_KEM_SK_LEN            (RRLWR_PKE_SK_LEN + RRLWR_PKE_PK_LEN + RRLWR_KEM_HPK_LEN + RRLWR_KEM_SEED_Z_LEN)
  #define RRLWR_KEM_PK_LEN            (RRLWR_PKE_PK_LEN)
  #define RRLWR_KEM_CT_LEN            (RRLWR_PKE_CT_LEN)

  // Define hash functions
  #define RRLWR_XOF(output, output_len, input, input_len) \
          pseudoXOF(8*(output_len), input, 8*(input_len), output)
  #define RRLWR_KEM_HASH_F(output, input, input_len) \
          pseudoXOF(8*(RRLWR_KEM_HPK_LEN), input, 8*(input_len), output)
  #define RRLWR_KEM_HASH_G(output, output_len, input, input_len) \
          pseudoXOF(8*(output_len), input, 8*(input_len), output)
  #define RRLWR_KEM_HASH_H(output, output_len, input, input_len) \
          pseudoXOF(8*(output_len), input, 8*(input_len), output)
  #define GENERATE_RANDOM_BYTES(output, output_len, ctx) \
          get_random_number(ctx, output, 8*(output_len))

  // The choice of prime for the arithmetic does not affect the algorithm, only the implementation.
  // The prime must be selected so that it is greater than the max coefficient of A*s, which lies in [-2*q*n*k, q*n*k].
  // This means log(prime) > log(3*2**13*128*17+1) = 25.67 to work for all parameter sets.
  #define RRLWR_PKE_PRIME             (0x3fff7801)
  #define RRLWR_PKE_PRIMEINV          (0xf7bf77ff) // -prime^-1 mod R
  #define RRLWR_KEM_RMODPRIME         (0x21ffc)    // R mod prime
  #define RRLWR_KEM_2RMODPRIME        (0x43ff8)    // 2*R mod prime
  #define RRLWR_NTTINV_FINALCONST     (0x107ef00) // R^2 / N
  // Roots of unity for the NTT stored in Montgomery domain
  #define RRLWR_KEM_ZETAS             {139260, 183021898, 575231866, 154617429, 414203114, 503673913, 896001265, 700776062, 536310640, 494782856, 334438326, 180725891, 66126242, 527676933, 68124283, 682341145, 198888666, 245510201, 824859155, 652214955, 475321564, 708190408, 795232325, 865099987, 758019085, 80115117, 495065332, 731677357, 534310702, 513677694, 450379788, 934003029, 84040604, 483123884, 408449576, 144227205, 228019302, 263062669, 554647014, 291180702, 257603396, 517156986, 845307688, 931235835, 384153767, 1005013559, 770071216, 488129715, 751025820, 480980915, 572256282, 941016023, 36883699, 308725850, 520483573, 1070144941, 795623980, 773432496, 798527482, 1019237886, 876954497, 838963457, 1037861729, 938217426, 403626681, 1007057068, 133347500, 773181451, 955922949, 726370058, 623698, 390271479, 39533583, 976772690, 900767575, 783547277, 199085505, 2139487, 543358193, 519342977, 93448818, 239769510, 72723441, 772603578, 76403209, 845615946, 94518117, 951507248, 1050433574, 925117629, 225687012, 938486696, 227564537, 1037554145, 324057322, 823310288, 475630802, 363673374, 385520805, 855333729, 198079039, 534689734, 559056607, 704165401, 266791040, 1036192333, 897792184, 506315165, 603631074, 204693219, 247689680, 499425879, 215777983, 903985655, 132714876, 106044468, 194802492, 44794590, 329822404, 316757988, 518538236, 1057930916, 572686735, 215713576, 690470708, 230276683, 753154922, 891299266}

  #define PRECOMPUTE_TWIST

  #ifdef PRECOMPUTE_TWIST
    extern int32_t precomputed_twist[RRLWR_N];
  #endif

#ifdef __cplusplus
}
#endif

#endif