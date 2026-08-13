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

// Force inline across translation units
#if defined(__GNUC__) || defined(__clang__)
  #define TACHYON_FORCE_INLINE __attribute__((always_inline)) inline
  #define TACHYON_NOINLINE __attribute__((noinline))
  #define TACHYON_LIKELY(x)   __builtin_expect(!!(x), 1)
  #define TACHYON_UNLIKELY(x) __builtin_expect(!!(x), 0)
  #define TACHYON_RESTRICT __restrict__
#elif defined(_MSC_VER)
  #define TACHYON_FORCE_INLINE __forceinline
  #define TACHYON_NOINLINE __declspec(noinline)
  #define TACHYON_LIKELY(x)   (x)
  #define TACHYON_UNLIKELY(x) (x)
  #define TACHYON_RESTRICT __restrict
#else
  #define TACHYON_FORCE_INLINE inline
  #define TACHYON_NOINLINE
  #define TACHYON_LIKELY(x)   (x)
  #define TACHYON_UNLIKELY(x) (x)
  #define TACHYON_RESTRICT
#endif