#pragma once

enum class Ethertype : uint16_t {
    ARP = 2054u,
    IPv4 = 2048u
};

using EthernetHeader = packet::Format<
    {"dmac", 8*6},
    {"smac", 8*6},
    {"ethertype", 16}
>;