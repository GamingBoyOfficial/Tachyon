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
#include <atomic>
#include <cstddef>
#include <cstring>
#include "tachyon/core/compiler_macros.hpp"

namespace tachyon {

template <typename T>
class DoubleBuffer {
    alignas(64) T buffers_[2];
    std::atomic<T*> current_;

public:
    DoubleBuffer() noexcept : current_(&buffers_[0]) {
        // value-initialize both buffers (zero for trivial types, safe for non-trivial)
        buffers_[0] = T{};
        buffers_[1] = T{};
    }

    // Get a pointer to the back buffer (for writing)
    TACHYON_FORCE_INLINE T* get_back() noexcept {
        T* front = current_.load(std::memory_order_relaxed);
        return (front == &buffers_[0]) ? &buffers_[1] : &buffers_[0];
    }

    // Publish the back buffer, making it the new front
    TACHYON_FORCE_INLINE void publish(T* back) noexcept {
        current_.store(back, std::memory_order_release);
    }

    // Get a read-only view of the front buffer
    TACHYON_FORCE_INLINE const T* get_front() noexcept {
        return current_.load(std::memory_order_acquire);
    }
};

} // namespace tachyon