#pragma once

#include <vector>
#include <algorithm>
#include <cstddef>
#include <span>
#include <ranges>

#include "types.h"
#include "packet.h"

namespace Arp 
{
    struct Entry {
        IPv4_t ip_address;
        MAC_t mac_address;
    };

    enum class OpCode: uint16_t {
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

    inline bool is_valid(std::span<const std::byte> packet) {
        return 
            packet.size() == Format::byte_size() && 
            Format::get<"hardware_type">(packet) == 1 &&
            Format::get<"protocol_type">(packet) == 0x0800 && 
            Format::get<"hardware_size">(packet) == 6 &&
            Format::get<"protocol_size">(packet) == 4;
    }

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