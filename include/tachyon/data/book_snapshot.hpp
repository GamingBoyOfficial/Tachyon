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
#include <cstdint>
#include <cstring>
#include <array>
#include <algorithm>
#include "tachyon/core/compiler_macros.hpp"

namespace tachyon {

// Represents a single trade/order event for OFI
struct TradeEvent {
    int8_t side;      // +1 buy, -1 sell, 0 unknown
    uint32_t volume;
};

struct alignas(64) BookSnapshot {
    // Top 10 bid/ask levels (prices in integer format to avoid float precision issues,
    // but we use float for SIMD convenience; real systems use fixed-point)
    float bid_prices[10];
    float bid_volumes[10];
    float ask_prices[10];
    float ask_volumes[10];

    uint64_t timestamp;
    float last_mid_price;   // for tick rule

    // Recent trade events for OFI (circular buffer)
    static constexpr size_t MAX_TRADE_EVENTS = 10;
    TradeEvent recent_trades[MAX_TRADE_EVENTS];
    size_t trade_head = 0;   // next write position

    // --- Order book maintenance (simplistic, O(10) operations) ---

    // Add or update a limit order on the given side.
    // side: +1 for buy (bid), -1 for sell (ask)
    TACHYON_FORCE_INLINE void add_order(float price, uint32_t volume, int side) noexcept {
        if (volume == 0) return;
        float* prices = (side > 0) ? bid_prices : ask_prices;
        float* volumes = (side > 0) ? bid_volumes : ask_volumes;

        // Find existing price or first empty slot (vol == 0)
        int i = 0;
        for (; i < 10; ++i) {
            if (volumes[i] == 0.0f || prices[i] == price) break;
        }
        if (i < 10) {
            prices[i] = price;
            volumes[i] = static_cast<float>(volume);
            // Keep sorted: bid descending, ask ascending
            if (side > 0)
                std::sort(&prices[0], &prices[i+1], std::greater<float>());
            else
                std::sort(&prices[0], &prices[i+1]);
        } // else beyond top 10, ignore
    }

    // Reduce volume (for cancels/executes). Not used in simplified demo but provided.
    TACHYON_FORCE_INLINE void reduce_volume(float price, uint32_t volume, int side) noexcept {
        float* prices = (side > 0) ? bid_prices : ask_prices;
        float* volumes = (side > 0) ? bid_volumes : ask_volumes;
        for (int i = 0; i < 10; ++i) {
            if (prices[i] == price && volumes[i] > 0.0f) {
                volumes[i] -= static_cast<float>(volume);
                if (volumes[i] <= 0.0f) {
                    // Remove level by shifting down
                    for (int j = i; j < 9; ++j) {
                        prices[j] = prices[j+1];
                        volumes[j] = volumes[j+1];
                    }
                    prices[9] = 0.0f;
                    volumes[9] = 0.0f;
                }
                break;
            }
        }
    }

    // Record a trade event for OFI. side: +1 buy, -1 sell.
    TACHYON_FORCE_INLINE void add_trade(int side, uint32_t volume) noexcept {
        recent_trades[trade_head] = {static_cast<int8_t>(side), volume};
        trade_head = (trade_head + 1) % MAX_TRADE_EVENTS;
    }

    // Reset book (for tests)
    void reset() noexcept {
        std::memset(bid_prices, 0, sizeof(bid_prices));
        std::memset(bid_volumes, 0, sizeof(bid_volumes));
        std::memset(ask_prices, 0, sizeof(ask_prices));
        std::memset(ask_volumes, 0, sizeof(ask_volumes));
        timestamp = 0;
        last_mid_price = 0.0f;
        trade_head = 0;
        for (auto& t : recent_trades) t = {0, 0};
    }
};

static_assert(sizeof(BookSnapshot) >= 64, "BookSnapshot should be at least one cache line");

} // namespace tachyon