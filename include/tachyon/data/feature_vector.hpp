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
#include <array>
#include <cstddef>

namespace tachyon {

struct alignas(64) FeatureVector {
    // 64 features = 256 bytes (4 cache lines). First 32 built-in, rest for plugins.
    std::array<float, 64> data;

    static constexpr size_t SIZE = 64;          // total capacity
    static constexpr size_t BUILT_IN = 32;      // built-in features

    constexpr float& operator[](size_t i) noexcept { return data[i]; }
    constexpr const float& operator[](size_t i) const noexcept { return data[i]; }

    void reset() noexcept { data.fill(0.0f); }
};

static_assert(sizeof(FeatureVector) == 256, "FeatureVector must be exactly 256 bytes");

} // namespace tachyon