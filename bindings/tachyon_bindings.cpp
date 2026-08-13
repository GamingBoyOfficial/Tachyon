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
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <tachyon/tachyon.hpp>
#include <cstdint>
#include <cstring>
#include <vector>

namespace py = pybind11;

// Batch processing: many raw packets → feature matrix
py::array_t<float> compute_features_batch(py::bytes data) {
    std::string raw = data;
    const std::byte* ptr = reinterpret_cast<const std::byte*>(raw.data());
    size_t total_len = raw.size();

    // Each packet is exactly 36 bytes (simplified Add‑Order only)
    constexpr size_t PACKET_SIZE = 36;
    size_t num_packets = total_len / PACKET_SIZE;

    // Allocate feature matrix (num_packets x 32)
    std::vector<py::ssize_t> shape = {
        static_cast<py::ssize_t>(num_packets), 32};
    auto result = py::array_t<float>(shape);
    auto buf = result.mutable_unchecked<2>();

    tachyon::FeatureVector fv;
    tachyon::BookSnapshot snap;

    for (size_t i = 0; i < num_packets; ++i) {
        std::span<const std::byte> packet(ptr + i * PACKET_SIZE, PACKET_SIZE);
        tachyon::ItchParser::parse_packet(packet, snap);
        tachyon::FeatureCalculator::compute(snap, fv);
        for (size_t j = 0; j < 32; ++j)
            buf(i, j) = fv.data[j];
    }
    return result;
}

PYBIND11_MODULE(tachyon_core, m) {
    m.doc() = "Tachyon: Ultra‑low‑latency feature engine (batch)";
    m.def("compute_features_batch", &compute_features_batch,
          "Compute 32 features for many 36‑byte packets. Input: raw bytes, output: Nx32 float array");
}