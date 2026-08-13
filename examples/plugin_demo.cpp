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
#include <tachyon/tachyon.hpp>
#include <tachyon/features/feature_plugin.hpp>
#include <tachyon/core/plugin_registry.hpp>
#include <cstdio>
#include <memory>

// A simple custom feature: mid-price range (max bid - min ask over 5 levels)
class MidRangePlugin : public tachyon::FeaturePlugin {
public:
    size_t getNumFeatures() const noexcept override { return 1; }

    void compute(const tachyon::BookSnapshot& book,
                 float* output,
                 size_t start_idx) noexcept override {
        float max_bid = book.bid_prices[0];
        float min_ask = book.ask_prices[0];
        for (int i = 1; i < 5; ++i) {
            if (book.bid_volumes[i] > 0.0f && book.bid_prices[i] > max_bid)
                max_bid = book.bid_prices[i];
            if (book.ask_volumes[i] > 0.0f && book.ask_prices[i] < min_ask)
                min_ask = book.ask_prices[i];
        }
        output[start_idx] = max_bid - min_ask;
    }
};

int main() {
    // Register the plugin before any computation
    tachyon::FeaturePluginRegistry::add(
        std::make_unique<MidRangePlugin>());

    // Create a book snapshot with some order book depth
    tachyon::BookSnapshot book;
    book.reset();
    book.add_order(100.0f, 100, 1);   // bid
    book.add_order(101.0f, 200, 1);   // better bid
    book.add_order(102.0f, 50, -1);   // ask
    book.add_order(103.0f, 80, -1);   // higher ask
    book.add_order(99.0f, 150, 1);    // lower bid
    book.add_order(104.0f, 120, -1);  // higher ask

    tachyon::FeatureVector fv;
    fv.reset();

    // Compute all features (built-in 32 + plugin 1)
    tachyon::FeatureCalculator::compute(book, fv);

    // Print the first 33 values (32 built-in + 1 plugin)
    printf("Built-in 32 features + 1 custom plugin feature:\n");
    for (size_t i = 0; i <= 32; ++i) {
        printf("Feature %2zu: %f\n", i, fv.data[i]);
    }

    printf("\nPlugin output (index 32) = %f\n", fv.data[tachyon::FeatureVector::BUILT_IN]);
    return 0;
}