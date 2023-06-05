#include <string_view>
#include <ranges>
#include <algorithm>
#include <numeric>
#include <concepts>
#include <type_traits>
#include <vector>
#include <array>
#include <span>
#include <cstddef>
#include <cstdint>
#include <bit>

#include <format>
#include <iostream>

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

template<std::size_t name_length>
struct Field 
{
    StringLiteral<name_length> name;
    std::size_t bit_length;
};

template<std::size_t N>
Field(const char (&name)[N], std::size_t) -> Field<N>;

template <Field ...fields>
constexpr bool are_names_unique()
{
    std::vector<std::string_view> names;
    
    (names.push_back(fields.name), ...);

    std::ranges::sort(names);

    return std::ranges::adjacent_find(names) == names.end();
}

// template<std::size_t bits>
// using leastN_t = decltype([]{
//     if constexpr      (bits <= 8)  {return std::uint8_t{};}
//     else if constexpr (bits <= 16) {return std::uint16_t{};}
//     else if constexpr (bits <= 32) {return std::uint32_t{};}
//     else if constexpr (bits <= 64) {return std::uint64_t{};}
//     else                           {return std::array<std::byte, bits / 8 + (bits % 8 == 0 ? 0 : 1)>{};}
// }());

template<std::size_t bits>
using leastN_t = 
    std::conditional_t< bits <= 8,  std::uint8_t, 
    std::conditional_t< bits <= 16, std::uint16_t,
    std::conditional_t< bits <= 32, std::uint32_t,
    std::conditional_t< bits <= 64, std::uint64_t,
                                    std::array<std::byte, bits / 8 + (bits % 8 == 0 ? 0 : 1)>
    >>>>;

constexpr std::size_t bits_to_bytes(std::size_t bits) {
    return bits / 8 + (bits % 8 == 0 ? 0 : 1);
}

template<Field ...fields>
    requires 
    (
        are_names_unique<fields...>() &&
        ((fields.bit_length >=1) && ...)
    )
struct Format
{
    static constexpr bool contains(std::string_view name)
    {
        return ((fields.name == name) || ...);
    }

    static constexpr std::size_t field_length(std::string_view name)
    {
        std::size_t length{};
        ([&]{
            if (fields.name == name) {
                length = fields.bit_length;
            }
        }(),...);

        return length;
    }

    static constexpr std::size_t field_start(std::string_view name)
    {
        using pair = std::pair<std::string_view, std::size_t>;

        static constexpr std::array field_vec {
            pair{fields.name, fields.bit_length}...
        };

        std::size_t start{};
        for (auto [field_name, length] : field_vec) {
            if (field_name == name) {
                break;
            }
            start += length;
        }
        return start;
    }

    template<StringLiteral name>
        requires (contains(name))
    static leastN_t<field_length(name)> get(std::span<std::byte> bytes)
    {
        static constexpr auto bit_begin = field_start(name);
        static constexpr auto bit_length = field_length(name);
        static constexpr auto bit_end = bit_begin + bit_length;
        static constexpr auto bits_in_last_byte = bit_end % 8;
        
        using return_t = leastN_t<bit_length>;

        const auto byte_begin = bit_begin / 8;
        const auto byte_length = bits_to_bytes(bit_end) - byte_begin;

        const auto field_bytes = [&]{
            const auto view = bytes | std::views::drop(byte_begin) | std::views::take(byte_length); 
            if constexpr (bits_in_last_byte == 0) {
                return view;
            } else {
                //std::cout << "bits_in_last_byte: " << bits_in_last_byte << "\n";
                return
                    view 
                    | std::views::pairwise_transform(
                        [](auto lhs, auto rhs){
                            //std::cout << std::format("{:0>8b} {:0>8b} | {:0>8b}\n", (int)lhs, (int)rhs, (int)((lhs << bits_in_last_byte) | (rhs >> (8 - bits_in_last_byte))));
                            return (lhs << bits_in_last_byte) | (rhs >> (8 - bits_in_last_byte));
                        }
                    );
            }
        }();

        return_t return_value{};
        auto first_return_byte = reinterpret_cast<std::byte*>(&return_value) + sizeof(return_value) - bits_to_bytes(bit_length);

        static constexpr auto first_byte_skipped = bits_in_last_byte > bit_begin % 8;

        if constexpr (first_byte_skipped) {
            *first_return_byte |= bytes[byte_begin] >> (8 - (bit_length % 8));
        }

        std::ranges::copy(
            field_bytes,
            first_return_byte + (first_byte_skipped ? 1 : 0)
        );

        if constexpr(bit_length % 8 != 0) {
            *first_return_byte &= std::byte{0xFF} >> (8 - (bit_length % 8));
        }

        if constexpr(std::endian::native == std::endian::little && std::integral<return_t>) {
            return_value = std::byteswap(return_value);
        }

        return return_value;
    }
};