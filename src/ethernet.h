#pragma once

#include <cstdint>

#include "packet.h"
#include "types.h"

namespace ethernet {
    enum class Ethertype : uint16_t {
        ARP = 2054u,
        IPv4 = 2048u
    };

    inline constexpr MAC_t mac_broadcast = 0xFF'FF'FF'FF'FF'FF; 

    using Format = packet::Format<
        {"destination_mac", 8*6},
        {"source_mac", 8*6},
        {"ethertype", 16, packet::field_type<Ethertype>}
    >;

    template<typename Range>
    using Packet = packet::Packet<Range, Format>;

    inline constexpr std::size_t max_size = 1522;
}
