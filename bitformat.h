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
#include <tuple>
#include <cstdint>

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

template<std::size_t bits>
using leastN_t = decltype([]{
    if constexpr      (bits <= 8)  {return std::uint8_t{};}
    else if constexpr (bits <= 16) {return std::uint16_t{};}
    else if constexpr (bits <= 32) {return std::uint32_t{};}
    else if constexpr (bits <= 64) {return std::uint64_t{};}
    else                           {return std::array<std::byte, bits / 8 + (bits % 8 == 0 ? 0 : 1)>{};}
}());

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
        static constexpr auto bit_start = Format::field_start(name);
        static constexpr auto bit_length = Format::field_length(name);
        using return_t = leastN_t<field_length(name)>;

        return_t return_value{};

        if constexpr(field_start % 8 == 0 && field_length % 8 == 0) {
            const auto byte_start = bit_start / 8;
            const auto byte_length = bit_start / 8;
            auto field_bytes = bytes | std::views::drop(byte_start) | std::views::take(byte_length);
            std::ranges::copy(
                field_bytes,
                (std::byte*)
            )
        }

        return return_value;
    }
};