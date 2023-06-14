#pragma once

#include <optional>
#include <cstdint>
#include <string_view>
#include <ranges>
#include <charconv>

#include "utils.h"

using IPv4_t = uint32_t;
using MAC_t = uint64_t;

constexpr inline std::optional<IPv4_t> parse_ipv4(std::string_view);
constexpr inline std::optional<MAC_t> parse_mac(std::string_view);

template<utils::StringLiteral string> requires (parse_ipv4(string).has_value())
constexpr IPv4_t operator""_ipv4() {
    return *parse_ipv4(string);
}

template<utils::StringLiteral string> requires (parse_mac(string).has_value())
constexpr MAC_t operator""_mac() {
    return *parse_mac(string);
}

template<typename ValueType> requires (std::unsigned_integral<ValueType>)
constexpr std::optional<ValueType> parse_delimited_bytes(std::string_view bytes, char delimiter, int byte_count, int base = 10) {
    ValueType value{};
    auto bytes_range = bytes | std::views::split(delimiter);
    auto byte_iterator = bytes_range.begin();

    for (int i = 0; i < byte_count; ++i) {
        if (byte_iterator == bytes_range.end()) {
            return std::nullopt;
        }

        std::string_view byte_view {(*byte_iterator).begin(), (*byte_iterator).end()};
        uint8_t byte{};
        
        auto result = std::from_chars(byte_view.begin(), byte_view.end(), byte, base);
        if (result.ec != std::errc{}) {
            return std::nullopt;
        }

        value = (value << 8) | byte;

        ++byte_iterator;
    }

    if (byte_iterator != bytes_range.end()) {
        return std::nullopt;
    }

    return value;
}

constexpr inline std::optional<IPv4_t> parse_ipv4(std::string_view address) {
    return parse_delimited_bytes<IPv4_t>(address, '.', 4);
}
constexpr inline std::optional<MAC_t> parse_mac(std::string_view address) {
    return parse_delimited_bytes<MAC_t>(address, ':', 6, 16);
}