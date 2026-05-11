#ifndef NTT_H
#define NTT_H

#include <stdint.h>

#include "parameters.h"
#include "ntt_param.h"

#ifdef __cplusplus
extern "C"
{
#endif

  void ntt_avx(int32_t *coeffs,
               int32_t prime,
               const int32_t *ntt_param);

#ifdef __cplusplus
}
#endif

#endif
