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
#include <tachyon/data/book_snapshot.hpp>
#include <tachyon/data/feature_vector.hpp>
#include <cstddef>

namespace tachyon {

// Interface for custom feature calculators.
// Users can compute additional features beyond the built-in 32.
struct FeaturePlugin {
    virtual ~FeaturePlugin() = default;

    // Return the number of features this plugin will produce.
    virtual size_t getNumFeatures() const noexcept = 0;

    // Compute custom features and write them into output[start_idx..].
    virtual void compute(const BookSnapshot& book,
                         float* output,
                         size_t start_idx) noexcept = 0;
};

} // namespace tachyon