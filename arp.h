#pragma once

#include <vector>
#include <algorithm>
#include <cstddef>
#include <span>
#include <ranges>
#include <iterator>

#include "types.h"
#include "packet.h"

namespace arp {
    struct Entry {
        IPv4_t ip_address;
        MAC_t mac_address;
    };

    enum OpCode: uint16_t {
        REQUEST = 1,
        REPLY = 2
    };

    using Format = packet::Format<
        {"hardware_type", 16},
        {"protocol_type", 16},
        {"hardware_size", 8},
        {"protocol_size", 8},
        {"opcode", 16},
        {"source_mac", 48},
        {"source_ip", 32},
        {"destination_mac", 48},
        {"destination_ip", 32}
    >;

    template<typename Byte>
    struct Packet : packet::Packet<Byte, Format>{
        using packet::Packet<Byte, Format>::Packet;

        bool is_valid() {
            return 
                this->bytes.size() == Format::byte_size() && 
                this->template get<"hardware_type">() == 1 &&
                this->template get<"protocol_type">() == 0x0800 && 
                this->template get<"hardware_size">() == 6 &&
                this->template get<"protocol_size">() == 4;
        }
    };

    //P2582R1 not implemented
    template<typename Range>
    Packet(Range&& r) -> Packet<std::iter_value_t<decltype(std::ranges::begin(r))>>;

    class Cache {
        std::vector<Entry> cache;

    public:
        bool update(Entry entry) {
            auto it = std::ranges::find(cache, entry.ip_address, &Entry::ip_address);
            if (it != cache.end()) {
                it->mac_address = entry.mac_address;
                return true;
            }
            return false;
        }

        void insert(Entry entry) {
            cache.push_back(entry);
        }
    };
}