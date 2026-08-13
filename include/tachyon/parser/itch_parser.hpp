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
#include <cstddef>
#include <cstdint>
#include <span>
#include <bit>          // std::byteswap (C++23)
#include "tachyon/data/book_snapshot.hpp"
#include "tachyon/core/compiler_macros.hpp"

namespace tachyon {

// Zero-copy NASDAQ ITCH 5.0 parser.
// Only processes Add Order (type 'A') for speed; all other messages
// are ignored without touching the book.  All divisions replaced by
// multiplications with precomputed reciprocals.
class ItchParser {
public:
    static constexpr size_t MIN_PACKET_SIZE = 1; // at least 1 byte to read message type

    TACHYON_FORCE_INLINE __attribute__((hot))
    static bool parse_packet(std::span<const std::byte> buffer,
                             BookSnapshot& snapshot) noexcept {
        // Must have at least a message type byte
        if (TACHYON_UNLIKELY(buffer.size() < MIN_PACKET_SIZE))
            return false;

        const uint8_t msg_type = static_cast<uint8_t>(buffer[0]);

        // Only 'A' (Add Order) is handled on the hot path
        if (TACHYON_UNLIKELY(msg_type != 'A'))
            return true;   // ignore all other message types, not an error

        // Add Order layout (36 bytes total):
        // Offset 0:  Message type (already read)
        //        1:  Stock Locate (2)   – ignored
        //        3:  Tracking Number (2)– ignored
        //        5:  Timestamp (6)      – ignored
        //       11:  Order Ref (8)      – ignored
        //       19:  Buy/Sell Ind (1)   – 'B' or 'S'
        //       20:  Shares (4, big‑endian)
        //       24:  Stock (8)          – ignored
        //       32:  Price (4, big‑endian)
        if (TACHYON_UNLIKELY(buffer.size() < 36))
            return false;

        // Side (byte 19 – 0‑based index)
        const uint8_t side_byte = static_cast<uint8_t>(buffer[19]);
        // Branchless: 'B' -> +1, everything else -> -1
        const int side = (side_byte == 'B') ? 1 : -1;

        // Shares (bytes 20‑23) – convert big‑endian to host via std::byteswap
        uint32_t shares;
        std::memcpy(&shares, &buffer[20], sizeof(shares));
        shares = std::byteswap(shares);            // host is little‑endian

        // Price (bytes 32‑35) – convert and apply reciprocal scaling
        uint32_t raw_price;
        std::memcpy(&raw_price, &buffer[32], sizeof(raw_price));
        raw_price = std::byteswap(raw_price);

        // Precomputed reciprocal: 1.0 / 10000.0
        constexpr float inv_10000 = 0.0001f;
        const float price = static_cast<float>(raw_price) * inv_10000;

        // Apply to order book
        snapshot.add_order(price, shares, side);
        snapshot.add_trade(side, shares);

        // Update last mid price (use best bid/ask from the just‑updated book)
        const float best_bid = snapshot.bid_prices[0];
        const float best_ask = snapshot.ask_prices[0];
        if (best_bid > 0.0f && best_ask > 0.0f) {
            snapshot.last_mid_price = 0.5f * (best_bid + best_ask);
        }
        return true;
    }
};

} // namespace tachyon