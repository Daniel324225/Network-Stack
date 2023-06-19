#pragma once

#include <span>
#include <ranges>
#include <bit>
#include <numeric>

#include "packet.h"

namespace ipv4 {
    using Format = packet::Format<
        {"version", 4},
        {"header_length", 4},
        {"type_of_service", 8},
        {"datagram_length", 16},
        {"id", 16},
        {"flags", 3},
        {"fragment_offset", 13},
        {"ttl", 8},
        {"protocol", 8},
        {"checksum", 16},
        {"source_address", 32},
        {"destination_address", 32}
    >;

    template<typename Range>
    struct Packet : packet::Packet<Range, Format> {
        using packet::Packet<Range, Format>::Packet;

        std::span<typename Packet::ValueType> options();
        std::span<typename Packet::ValueType> payload();

        std::span<typename Packet::ConstValueType> options() const;
        std::span<typename Packet::ConstValueType> payload() const;

        uint16_t calculate_checksum() const;
    };

    template<typename Range>
    auto Packet<Range>::payload() -> std::span<typename Packet::ValueType> {
        return this->to_span(this->template get<"header_length">() * 4);
    }

    template<typename Range>
    auto Packet<Range>::payload() const -> std::span<typename Packet::ConstValueType> {
        return this->to_span(this->template get<"header_length">() * 4);
    }

    template<typename Range>
    auto Packet<Range>::options() -> std::span<typename Packet::ValueType> {
        return std::span{
            this->data().begin(),
            payload().begin()
        };
    }

    template<typename Range>
    auto Packet<Range>::options() const -> std::span<typename Packet::ConstValueType> {
        return std::span{
            this->data().begin(),
            payload().begin()
        };
    }

    template<typename Range>
    uint16_t Packet<Range>::calculate_checksum() const {
        auto header_span = std::span{this->to_span().begin(), options().end()};
        auto nums = 
            header_span 
            | std::views::chunk(2) 
            | std::views::transform([](auto word){
                uint16_t value;
                std::ranges::copy(
                    word,
                    reinterpret_cast<std::byte*>(&value)
                );
                return value;
            });
        auto sum = std::accumulate(nums.begin(), nums.end(), std::uint32_t{});
        while (sum >> 16 != 0) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }

        auto checksum = static_cast<uint16_t>(sum);

        if constexpr (std::endian::native == std::endian::little) {
            checksum = std::byteswap(checksum);
        }

        return ~checksum;
    }

    template<typename Range>
    Packet(Range&& r) -> Packet<Range>;
}
