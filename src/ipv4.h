#pragma once

#include <span>
#include <ranges>
#include <bit>
#include <numeric>
#include <cstdint>
#include <vector>
#include <cstddef>
#include <set>
#include <map>
#include <queue>
#include <chrono>
#include <utility>
#include <limits>
#include <optional>

#include "packet.h"
#include "types.h"

namespace ipv4 {
    using Format = packet::Format<
        {"version", 4},
        {"header_length", 4},
        {"type_of_service", 8},
        {"total_length", 16},
        {"id", 16},
        {"reserved", 1},
        {"dont_fragment", 1},
        {"more_fragments", 1},
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
    Packet(Range r) -> Packet<Range>;

    enum class Protocol : uint8_t {
        ICMP = 0x01,
        TCP = 0x06,
        UDP = 0x11
    };

    struct Datagram {
        IPv4_t source_address;
        IPv4_t destination_address;
        Protocol protocol;

        std::vector<std::byte> data;
    };

    class Assembler {
        using clock = std::chrono::steady_clock;

        struct Key {
            IPv4_t source_address;
            IPv4_t destination_address;
            Protocol protocol;
            uint16_t id;

            bool operator==(const Key&) const = default;
            auto operator<=>(const Key&) const = default;
        };

        struct Value {
            std::vector<std::byte> data;
            std::size_t total_size = std::numeric_limits<std::size_t>::max();
            std::size_t bytes_received{};
            std::set<uint16_t> offsets_received;
        };

        std::map<Key, Value> datagrams;
        std::queue<std::pair<clock::time_point, Key>> queue;

        void remove_older_then(clock::duration);
    
    public:
        [[nodiscard]] std::optional<Datagram> assemble(Packet<std::span<const std::byte>> packet);
    };

    template<typename InternetLayer>
    class Handler {
        Assembler assembler;

        InternetLayer& internet_layer() {
            return static_cast<InternetLayer&>(*this);
        }
    public:
        Handler() requires(std::derived_from<InternetLayer, Handler>) = default;

        void handle(Packet<std::span<const std::byte>> packet);
    };

    template<typename Range>
    auto Packet<Range>::payload() -> std::span<typename Packet::ValueType> {
        auto header_length = this->template get<"header_length">() * 4;
        return this->to_span(
            header_length,
            this->template get<"total_length">() - header_length
        );
    }

    template<typename Range>
    auto Packet<Range>::payload() const -> std::span<typename Packet::ConstValueType> {
        auto header_length = this->template get<"header_length">() * 4;
        return this->to_span(
            header_length,
            this->template get<"total_length">() - header_length
        );
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

    template<typename LinkLayer>
    void Handler<LinkLayer>::handle(Packet<std::span<const std::byte>> packet) {
        auto datagram = assembler.assemble(packet);

        if (datagram) {
            internet_layer().handle(std::move(*datagram));
        }
    }
}
