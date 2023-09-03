#pragma once

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

#include "utils.h"

namespace packet {
    template<typename T>
    struct is_byte_array : std::false_type{};

    template<std::size_t N>
    struct is_byte_array<std::array<std::byte, N>> : std::true_type{};

    template<utils::StringLiteral Name, std::size_t BitLength, typename Type = utils::leastN_t<BitLength>>
    requires (
        sizeof(Type) * 8 >= BitLength && 
        (
            std::is_unsigned_v<Type> || 
            (std::is_enum_v<Type> && std::is_unsigned_v<std::underlying_type_t<Type>>) ||
            is_byte_array<Type>::value
        )
    )
    struct Field_t {
        static constexpr auto name = Name;
        static constexpr auto bit_length =  BitLength;
        using type = Type;
    };

    template<typename T>
    struct FieldType {
        using type = T;
    };

    template<typename T>
    constexpr FieldType<T> field_type{}; 

    template<typename type, std::size_t name_length>
    struct Field {
        utils::StringLiteral<name_length> name;
        std::size_t bit_length;
        FieldType<type> field_type = {};
    };

    template<std::size_t N>
    Field(const char (&name)[N], std::size_t) -> Field<void, N>;

    template<std::size_t N, typename type>
    Field(const char (&name)[N], std::size_t, FieldType<type>) -> Field<type, N>;

    template<typename T>
    struct is_field : std::false_type{};

    template<utils::StringLiteral name_, std::size_t bit_length_, typename type_>
    struct is_field<Field_t<name_, bit_length_, type_>> : std::true_type{};

    template<typename T>
    concept field_concept = is_field<T>::value;

    template<field_concept... Fields>
    constexpr bool are_names_unique() {
        std::vector<std::string_view> names;
        
        (names.push_back(Fields::name), ...);

        std::ranges::sort(names);

        return std::ranges::adjacent_find(names) == names.end();
    }

    template<std::size_t first_bit, std::size_t bit_length, typename ReturnType>
    ReturnType get(std::span<const std::byte, utils::bits_to_bytes(first_bit + bit_length)> bytes) {
        constexpr auto bit_end = (first_bit + bit_length) % 8;

        const auto field_bytes = [&]{
            if constexpr (bit_end == 0) {
                return bytes;
            } else {
                return
                    bytes
                    | std::views::pairwise_transform(
                        [&](auto lhs, auto rhs){
                            return (lhs << bit_end) | (rhs >> (8 - bit_end));
                        }
                    );
            }
        }();

        using UnderlyingType = decltype([]{
            if constexpr (std::is_enum_v<ReturnType>) {
                return std::underlying_type_t<ReturnType>{};
            } else {
                return ReturnType{};
            }
        }());

        UnderlyingType return_value{};
        auto first_return_byte = reinterpret_cast<std::byte*>(&return_value) + sizeof(return_value) - utils::bits_to_bytes(bit_length);

        constexpr auto first_byte_skipped = bit_end > first_bit;

        if constexpr (first_byte_skipped) {
            *first_return_byte |= bytes.front() >> (8 - bit_end);
        }

        std::ranges::copy(
            field_bytes,
            first_return_byte + (first_byte_skipped ? 1 : 0)
        );

        if constexpr(bit_length % 8 != 0) {
            *first_return_byte &= std::byte{0xFF} >> (8 - (bit_length % 8));
        }

        if constexpr(std::endian::native == std::endian::little && std::integral<UnderlyingType>) {
            return_value = std::byteswap(return_value);
        }

        if constexpr(std::is_enum_v<ReturnType>) {
            return ReturnType{return_value};
        } else {
            return return_value;
        }
    }

