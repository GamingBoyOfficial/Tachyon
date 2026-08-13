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
#include <array>
#include <cstdint>
#include <cstring>
#include <x86intrin.h>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <thread>
#include <chrono>

// CPU frequency detection
static double g_cpu_ghz = 3.0;

static void calibrate_cpu_frequency() noexcept {
    using namespace std::chrono;
    constexpr auto sleep_dur = milliseconds(100);
    uint64_t start_tsc = __rdtsc();
    auto start_wall = high_resolution_clock::now();
    std::this_thread::sleep_for(sleep_dur);
    auto end_wall = high_resolution_clock::now();
    uint64_t end_tsc = __rdtsc();
    double wall_sec = duration<double>(end_wall - start_wall).count();
    if (wall_sec > 0.0) {
        double tsc_per_sec = static_cast<double>(end_tsc - start_tsc) / wall_sec;
        g_cpu_ghz = tsc_per_sec / 1e9;
    }
}

// Serialised TSC
static uint64_t rdtsc_serialised() noexcept {
    uint64_t tsc;
    unsigned int aux;
    tsc = __rdtscp(&aux);
    _mm_lfence();
    return tsc;
}

// Test packet – no constexpr string_view to avoid GCC 16 assertion
static std::array<std::byte, 36> make_test_packet() {
    std::array<std::byte, 36> pkt{};
    pkt[0]  = std::byte{'A'};
    pkt[19] = std::byte{'B'};
    pkt[23] = std::byte{100};
    const char stock[] = "AAPL   ";
    for (int i = 0; i < 8; ++i) pkt[24+i] = std::byte(stock[i]);
    const uint32_t raw_price = 1502500;
    pkt[32] = std::byte((raw_price >> 24) & 0xFF);
    pkt[33] = std::byte((raw_price >> 16) & 0xFF);
    pkt[34] = std::byte((raw_price >> 8) & 0xFF);
    pkt[35] = std::byte(raw_price & 0xFF);
    return pkt;
}
static const std::array<std::byte, 36> kTestPacket = make_test_packet();

int main() {
    calibrate_cpu_frequency();
    printf("Detected CPU frequency: %.2f GHz\n", g_cpu_ghz);

    tachyon::DoubleBuffer<tachyon::BookSnapshot> double_buf;
    tachyon::SPSCRingBuffer<tachyon::FeatureVector, 1024> ring;
    tachyon::FeatureVector fv;

    // Warm up
    for (int i = 0; i < 1000; ++i) {
        tachyon::BookSnapshot* back = double_buf.get_back();
        tachyon::ItchParser::parse_packet(kTestPacket, *back);
        tachyon::FeatureCalculator::compute(*back, fv);
        ring.push(fv);
        double_buf.publish(back);
        back = double_buf.get_back();
        tachyon::FeatureVector tmp; ring.pop(tmp);
    }

    constexpr size_t ITER = 1'000'000;
    std::vector<uint64_t> cycles;
    cycles.reserve(ITER);

    for (size_t i = 0; i < ITER; ++i) {
        const uint64_t start = rdtsc_serialised();
        tachyon::BookSnapshot* back = double_buf.get_back();
        tachyon::ItchParser::parse_packet(kTestPacket, *back);
        tachyon::FeatureCalculator::compute(*back, fv);
        ring.push(fv);
        double_buf.publish(back);
        const uint64_t end = rdtsc_serialised();
        cycles.push_back(end - start);

        tachyon::FeatureVector tmp; ring.pop(tmp);
    }

    std::sort(cycles.begin(), cycles.end());
    double avg = 0.0;
    for (auto c : cycles) avg += c;
    avg /= cycles.size();

    size_t i50 = cycles.size() * 50 / 100;
    size_t i99 = cycles.size() * 99 / 100;
    size_t i999 = cycles.size() * 999 / 1000;

    printf("\n=== Critical Path Latency (pure rdtsc) ===\n");
    printf("P50  : %5llu cycles  (%.2f ns)\n", (unsigned long long)cycles[i50], cycles[i50] / g_cpu_ghz);
    printf("P99  : %5llu cycles  (%.2f ns)\n", (unsigned long long)cycles[i99], cycles[i99] / g_cpu_ghz);
    printf("P999 : %5llu cycles  (%.2f ns)\n", (unsigned long long)cycles[i999], cycles[i999] / g_cpu_ghz);
    printf("Avg  : %5.1f cycles  (%.2f ns)\n", avg, avg / g_cpu_ghz);
    return 0;
}