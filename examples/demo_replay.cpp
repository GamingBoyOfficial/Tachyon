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
#include <iostream>
#include <vector>
#include <cstring>

int main() {
    // Demonstration: simulate a packet stream and print computed features.
    tachyon::DoubleBuffer<tachyon::BookSnapshot> double_buf;
    tachyon::SPSCRingBuffer<tachyon::FeatureVector, 16> ring;

    // Pre-built packet
    std::vector<std::byte> pkt(36, std::byte{0});
    pkt[0] = std::byte{'A'};
    pkt[19] = std::byte{'B'}; // Buy
    pkt[23] = std::byte{200}; // shares 200
    std::memcpy(&pkt[24], "MSFT   ", 8);
    uint32_t price = 2500000; // 250.00
    pkt[32] = static_cast<std::byte>((price >> 24) & 0xFF);
    pkt[33] = static_cast<std::byte>((price >> 16) & 0xFF);
    pkt[34] = static_cast<std::byte>((price >> 8) & 0xFF);
    pkt[35] = static_cast<std::byte>(price & 0xFF);

    for (int i = 0; i < 20; ++i) {
        tachyon::BookSnapshot* back = double_buf.get_back();
        tachyon::ItchParser::parse_packet(pkt, *back);
        tachyon::FeatureVector fv;
        tachyon::FeatureCalculator::compute(*back, fv);
        ring.push(fv);
        double_buf.publish(back);

        tachyon::FeatureVector out;
        if (ring.pop(out)) {
            std::cout << "Tick " << i << ": EWMA=" << out[0]
                      << " OFI=" << out[1]
                      << " Vol=" << out[2]
                      << " Pressure=" << out[4]
                      << std::endl;
        }
    }
    return 0;
}