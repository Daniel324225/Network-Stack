#include "gtest/gtest.h"
#include "types.h"

TEST(Parsing, IPv4) {
    EXPECT_EQ("0.0.0.0"_ipv4, 0);
    EXPECT_EQ("127.0.0.1"_ipv4, 0x7F000001u);
    EXPECT_EQ("255.255.255.255"_ipv4, 0xFFFFFFFFu);
    EXPECT_EQ("192.168.10.25"_ipv4, 0xC0A80A19u);

    EXPECT_FALSE(parse_ipv4(""));
    EXPECT_FALSE(parse_ipv4("..."));
    EXPECT_FALSE(parse_ipv4("256.0.0.0"));
    EXPECT_FALSE(parse_ipv4("-1.0.0.0"));
    EXPECT_FALSE(parse_ipv4("0.0.0.0.0"));
    EXPECT_FALSE(parse_ipv4("0.0.0"));
    EXPECT_FALSE(parse_ipv4("0.0.0."));
    EXPECT_FALSE(parse_ipv4(".0.0.0"));
    EXPECT_FALSE(parse_ipv4("a.b.c.d"));
    EXPECT_FALSE(parse_ipv4("A.B.C.D"));
}

TEST(Parsing, MAC) {
    EXPECT_EQ("00:00:00:00:00:00"_mac, 0);
    EXPECT_EQ("01:02:03:04:05:06"_mac, 0x010203040506u);
    EXPECT_EQ("1:2:3:4:5:6"_mac, 0x010203040506u);
    EXPECT_EQ("01:10:ff:FF:Af:dC"_mac, 0x0110ffffafdcu);

    EXPECT_FALSE(parse_mac("-0:00:00:00:00:00"));
    EXPECT_FALSE(parse_mac("-00:00:00:00:00:00"));
    EXPECT_FALSE(parse_mac("G0:00:00:00:00:00"));
    EXPECT_FALSE(parse_mac("g0:00:00:00:00:00"));
    EXPECT_FALSE(parse_mac("123:00:00:00:00:00"));
    EXPECT_FALSE(parse_mac(":02:03:04:05:06"));
    EXPECT_FALSE(parse_mac("01:02:03:04:05:"));
    EXPECT_FALSE(parse_mac("01:02:03:04:05"));
    EXPECT_FALSE(parse_mac("01:02:03:04:05:06:07"));
    EXPECT_FALSE(parse_mac(":::::"));
    EXPECT_FALSE(parse_mac(""));
}