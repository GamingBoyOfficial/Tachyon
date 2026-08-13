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
#include <immintrin.h>
#include <cmath>
#include <cstdint>
#include "tachyon/data/book_snapshot.hpp"
#include "tachyon/data/feature_vector.hpp"
#include "tachyon/core/compiler_macros.hpp"
#include "tachyon/core/platform_detection.hpp"
#include "tachyon/features/feature_plugin.hpp"
#include "tachyon/core/plugin_registry.hpp"

namespace tachyon {

class FeatureCalculator {
public:
    TACHYON_FORCE_INLINE __attribute__((hot))
    static void compute(const BookSnapshot& book,
                        FeatureVector& output) noexcept {
        // Feature indices – all distinct (built-in)
        enum FeatureIdx : size_t {
            EWMA_PRICE = 0,
            OFI = 1,
            REALIZED_VOL = 2,
            TICK_RULE = 3,
            BOOK_PRESSURE = 4,
            SPREAD_TO_VOL = 5,
            MICRO_PRICE = 6,
            DEPTH_DECAY_2_5 = 7,
            BID_ASK_BALANCE = 8,
            WEIGHTED_MID = 9,
            ORDER_BOOK_SLOPE = 10,
            LIQUIDITY_IMBALANCE = 11,
            VOLUME_WEIGHTED_AVG_PRICE = 12,
            TRADE_IMBALANCE = 13,
            CUMULATIVE_DEPTH_BID = 14,
            CUMULATIVE_DEPTH_ASK = 15,
            EFFECTIVE_SPREAD = 16,
            QUOTED_SPREAD = 17,
            LOG_RETURN = 18,
            REALIZED_SKEW = 19,
            REALIZED_KURTOSIS = 20,
            PRICE_IMPACT = 21,
            ORDER_FLOW_TOXICITY = 22,
            VPIN = 23,
            VOLATILITY_OF_VOL = 24,
            MID_PRICE_RANGE = 25,
            BID_ASK_CROSS = 26,
            DEPTH_WEIGHTED_PRICE = 27,
            TICK_SPREAD = 28,
            VOLUME_PROFILE = 29,
            SPREAD_CROSS = 30,
            MICRO_VOL = 31
        };

        // --- Static state for EWMA and Welford (aligned) ---
        alignas(32) static float ewma_old = 0.0f;
        alignas(32) static float welford_mean = 0.0f;
        alignas(32) static float welford_M2 = 0.0f;
        alignas(32) static float welford_M3 = 0.0f;
        alignas(32) static float welford_M4 = 0.0f;
        alignas(32) static float vol_of_vol_mean = 0.0f;
        alignas(32) static float vol_of_vol_M2 = 0.0f;
        static uint64_t welford_n = 0;

        const float best_bid = book.bid_prices[0];
        const float best_ask = book.ask_prices[0];
        const float mid = 0.5f * (best_bid + best_ask);

        // 1. EWMA Price
        constexpr float alpha = 0.1f, beta = 1.0f - alpha;
        const __m256 alpha_vec = _mm256_set1_ps(alpha);
        const __m256 mid_vec   = _mm256_set1_ps(mid);
        const __m256 old_vec   = _mm256_set1_ps(ewma_old);
        const __m256 new_ewma = _mm256_fmadd_ps(
            alpha_vec, mid_vec,
            _mm256_mul_ps(_mm256_set1_ps(beta), old_vec));
        const float ewma_val = _mm256_cvtss_f32(new_ewma);
        output[EWMA_PRICE] = ewma_val;
        ewma_old = ewma_val;

        // 2. OFI
        float buy_vol = 0.0f, sell_vol = 0.0f;
        for (size_t i = 0; i < BookSnapshot::MAX_TRADE_EVENTS; ++i) {
            const auto& ev = book.recent_trades[i];
            if (ev.side > 0)
                buy_vol  += static_cast<float>(ev.volume);
            else if (ev.side < 0)
                sell_vol += static_cast<float>(ev.volume);
        }
        output[OFI] = buy_vol - sell_vol;

        // 3. Volatility
        ++welford_n;
        const float inv_n = 1.0f / static_cast<float>(welford_n);
        const float delta = mid - welford_mean;
        welford_mean += delta * inv_n;
        const float delta2 = mid - welford_mean;
        welford_M2 += delta * delta2;
        float variance = (welford_n > 1) ? welford_M2 / static_cast<float>(welford_n - 1) : 0.0f;
        const __m128 var128 = _mm_set_ss(variance);
        const __m128 vol128 = _mm_sqrt_ss(var128);
        output[REALIZED_VOL] = _mm_cvtss_f32(vol128);

        // 4. Tick Rule (branchless)
        const float diff = mid - book.last_mid_price;
        const int sign = (diff > 0.0f) - (diff < 0.0f);
        output[TICK_RULE] = static_cast<float>(sign);

        // 5. Book Pressure
        float bid_total = 0.0f, ask_total = 0.0f;
        for (int i = 0; i < 10; ++i) {
            bid_total += book.bid_volumes[i];
            ask_total += book.ask_volumes[i];
        }
        const float pressure_total = bid_total + ask_total;
        output[BOOK_PRESSURE] = (pressure_total > 0.0f) ? bid_total / pressure_total : 0.5f;

        // 6. Spread‑to‑Vol
        const float spread = best_ask - best_bid;
        const float vol = output[REALIZED_VOL];
        output[SPREAD_TO_VOL] = (vol > 1e-9f) ? spread / vol : 0.0f;

        // 7. Micro‑Price
        const float bid_vol0 = book.bid_volumes[0];
        const float ask_vol0 = book.ask_volumes[0];
        const float denom = bid_vol0 + ask_vol0;
        output[MICRO_PRICE] = (denom > 0.0f) ? (best_bid * ask_vol0 + best_ask * bid_vol0) / denom : mid;

        // 8. Depth Decay
        const float top_vol = book.bid_volumes[0];
        float deeper_vol = 0.0f;
        for (int i = 1; i < 5 && i < 10; ++i) deeper_vol += book.bid_volumes[i];
        output[DEPTH_DECAY_2_5] = (top_vol > 0.0f) ? deeper_vol / top_vol : 0.0f;

        // 9. Bid‑Ask Balance
        float bid3 = 0.0f, ask3 = 0.0f;
        for (int i = 0; i < 3; ++i) {
            bid3 += book.bid_volumes[i];
            ask3 += book.ask_volumes[i];
        }
        output[BID_ASK_BALANCE] = (bid3 + ask3 > 0.0f) ? (bid3 - ask3) / (bid3 + ask3) : 0.0f;

        // 10. Weighted Mid
        float weight_bid = 0.0f, weight_ask = 0.0f;
        for (int i = 0; i < 3; ++i) {
            const float inv_dist = 1.0f / (1.0f + static_cast<float>(i));
            weight_bid += book.bid_volumes[i] * inv_dist;
            weight_ask += book.ask_volumes[i] * inv_dist;
        }
        output[WEIGHTED_MID] = (weight_bid + weight_ask > 0.0f) ?
                               (best_bid * weight_ask + best_ask * weight_bid) / (weight_bid + weight_ask) : mid;

        // 11. Order Book Slope
        float sum_x = 0.0f, sum_y = 0.0f, sum_xy = 0.0f, sum_x2 = 0.0f;
        int valid_levels = 0;
        for (int i = 0; i < 5; ++i) {
            if (book.bid_volumes[i] <= 0.0f) continue;
            const float y = std::logf(book.bid_prices[i]);
            const float x = static_cast<float>(i);
            sum_x += x;
            sum_y += y;
            sum_xy += x * y;
            sum_x2 += x * x;
            ++valid_levels;
        }
        if (valid_levels >= 2) {
            const float n = static_cast<float>(valid_levels);
            const float slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
            output[ORDER_BOOK_SLOPE] = slope;
        } else {
            output[ORDER_BOOK_SLOPE] = 0.0f;
        }

        // 12. Liquidity Imbalance
        output[LIQUIDITY_IMBALANCE] = (bid_vol0 + ask_vol0 > 0.0f) ?
                                      (bid_vol0 - ask_vol0) / (bid_vol0 + ask_vol0) : 0.0f;

        // 13. VWAP
        float vwap_num = 0.0f, vwap_den = 0.0f;
        for (size_t i = 0; i < BookSnapshot::MAX_TRADE_EVENTS; ++i) {
            const auto& ev = book.recent_trades[i];
            if (ev.volume == 0) continue;
            const float price = mid;
            vwap_num += price * static_cast<float>(ev.volume);
            vwap_den += static_cast<float>(ev.volume);
        }
        output[VOLUME_WEIGHTED_AVG_PRICE] = (vwap_den > 0.0f) ? (vwap_num / vwap_den) : mid;

        // 14. Trade Imbalance
        const float buy_total = buy_vol;
        const float sell_total = sell_vol;
        const float trade_total = buy_total + sell_total;
        output[TRADE_IMBALANCE] = (trade_total > 0.0f) ? buy_total / trade_total : 0.5f;

        // 15. Cumulative Depth Bid
        output[CUMULATIVE_DEPTH_BID] = bid_total;

        // 16. Cumulative Depth Ask
        output[CUMULATIVE_DEPTH_ASK] = ask_total;

        // 17. Effective Spread
        output[EFFECTIVE_SPREAD] = 2.0f * std::fabsf(mid - mid);

        // 18. Quoted Spread
        output[QUOTED_SPREAD] = spread;

        // 19. Log Return
        output[LOG_RETURN] = (book.last_mid_price > 0.0f) ? std::logf(mid / book.last_mid_price) : 0.0f;

        // 20. Realized Skew
        if (welford_n > 2) {
            welford_M3 += (delta * delta * delta) * (1.0f - 2.0f * inv_n);
            const float skew = (welford_n * std::sqrtf(static_cast<float>(welford_n - 1)) * welford_M3) /
                               ((welford_n - 2) * std::powf(welford_M2, 1.5f));
            output[REALIZED_SKEW] = skew;
        } else {
            output[REALIZED_SKEW] = 0.0f;
        }

        // 21. Realized Kurtosis
        if (welford_n > 3) {
            welford_M4 += (delta * delta * delta * delta) * (1.0f - 3.0f * inv_n);
            const float kurt = (static_cast<float>(welford_n * welford_n - 1) * welford_M4) /
                               ((welford_n - 2) * (welford_n - 3) * welford_M2 * welford_M2) - 3.0f;
            output[REALIZED_KURTOSIS] = kurt;
        } else {
            output[REALIZED_KURTOSIS] = 0.0f;
        }

        // 22. Price Impact
        const float vol_total_trade = trade_total;
        output[PRICE_IMPACT] = (vol_total_trade > 0.0f) ? std::fabsf(mid - book.last_mid_price) / vol_total_trade : 0.0f;

        // 23. Order Flow Toxicity
        output[ORDER_FLOW_TOXICITY] = output[OFI] / (vol_total_trade + 1.0f);

        // 24. VPIN
        output[VPIN] = std::fabsf(output[OFI]) / (trade_total + 1.0f);

        // 25. Volatility of Volatility
        {
            const float cur_vol = output[REALIZED_VOL];
            vol_of_vol_mean = 0.95f * vol_of_vol_mean + 0.05f * cur_vol;
            const float vdelta = cur_vol - vol_of_vol_mean;
            vol_of_vol_M2 = 0.95f * vol_of_vol_M2 + 0.05f * vdelta * vdelta;
            output[VOLATILITY_OF_VOL] = std::sqrtf(vol_of_vol_M2);
        }

        // 26. Mid Price Range
        float max_bid = best_bid, min_ask = best_ask;
        for (int i = 1; i < 5; ++i) {
            if (book.bid_volumes[i] > 0.0f && book.bid_prices[i] > max_bid) max_bid = book.bid_prices[i];
            if (book.ask_volumes[i] > 0.0f && book.ask_prices[i] < min_ask) min_ask = book.ask_prices[i];
        }
        output[MID_PRICE_RANGE] = max_bid - min_ask;

        // 27. Bid‑Ask Cross
        output[BID_ASK_CROSS] = (best_bid >= best_ask) ? 1.0f : 0.0f;

        // 28. Depth‑Weighted Price
        float dwp_num = 0.0f, dwp_den = 0.0f;
        for (int i = 0; i < 5; ++i) {
            dwp_num += book.bid_prices[i] * book.bid_volumes[i] + book.ask_prices[i] * book.ask_volumes[i];
            dwp_den += book.bid_volumes[i] + book.ask_volumes[i];
        }
        output[DEPTH_WEIGHTED_PRICE] = (dwp_den > 0.0f) ? dwp_num / dwp_den : mid;

        // 29. Tick Spread
        output[TICK_SPREAD] = spread * 100.0f;

        // 30. Volume Profile
        output[VOLUME_PROFILE] = (bid_vol0 + ask_vol0 > 0.0f) ? bid_vol0 / (bid_vol0 + ask_vol0) : 0.5f;

        // 31. Spread Cross
        output[SPREAD_CROSS] = (mid > 0.0f) ? (spread / mid) : 0.0f;

        // 32. Micro Volatility
        output[MICRO_VOL] = std::fabsf(mid - book.last_mid_price);

        // ─── Plugin execution (additional features) ─────────────────────
        // Write into slots after the built-in features.
        const auto& plugins = FeaturePluginRegistry::getAll();
        if (!plugins.empty()) {
            size_t idx = FeatureVector::BUILT_IN;
            float* out_ptr = output.data.data();
            for (const auto& plugin : plugins) {
                const size_t n = plugin->getNumFeatures();
                if (idx + n > FeatureVector::SIZE) {
                    // Not enough space – stop processing further plugins.
                    break;
                }
                plugin->compute(book, out_ptr, idx);
                idx += n;
            }
        }
    }
};

} // namespace tachyon