    template<std::size_t first_bit, std::size_t bit_length, typename Type>
    void set(std::span<std::byte, utils::bits_to_bytes(first_bit + bit_length)> bytes, Type value) {
        using ExtendedValueType = utils::leastN_t<first_bit + bit_length>;

        constexpr auto bit_end = (first_bit + bit_length) % 8;
        constexpr auto left_shift = (8 - bit_end) % 8;

        if constexpr (std::integral<ExtendedValueType>) {
            ExtendedValueType extended_value = [&]{
                if constexpr(std::integral<Type>) {
                    return value;
                } else if constexpr(std::is_enum_v<Type>) {
                    return std::to_underlying(value);
                } else {
                    ExtendedValueType extended_value;
                    for (auto byte : std::views::counted(reinterpret_cast<unsigned char*>(&value), sizeof(value))) {
                        extended_value <<= 8;
                        extended_value += byte;
                    }
                }
            }();

            constexpr auto first_value_byte_offset = sizeof(extended_value) - utils::bits_to_bytes(first_bit + bit_length);
            const auto first_value_byte = reinterpret_cast<std::byte*>(&extended_value) + first_value_byte_offset;

            if constexpr(bit_length != sizeof(extended_value) * 8) {
                const auto mask = ~((~ExtendedValueType{}) << bit_length) << left_shift;
                extended_value <<= left_shift;
                
                ExtendedValueType old{};
                std::ranges::copy(
                    bytes,
                    reinterpret_cast<std::byte*>(&old) + first_value_byte_offset
                );
                if constexpr(std::endian::native == std::endian::little) {
                    old = std::byteswap(old);
                }

                extended_value = (old & (~mask)) | (extended_value & mask);
            }

            if constexpr(std::endian::native == std::endian::little) {
                extended_value = std::byteswap(extended_value);
            }
            
            std::ranges::copy_n(
                first_value_byte, bytes.size(),
                bytes.begin()
            );
        } else {
            if constexpr (std::integral<decltype(value)> && std::endian::native == std::endian::little) {
                value = std::byteswap(value);
            }

            constexpr auto first_byte_skipped = bit_length % 8 + left_shift > 8;

            auto first_value_byte = reinterpret_cast<std::byte*>(&value);

            if constexpr (first_byte_skipped) {
                const auto mask = std::byte{0xFF} >> first_bit;
                bytes.front() = (bytes.front() & ~mask) | ((*first_value_byte >> (8 - left_shift)) & mask);
            } else if constexpr(constexpr auto bits_in_first_value_byte = bit_length % 8; bits_in_first_value_byte != 0) {
                const auto mask = std::byte{0xFF} >> (8 - bits_in_first_value_byte);
                *first_value_byte &= mask;
                *first_value_byte |= (bytes.front() >> left_shift) & ~mask;
            }

            auto value_bytes = std::ranges::subrange(first_value_byte, first_value_byte + utils::bits_to_bytes(bit_length));

            auto packet_bytes = [&]{
                if constexpr(bit_end == 0) {
                    return value_bytes;
                } else {
                    return
                        value_bytes
                        | std::views::pairwise_transform(
                            [&](auto lhs, auto rhs){
                                return (lhs << left_shift) | (rhs >> (8 - left_shift));
                            }
                        );
                }
            }();

            std::ranges::copy(
                packet_bytes,
                bytes.begin() + (first_byte_skipped ? 1 : 0)
            );

            if constexpr(bit_end != 0) {
                const auto mask = std::byte{0xFF} >> (8 - left_shift);
                bytes.back() = (bytes.back() & mask) | (value_bytes.back() << left_shift);
            }
        }
    }

    template<field_concept... Fields>
        requires (
            are_names_unique<Fields...>() &&
            ((Fields::bit_length >=1) && ...)
        )
    struct Format_t {
        static constexpr bool contains(std::string_view name) {
            return ((Fields::name == name) || ...);
        }

        static constexpr std::size_t bit_size() {
            return (Fields::bit_length + ...);
        }

        static constexpr std::size_t byte_size() {
            return utils::bits_to_bytes(bit_size());
        }

        static constexpr std::size_t field_length(std::string_view name) {
            std::size_t length{};
            ([&]{
                if (Fields::name == name) {
                    length = Fields::bit_length;
                }
            }(),...);

            return length;
        }

