#include <ranges>
#include <algorithm>
#include <thread>
#include <chrono>
#include "gtest/gtest.h"

#include "ipv4.h"

TEST(IPv4, Checksum) {
    std::array<unsigned char, 24> buffer{0x45, 0x00, 0x00, 0x73, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11, 0x00, 0x00, 0xc0, 0xa8, 0x00, 0x01, 0xc0, 0xa8, 0x00, 0xc7, 0x01, 0x02, 0x04, 0x08};
    ipv4::Packet packet{std::as_writable_bytes(std::span{buffer})};

    EXPECT_EQ(packet.calculate_checksum(), 0xb861);
    packet.set<"checksum">(0xb861);
    EXPECT_EQ(packet.calculate_checksum(), 0);

    packet.set<"header_length">(6);
    packet.set<"checksum">(0);

    EXPECT_EQ(packet.calculate_checksum(), 0xb257);
    packet.set<"checksum">(0xb257);
    EXPECT_EQ(packet.calculate_checksum(), 0);
}

TEST(IPv4, PacketReassembly) {
    std::array<std::byte, 24> payload;
    std::ranges::copy(
        std::views::iota(0, 24) | std::views::transform([](auto v){return static_cast<std::byte>(v);}),
        payload.begin()
    );

    constexpr uint16_t header_length = ipv4::Format::byte_size();
    ipv4::Packet<std::array<std::byte, header_length + 24>> packet1{};
    packet1.set<"version">(4); 
    packet1.set<"header_length">(5); 
    packet1.set<"total_length">(header_length + 24);
    std::ranges::copy(
        payload,
        packet1.payload().begin()
    );

    auto packet2_1 = packet1;
    packet2_1.set<"protocol">(ipv4::Protocol(1));
    auto packet2_2 = packet2_1;

    packet2_1.set<"total_length">(header_length + 16);
    packet2_1.set<"more_fragments">(1);
    
    packet2_2.set<"total_length">(header_length + 8);
    packet2_2.set<"fragment_offset">(2);
    std::ranges::copy(
        payload | std::views::drop(16),
        packet2_2.payload().begin()
    );

    auto packet3_1 = packet1;
    packet3_1.set<"id">(1);
    packet3_1.set<"total_length">(header_length + 8);
    packet3_1.set<"more_fragments">(1);

    auto packet3_2 = packet3_1;
    packet3_2.set<"fragment_offset">(1);
    std::ranges::copy(
        std::span{payload}.subspan(8, 8),
        packet3_2.payload().begin()
    );

    auto packet3_3 = packet3_1;
    packet3_3.set<"fragment_offset">(2);
    packet3_3.set<"more_fragments">(0);
    std::ranges::copy(
        std::span{payload}.subspan(16, 8),
        packet3_3.payload().begin()
    );

    ipv4::Assembler assembler;

    auto datagram = assembler.assemble(packet1);
    ASSERT_TRUE(datagram.has_value());
    EXPECT_TRUE(std::ranges::equal(datagram->data, payload));

    datagram = assembler.assemble(packet2_1);
    EXPECT_FALSE(datagram.has_value());
    datagram = assembler.assemble(packet2_2);
    ASSERT_TRUE(datagram.has_value());
    EXPECT_TRUE(std::ranges::equal(datagram->data, payload));

    datagram = assembler.assemble(packet3_2);
    EXPECT_FALSE(datagram.has_value());
    datagram = assembler.assemble(packet3_3);
    EXPECT_FALSE(datagram.has_value());
    datagram = assembler.assemble(packet3_3);
    EXPECT_FALSE(datagram.has_value());
    datagram = assembler.assemble(packet3_1);
    ASSERT_TRUE(datagram.has_value());
    EXPECT_TRUE(std::ranges::equal(datagram->data, payload));

    assembler = {};
    
    datagram = assembler.assemble(packet2_1);
    EXPECT_FALSE(datagram.has_value());

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1100ms);

    datagram = assembler.assemble(packet2_2);
    EXPECT_FALSE(datagram.has_value());

    assembler = {};

    datagram = assembler.assemble(packet2_1);
    EXPECT_FALSE(datagram.has_value());
    datagram = assembler.assemble(packet3_2);
    EXPECT_FALSE(datagram.has_value());
    datagram = assembler.assemble(packet3_3);
    EXPECT_FALSE(datagram.has_value());

    datagram = assembler.assemble(packet2_2);
    ASSERT_TRUE(datagram.has_value());
    EXPECT_TRUE(std::ranges::equal(datagram->data, payload));
    EXPECT_EQ(datagram->protocol, ipv4::Protocol{1});

    datagram = assembler.assemble(packet3_1);
    ASSERT_TRUE(datagram.has_value());
    EXPECT_TRUE(std::ranges::equal(datagram->data, payload));
    EXPECT_EQ(datagram->protocol, ipv4::Protocol{0});
}
