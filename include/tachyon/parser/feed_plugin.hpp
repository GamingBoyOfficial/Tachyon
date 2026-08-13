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
#include <span>

namespace tachyon {

// Interface for custom feed parsers (e.g., different exchange protocols)
struct FeedPlugin {
    virtual ~FeedPlugin() = default;

    // Parse a raw packet into the order book snapshot.
    // Returns true on success, false on malformed data.
    virtual bool parse(std::span<const std::byte> packet,
                       BookSnapshot& book) noexcept = 0;
};

} // namespace tachyon