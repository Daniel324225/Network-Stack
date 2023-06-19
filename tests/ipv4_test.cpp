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
