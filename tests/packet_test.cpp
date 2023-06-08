#include "gtest/gtest.h"
#include "packet.h"

#include <array>
#include <ranges>
#include <algorithm>
#include <format>

std::byte operator""_b(unsigned long long b) {
    return static_cast<std::byte>(b);
}

using packet::Format;
using packet::Field;

TEST(FormatGet, ByteAligned) 
{
    std::array<unsigned char, 33> packet_array{};
    std::ranges::iota(
        packet_array,
        1
    );

    auto packet = as_bytes(std::span{packet_array});

    using format = Format<
        Field{"a", 8},
        Field{"b", 16},
        Field{"c", 24},
        Field{"d", 32},
        Field{"e", 40},
        Field{"f", 64},
        Field{"g", 80}
    >;

    EXPECT_EQ(format::get<"a">(packet), 0x01);
    EXPECT_EQ(format::get<"b">(packet), 0x02'03);
    EXPECT_EQ(format::get<"c">(packet), 0x04'05'06);
    EXPECT_EQ(format::get<"d">(packet), 0x07'08'09'0A);
    EXPECT_EQ(format::get<"e">(packet), 0x0B'0C'0D'0E'0F);
    EXPECT_EQ(format::get<"f">(packet), 0x10'11'12'13'14'15'16'17);
    EXPECT_EQ(format::get<"g">(packet), (std::array{0x18_b, 0x19_b, 0x1A_b, 0x1B_b, 0x1C_b, 0x1D_b, 0x1E_b, 0x1F_b, 0x20_b, 0x21_b}));
}

TEST(FormatGet, ByteUnaligned) 
{
    //                                                    5        7                    18                                              71                          4       5    2
    //                                                    a  |     b     |              c           |                                   d                       |   e    |  f  | g
    auto packet_array = std::to_array<unsigned char>({0b10001'101, 0b1001'1001, 0b10000001, 0b100001'11, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0b10001'101, 0b1'10001'11});
    auto packet = as_bytes(std::span{packet_array});

    using format = Format<
        Field{"a", 5},
        Field{"b", 7},  
        Field{"c", 18},
        Field{"d", 71},
        Field{"e", 4},
        Field{"f", 5},
        Field{"g", 2}
    >;

    EXPECT_EQ(format::get<"a">(packet), 0b10001);
    EXPECT_EQ(format::get<"b">(packet), 0b101'1001);
    EXPECT_EQ(format::get<"c">(packet), 0b1001'10000001'100001);
    EXPECT_EQ(format::get<"d">(packet), (std::array{0b0111'1111_b, 0b1110'0000_b, 0b0001'1111_b, 0b1110'0000_b, 0b0001'1111_b, 0b1110'0000_b, 0b0001'1111_b, 0b1110'0000_b, 0b0001'0001_b}));
    EXPECT_EQ(format::get<"e">(packet), 0b1011);
    EXPECT_EQ(format::get<"f">(packet), 0b10001);
    EXPECT_EQ(format::get<"g">(packet), 0b11);
}

TEST(FormatSet, ByteAligned) {
    std::array<unsigned char, 34> packet_array{};

    auto packet = as_writable_bytes(std::span{packet_array});

    using format = Format<
        Field{"a", 8},
        Field{"b", 16},
        Field{"c", 24},
        Field{"d", 32},
        Field{"e", 40},
        Field{"f", 64},
        Field{"g", 80},
        Field{"h", 8}
    >;

    format::set<"a">(packet, 0x81u);
    EXPECT_EQ(format::get<"a">(packet), 0x81u);
    EXPECT_EQ(format::get<"b">(packet), 0u);
    format::set<"b">(packet, 0x8181u);
    EXPECT_EQ(format::get<"b">(packet), 0x8181u);
    format::set<"a">(packet, 0u);
    EXPECT_EQ(format::get<"a">(packet), 0u);
    EXPECT_EQ(format::get<"b">(packet), 0x8181u);

    format::set<"c">(packet, 0x818181);
    format::set<"d">(packet, 0x81818181);
    EXPECT_EQ(format::get<"b">(packet), 0x8181u);
    EXPECT_EQ(format::get<"c">(packet), 0x818181u);
    EXPECT_EQ(format::get<"d">(packet), 0x81818181u);

    format::set<"c">(packet, 0u);
    EXPECT_EQ(format::get<"b">(packet), 0x8181u);
    EXPECT_EQ(format::get<"c">(packet), 0u);
    EXPECT_EQ(format::get<"d">(packet), 0x81818181u);

    std::array<std::byte, 10> array;
    std::ranges::fill(array, std::byte{0x81});
    format::set<"g">(packet, array);
    EXPECT_EQ(format::get<"f">(packet), 0u);
    EXPECT_EQ(format::get<"g">(packet), array);
    EXPECT_EQ(format::get<"h">(packet), 0u);

    format::set<"f">(packet, 0x81'81'81'81'81'81'81'81u);
    format::set<"h">(packet, 0x81u);
    format::set<"g">(packet, {});

    EXPECT_EQ(format::get<"f">(packet), 0x81'81'81'81'81'81'81'81u);
    EXPECT_EQ(format::get<"g">(packet), (std::array<std::byte, 10>{}));
    EXPECT_EQ(format::get<"h">(packet), 0x81u);
}

TEST(FormatSet, ByteUnaligned) 
{
    std::array<unsigned char, 14> packet_array{};
    auto packet = std::as_writable_bytes(std::span{packet_array});

    using format = Format<
        Field{"a", 5},
        Field{"b", 7},  
        Field{"c", 18},
        Field{"d", 71},
        Field{"e", 4},
        Field{"f", 5},
        Field{"g", 2}
    >;

    format::set<"a">(packet, 0b10001u);
    EXPECT_EQ(format::get<"a">(packet), 0b10001u);
    EXPECT_EQ(format::get<"b">(packet), 0u);

    packet_array = {};
    format::set<"b">(packet, 0b1000001u);
    EXPECT_EQ(format::get<"a">(packet), 0u);
    EXPECT_EQ(format::get<"b">(packet), 0b1000001u);
    EXPECT_EQ(format::get<"c">(packet), 0u);

}
