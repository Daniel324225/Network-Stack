#pragma once

#include <cstdint>

#include "packet.h"

namespace Ethernet {
    enum Ethertype : uint16_t {
        ARP = 2054u,
        IPv4 = 2048u
    };

    using Format = packet::Format<
        {"dmac", 8*6},
        {"smac", 8*6},
        {"ethertype", 16}
    >;

    template<typename Byte>
    using Packet = packet::Packet<Byte, Format>;
}
