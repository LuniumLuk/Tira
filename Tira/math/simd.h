//
// Created by Ziyi.Lu 2022/02/22
//

#ifndef SIMD_H
#define SIMD_H

#include <xmmintrin.h>

namespace tira {

    inline float _access_m128f(__m128 const* m, size_t idx) {
        return ((float*)m)[idx];
    }

} // namespace tira

#endif
