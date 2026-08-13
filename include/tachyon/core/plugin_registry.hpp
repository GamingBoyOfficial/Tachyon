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
#include <vector>
#include <memory>
#include <algorithm>

namespace tachyon {

// A static registry for plugins. Users can register their plugins
// before starting the engine. All plugins are called after the core.
template <typename PluginBase>
class PluginRegistry {
public:
    using PluginPtr = std::unique_ptr<PluginBase>;

    // Register a plugin (takes ownership)
    static void add(PluginPtr plugin) {
        getPlugins().push_back(std::move(plugin));
    }

    // Get all registered plugins (read-only)
    static const std::vector<PluginPtr>& getAll() {
        return getPlugins();
    }

private:
    static std::vector<PluginPtr>& getPlugins() {
        static std::vector<PluginPtr> plugins;
        return plugins;
    }
};

// Define registries for our plugin types
using FeedPluginRegistry = PluginRegistry<struct FeedPlugin>;
using FeaturePluginRegistry = PluginRegistry<struct FeaturePlugin>;

} // namespace tachyon