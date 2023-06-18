#pragma once

#include <cstdint>

#include "packet.h"

namespace ethernet {
    enum Ethertype : uint16_t {
        ARP = 2054u,
        IPv4 = 2048u
    };

    using Format = packet::Format<
        {"destination_mac", 8*6},
        {"source_mac", 8*6},
        {"ethertype", 16}
    >;

    template<typename Byte>
    using Packet = packet::Packet<Byte, Format>;

    inline constexpr std::size_t max_size = 1522;
}
