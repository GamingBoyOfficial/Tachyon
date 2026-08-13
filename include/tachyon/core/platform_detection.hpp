/*
 * Copyright (C) 2026 Parikshit Sharma (GamingBoyOfficial)
 * This file is part of Tachyon.
 *
 * Tachyon is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Tachyon is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Tachyon.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

// Detect x86-64 and SIMD capabilities
#if defined(__x86_64__) || defined(_M_X64)
  #define TACHYON_ARCH_X86_64 1
  #if defined(__AVX2__) || (defined(__GNUC__) && __AVX2__)
    #define TACHYON_AVX2 1
  #endif
  #if defined(__FMA__) || (defined(__GNUC__) && __FMA__)
    #define TACHYON_FMA 1
  #endif
  #define TACHYON_SSE 1
#else
  #error "Tachyon currently only supports x86-64 with AVX2+FMA"
#endif

#if !defined(TACHYON_AVX2) || !defined(TACHYON_FMA)
  #error "Tachyon requires AVX2 and FMA support. Compile with -mavx2 -mfma."
#endif

// Include intrinsics for the detected platform
#include <immintrin.h>