        static constexpr std::size_t field_start(std::string_view name) {
            using pair = std::pair<std::string_view, std::size_t>;

            static constexpr std::array field_vec {
                pair{Fields::name, Fields::bit_length}...
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

        template<utils::StringLiteral name, field_concept Field>
        struct names_equal : Field {
            static constexpr bool value = name == Field::name;
            //using type = typename Field::type;
        };

        template<utils::StringLiteral name>
            requires (contains(name))
        using field_type = typename std::disjunction<names_equal<name, Fields>...>::type;

        template<utils::StringLiteral name>
            requires (contains(name))
        static field_type<name> get(std::span<const std::byte> bytes) {
            static constexpr auto bit_begin = field_start(name);
            static constexpr auto bit_length = field_length(name);

            const auto byte_begin = bit_begin / 8;
            const auto byte_length = utils::bits_to_bytes(bit_begin + bit_length) - byte_begin;
            
            return ::packet::get<bit_begin % 8, bit_length, field_type<name>>(bytes.subspan<byte_begin, byte_length>());
        }

        template<utils::StringLiteral name>
            requires (contains(name))
        static void set(std::span<std::byte> bytes, field_type<name> value) {
            static constexpr auto bit_begin = field_start(name);
            static constexpr auto bit_length = field_length(name);

            const auto byte_begin = bit_begin / 8;
            const auto byte_length = utils::bits_to_bytes(bit_begin + bit_length) - byte_begin;
            
            ::packet::set<bit_begin % 8, bit_length>(bytes.subspan<byte_begin, byte_length>(), value);
        }
    };

    template<std::ranges::contiguous_range Range_, typename Format_>
        requires (std::same_as<std::ranges::range_value_t<Range_>, std::byte>)
    struct Packet {
        using Range = Range_;
        using Format = Format_;
        using ValueType = std::remove_reference_t<std::ranges::range_reference_t<Range>>;
        using ConstValueType = std::add_const_t<ValueType>;

        Range bytes;

        Packet() = default;

        template<std::convertible_to<Range> R>
        Packet(R&& bytes) : bytes{std::forward<R>(bytes)} {}
        
        template<typename P>
            requires (
                std::constructible_from<Range, typename std::remove_reference_t<P>::Range> &&
                std::same_as<Format, typename std::remove_reference_t<P>::Format>
            )
        Packet(P&& other) : bytes{utils::forward_like<P>(other.bytes)} {}

        template<utils::StringLiteral name>
        Format::template field_type<name> get() const {
            return Format::template get<name>(bytes);
        }

        template<utils::StringLiteral name>
            requires (!std::is_const_v<ValueType>)
        void set(Format::template field_type<name> value) {
            Format::template set<name>(bytes, value);
        }

        std::span<ValueType> to_span(std::size_t offset = 0, std::size_t count = std::dynamic_extent) {
            return std::span{bytes}.subspan(offset, count);
        }

        std::span<ConstValueType> to_span(std::size_t offset = 0, std::size_t count = std::dynamic_extent) const {
            return std::span<ConstValueType>{bytes}.subspan(offset, count);
        }

        template<template<typename> typename T>
        T<std::span<ValueType>> data() {
            return to_span().subspan(Format::byte_size());
        }

        template<template<typename> typename T>
        T<std::span<ConstValueType>> data() const {
            return to_span().subspan(Format::byte_size());
        }

        std::span<ValueType> data() {
            return to_span().subspan(Format::byte_size());
        }

        std::span<ConstValueType> data() const {
            return to_span().subspan(Format::byte_size());
        }
    };

    template<typename Range, typename Format>
    Packet(Range r) -> Packet<Range, Format>;

    template<Field... fields>
    using Format = Format_t<
        Field_t<
            fields.name, 
            fields.bit_length,
            std::conditional_t<
                std::same_as<void, typename decltype(fields.field_type)::type>,
                utils::leastN_t<fields.bit_length>,
                typename decltype(fields.field_type)::type
            >
        >...
    >;
}