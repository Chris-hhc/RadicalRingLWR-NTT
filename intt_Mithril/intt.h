#ifndef INTT_H
#define INTT_H

#include <stdint.h>

#include "intt_param.h"

#ifdef __cplusplus
extern "C"
{
#endif

  void intt_avx(int32_t *coeffs,
                int32_t prime,
                const int32_t *intt_param);

#ifdef __cplusplus
}
#endif

#endif
