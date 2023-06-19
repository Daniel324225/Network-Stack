#pragma once

#include <cstddef>
#include <algorithm>
#include <utility>

namespace utils {
    template<std::size_t length>
    struct StringLiteral {
        char string[length]{};

        constexpr StringLiteral(const char (&str)[length]) {
            std::ranges::copy(
                str,
                string
            );
        }

        constexpr operator std::string_view() const {
            return string;
        }

        template<std::size_t N>
        constexpr bool operator==(const StringLiteral<N>& other) const {
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

    //https://en.cppreference.com/w/cpp/utility/forward_like
    //not implemented in gcc 13.1
    template<class T, class U>
    [[nodiscard]] constexpr auto&& forward_like(U&& x) noexcept {
        constexpr bool is_adding_const = std::is_const_v<std::remove_reference_t<T>>;
        if constexpr (std::is_lvalue_reference_v<T&&>)
        {
            if constexpr (is_adding_const)
                return std::as_const(x);
            else
                return static_cast<U&>(x);
        }
        else
        {
            if constexpr (is_adding_const)
                return std::move(std::as_const(x));
            else
                return std::move(x);
        }
    }
}