#pragma once

#include <cstddef>
#include <algorithm>

namespace utils {
    template <std::size_t length>
    struct StringLiteral
    {
        char string[length]{};

        constexpr StringLiteral(const char (&str)[length])
        {
            std::ranges::copy(
                str,
                string
            );
        }

        constexpr operator std::string_view() const
        {
            return string;
        }

        template<std::size_t N>
        constexpr bool operator==(const StringLiteral<N>& other) const
        {
            return std::ranges::equal(string, other.string);
        }
    };

    constexpr inline std::size_t bits_to_bytes(std::size_t bits) {
        return bits / 8 + (bits % 8 == 0 ? 0 : 1);
    }

    template<std::size_t bits>
    using leastN_t = 
        std::conditional_t< bits <= 8,  std::uint8_t, 
        std::conditional_t< bits <= 16, std::uint16_t,
        std::conditional_t< bits <= 32, std::uint32_t,
        std::conditional_t< bits <= 64, std::uint64_t,
                                        std::array<std::byte, bits_to_bytes(bits)>
        >>>>;
}