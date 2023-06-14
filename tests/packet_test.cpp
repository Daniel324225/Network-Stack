#include <array>
#include <ranges>
#include <algorithm>
#include <format>

#include "gtest/gtest.h"
#include "packet.h"
#include "utils.h"

constexpr std::byte operator""_b(unsigned long long b) {
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
    auto packet_array = std::to_array<unsigned char>({0b10001'101, 0b1101'1001, 0b10000001, 0b100001'11, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0b10001'101, 0b1'10001'11});
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
    EXPECT_EQ(format::get<"b">(packet), 0b101'1101);
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

template<
    utils::StringLiteral a, auto a_value, 
    utils::StringLiteral b, auto b_value,
    utils::StringLiteral c, auto c_value
    >
void test(
    auto format, auto packet_array
) {
    auto packet = std::as_writable_bytes(std::span{packet_array});

    format.template set<b>(packet, b_value);
    EXPECT_EQ(format.template get<a>(packet), decltype(a_value){}) << std::format("Setting field {} changes field {}\n", b.string, a.string);
    EXPECT_EQ(format.template get<b>(packet), b_value) << std::format("Field {} is not set correctly", b.string);
    EXPECT_EQ(format.template get<c>(packet), decltype(c_value){}) << std::format("Setting field {} changes field {}\n", b.string, c.string);

    constexpr auto bit_len = format.template field_length(b);
    if (bit_len % 8 != 0) {
        utils::leastN_t<bit_len> copy = b_value;
        *(std::byte*)&copy |= 0b1000'0000_b;
        format.template set<b>(packet, copy);
        EXPECT_EQ(format.template get<a>(packet), decltype(a_value){}) << std::format("Setting field {} changes field {}\n", b.string, a.string);
        EXPECT_EQ(format.template get<b>(packet), b_value) << std::format("Field {} is not set correctly", b.string);
        EXPECT_EQ(format.template get<c>(packet), decltype(c_value){}) << std::format("Setting field {} changes field {}\n", b.string, c.string);
    }
    format.template set<a>(packet, a_value);
    format.template set<c>(packet, c_value);
    format.template set<b>(packet, decltype(b_value){});

    EXPECT_EQ(format.template get<a>(packet), a_value) << std::format("Clearing field {} changes field {}\n", b.string, a.string);
    EXPECT_EQ(format.template get<b>(packet), decltype(b_value){}) << std::format("Field {} is not cleared correctly", b.string);
    EXPECT_EQ(format.template get<c>(packet), c_value) << std::format("Clearing field {} changes field {}\n", b.string, c.string);
};

TEST(FormatSet, ByteUnaligned) 
{
    std::array<unsigned char, 41> packet_array{};
    auto packet = std::as_writable_bytes(std::span{packet_array});

    using format = Format< // starting bit position
        Field{"start", 8}, // 0
        Field{"a",     5}, // 0
        Field{"b",     7}, // 5
        Field{"c",     18},// 4
        Field{"d",     71},// 6
        Field{"e",     4}, // 5
        Field{"f",     5}, // 1
        Field{"g",     60},// 6
        Field{"h",     68},// 2
        Field{"i",     70},// 2 
        Field{"j",     4}, // 4 
        Field{"end",   8}  // 0
    >;

    test<
        "start", 0b1000'0001u, 
        "a",     0b0001'0001u,   
        "b",     0b0100'0001u
    >(format{}, packet);

    test<
        "a",     0b0001'0001u,   
        "b",     0b0100'0001u,
        "c",     0b0000'0011'1000'0001'1000'0001u
    >(format{}, packet);

    test<
        "b",     0b0100'0001u,
        "c",     0b0000'0011'1000'0001'1000'0001u, 
        "d",     std::array{0b0100'0001_b, 0x81_b, 0xFF_b, 0x00_b, 0xFF_b, 0x81_b, 0x81_b, 0x81_b, 0x81_b}
    >(format{}, packet);

    test<
        "c",     0b0000'0011'1000'0001'1000'0001u, 
        "d",     std::array{0b0100'0001_b, 0x81_b, 0xFF_b, 0x00_b, 0xFF_b, 0x81_b, 0x81_b, 0x81_b, 0x81_b},
        "e",     0b0000'1001u
    >(format{}, packet);

    test<
        "d",     std::array{0b0100'0001_b, 0x81_b, 0xFF_b, 0x00_b, 0xFF_b, 0x81_b, 0x81_b, 0x81_b, 0x81_b},
        "e",     0b0000'1001u,
        "f",     0b0001'0001u
    >(format{}, packet);

    test<
        "e",     0b0000'1001u,
        "f",     0b0001'0001u,
        "g",     0b0000'1001'1000'0001'1000'0001'1000'0001'1000'0001'1000'0001'1000'0001'1000'0001u
    >(format{}, packet);

    test<
        "f",     0b0001'0001u,
        "g",     0b0000'1001'1000'0001'1000'0001'1000'0001'1000'0001'1000'0001'1000'0001'1000'0001u,
        "h",     std::array{0b0000'1001_b, 0x81_b, 0xFF_b, 0x00_b, 0xFF_b, 0x81_b, 0x81_b, 0x81_b, 0x81_b}
    >(format{}, packet);

    test<
        "g",     0b0000'1001'1000'0001'1000'0001'1000'0001'1000'0001'1000'0001'1000'0001'1000'0001u,
        "h",     std::array{0b0000'1001_b, 0x81_b, 0xFF_b, 0x00_b, 0xFF_b, 0x81_b, 0x81_b, 0x81_b, 0x81_b},
        "i",     std::array{0b0010'1001_b, 0x81_b, 0xFF_b, 0x00_b, 0xFF_b, 0x81_b, 0x81_b, 0x81_b, 0x81_b}
    >(format{}, packet);

    test<
        "h",     std::array{0b0000'1001_b, 0x81_b, 0xFF_b, 0x00_b, 0xFF_b, 0x81_b, 0x81_b, 0x81_b, 0x81_b},
        "i",     std::array{0b0010'1001_b, 0x81_b, 0xFF_b, 0x00_b, 0xFF_b, 0x81_b, 0x81_b, 0x81_b, 0x81_b},
        "j",     0b0000'1001u
    >(format{}, packet);

    test<
        "i",     std::array{0b0010'1001_b, 0x81_b, 0xFF_b, 0x00_b, 0xFF_b, 0x81_b, 0x81_b, 0x81_b, 0x81_b},
        "j",     0b0000'1001u,
        "end",   0b1000'0001u
    >(format{}, packet);
}
