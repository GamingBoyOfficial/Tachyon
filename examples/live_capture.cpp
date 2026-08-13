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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>
#include <x86intrin.h>

constexpr size_t PACKET_SIZE = 36;

static inline uint64_t rdtsc_serialised() noexcept {
    uint64_t tsc;
    unsigned int aux;
    tsc = __rdtscp(&aux);
    _mm_lfence();
    return tsc;
}

int main(int argc, char** argv) {
    const char* input = (argc > 1) ? argv[1] : "sample.ITCH";

    std::ifstream file(input, std::ios::binary);
    if (!file) {
        fprintf(stderr, "Error: cannot open %s\n", input);
        return 1;
    }
    std::vector<char> file_data((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
    size_t total_bytes = file_data.size();
    size_t num_packets = total_bytes / PACKET_SIZE;
    if (num_packets == 0) {
        fprintf(stderr, "Error: file too small (need at least %u bytes)\n", (unsigned)PACKET_SIZE);
        return 1;
    }

    tachyon::BookSnapshot snap;
    tachyon::FeatureVector fv;

    // Warm‑up: force the CPU to max frequency
    for (size_t i = 0; i < 1000 && i < num_packets; ++i) {
        const char* packet = file_data.data() + i * PACKET_SIZE;
        tachyon::ItchParser::parse_packet(
            std::span<const std::byte>(reinterpret_cast<const std::byte*>(packet), PACKET_SIZE),
            snap);
        tachyon::FeatureCalculator::compute(snap, fv);
    }

    // Volatile sink that consumes ALL 32 features (prevents dead‑code elimination)
    volatile float sink = 0.0f;
    uint64_t start_tsc = rdtsc_serialised();
    for (size_t i = 0; i < num_packets; ++i) {
        const char* packet = file_data.data() + i * PACKET_SIZE;
        tachyon::ItchParser::parse_packet(
            std::span<const std::byte>(reinterpret_cast<const std::byte*>(packet), PACKET_SIZE),
            snap);
        tachyon::FeatureCalculator::compute(snap, fv);

        // Sum every feature into the volatile sink – no feature can be skipped
        float sum = 0.0f;
        for (size_t j = 0; j < 32; ++j) sum += fv.data[j];
        sink += sum;
    }
    uint64_t end_tsc = rdtsc_serialised();
    uint64_t total_cycles = end_tsc - start_tsc;

    double cycles_per_packet = (double)total_cycles / num_packets;
    double ns_per_packet = cycles_per_packet / 3.0;   // nominal 3 GHz

    printf("=== Tachyon Live Capture (file replay, rdtsc) ===\n");
    printf("File           : %s\n", input);
    printf("Packets        : %zu\n", num_packets);
    printf("Total cycles   : %llu\n", (unsigned long long)total_cycles);
    printf("Cycles/packet  : %.1f\n", cycles_per_packet);
    printf("Approx ns/pkt  : %.1f (assuming 3.0 GHz)\n", ns_per_packet);
    printf("Throughput     : %.2f M pkts/s (if 3 GHz)\n", 3.0e9 / cycles_per_packet / 1e6);
    printf("Sink           : %f (ignore)\n", sink);
    return 0;
}