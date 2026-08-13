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
#include <cassert>
#include <cstring>
#include <iostream>
#include <array>
#include <tachyon/tachyon.hpp>

void test_feature_vector() {
    tachyon::FeatureVector fv;
    fv.reset();
    assert(fv[0] == 0.0f);
    fv[0] = 1.5f;
    assert(fv[0] == 1.5f);
    std::cout << "FeatureVector test passed\n";
}

void test_book_snapshot_add_order() {
    tachyon::BookSnapshot book;
    book.reset();
    book.add_order(150.0f, 100, 1);
    assert(book.bid_prices[0] == 150.0f);
    assert(book.bid_volumes[0] == 100.0f);
    book.add_order(151.0f, 200, 1);
    assert(book.bid_prices[0] == 151.0f);
    book.add_order(152.0f, 50, -1);
    assert(book.ask_prices[0] == 152.0f);
    std::cout << "BookSnapshot add_order test passed\n";
}

void test_parser_valid_packet() {
    std::array<std::byte, 36> pkt{};
    pkt[0] = std::byte{'A'};
    pkt[19] = std::byte{'B'};
    // shares = 100 big‑endian
    pkt[20] = std::byte{0}; pkt[21] = std::byte{0}; pkt[22] = std::byte{0}; pkt[23] = std::byte{100};
    std::memcpy(&pkt[24], "AAPL   ", 8);
    uint32_t price = 1502500;
    pkt[32] = static_cast<std::byte>((price >> 24) & 0xFF);
    pkt[33] = static_cast<std::byte>((price >> 16) & 0xFF);
    pkt[34] = static_cast<std::byte>((price >> 8) & 0xFF);
    pkt[35] = static_cast<std::byte>(price & 0xFF);

    tachyon::BookSnapshot snap;
    snap.reset();
    bool ok = tachyon::ItchParser::parse_packet(pkt, snap);
    assert(ok);
    assert(snap.bid_prices[0] == 150.25f);
    assert(snap.bid_volumes[0] == 100.0f);
    std::cout << "Parser valid packet test passed\n";
}

void test_parser_short_packet() {
    std::array<std::byte, 1> short_pkt{std::byte{0}};
    tachyon::BookSnapshot snap;
    bool ok = tachyon::ItchParser::parse_packet(short_pkt, snap);
    assert(!ok);
    std::cout << "Parser short packet test passed\n";
}

void test_calculator_basic() {
    tachyon::BookSnapshot snap;
    snap.reset();
    snap.add_order(100.0f, 1000, 1);
    snap.add_order(101.0f, 800, -1);
    snap.last_mid_price = 100.5f;
    for (int i = 0; i < 10; ++i) snap.add_trade(1, 100);

    tachyon::FeatureVector fv;
    tachyon::FeatureCalculator::compute(snap, fv);
    assert(fv[0] != 0.0f);  // EWMA updated
    std::cout << "Calculator basic test passed\n";
}

void test_spsc_ring() {
    tachyon::SPSCRingBuffer<int, 4> ring;
    assert(ring.push(42));
    int out = 0;
    assert(ring.pop(out));
    assert(out == 42);
    assert(!ring.pop(out));
    std::cout << "SPSC ring test passed\n";
}

void test_double_buffer() {
    tachyon::DoubleBuffer<int> db;
    int* back = db.get_back();
    *back = 10;
    db.publish(back);
    assert(*db.get_front() == 10);
    back = db.get_back();
    *back = 20;
    db.publish(back);
    assert(*db.get_front() == 20);
    std::cout << "DoubleBuffer test passed\n";
}

int main() {
    test_feature_vector();
    test_book_snapshot_add_order();
    test_parser_valid_packet();
    test_parser_short_packet();
    test_calculator_basic();
    test_spsc_ring();
    test_double_buffer();
    std::cout << "All unit tests passed.\n";
    return 0;
}