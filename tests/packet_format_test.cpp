#include "gtest/gtest.h"
#include "packet_format.h"

#include <array>
#include <ranges>
#include <algorithm>
#include <format>

std::byte operator""_b(unsigned long long b) {
    return static_cast<std::byte>(b);
}

TEST(PacketFormat, ByteAligned) 
{
    std::array<std::byte, 33> packet{};
    std::ranges::copy(
        std::views::iota(1, 34) | std::views::transform([](auto value){return std::byte(value);}),
        packet.begin()
    );

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

TEST(PacketFormat, ByteUnaligned) 
{
    //                    5         7                     18                                                  71                                            4         5    2
    //                    a  |      b      |              c               |                                   d                                         |   e      |  f  | g
    std::array packet{0b10001'101_b, 0b1001'1001_b, 0b10000001_b, 0b100001'11_b, 0xFF_b, 0x00_b, 0xFF_b, 0x00_b, 0xFF_b, 0x00_b, 0xFF_b, 0x00_b, 0b10001'101_b, 0b1'10001'11_b};

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
