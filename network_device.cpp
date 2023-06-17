#include <format>
#include <chrono>
#include <iostream>

#include "packet.h"
#include "network_device.h"
#include "ethernet.h"

void NetworkDevice::handle(arp::Packet<const std::byte> packet) {
    if (!packet.is_valid()) {
        std::cout << "Invalid ARP packet\n";
        return;
    }

    arp::Entry entry = {
        packet.get<"source_ip">(),
        packet.get<"source_mac">()
    };

    bool merge = arp_cache.update(entry);

    if (ip_address == packet.get<"destination_ip">()) {
        if (!merge) {
            arp_cache.insert(entry);
        }
        
        if (packet.get<"opcode">() == arp::REQUEST) {
            std::array<std::byte, arp::Format::byte_size()> reply_buffer;
            std::ranges::copy(packet.bytes, std::begin(reply_buffer));

            arp::Packet reply(reply_buffer);
            
            const auto source_mac = packet.get<"source_mac">();
            const auto source_ip = packet.get<"source_ip">();

            reply.set<"opcode">(arp::REPLY);
            reply.set<"destination_mac">(source_mac);
            reply.set<"destination_ip">(source_ip);
            reply.set<"source_mac">(mac_address);
            reply.set<"source_ip">(ip_address);

            send(source_mac, ethernet::ARP, reply_buffer);
        }
    }
}

static constexpr std::size_t ethernet_max_size = 1522;

void NetworkDevice::send(MAC_t destination, ethernet::Ethertype ethertype, std::span<const std::byte> payload) {
    std::array<std::byte, ethernet_max_size> buffer;

    ethernet::Packet packet{buffer};

    packet.set<"dmac">(destination);
    packet.set<"smac">(mac_address);
    packet.set<"ethertype">(ethertype);

    auto [_, last] = std::ranges::copy(payload, packet.data().begin());

    tap_device.write({packet.bytes.begin(), last});
}

void NetworkDevice::run(std::atomic_flag& stop_requested) {
    std::byte buffer[ethernet_max_size];

    while (!stop_requested.test()) {
        using namespace std::chrono_literals;
        auto read = tap_device.try_read(buffer, 100ms);

        if (read < 0) {
            break;
        }
        if (read < 38) {
            continue;
        }

        std::cout << std::format("[{:%T}] read {} bytes\n", std::chrono::system_clock::now(), read);

        ethernet::Packet packet{std::span{buffer, buffer + read}};

        auto ethertype = packet.get<"ethertype">();

        switch (ethertype) {
        case ethernet::ARP:
            std::cout << "ARP packet\n";
            handle(packet.data<arp::Packet>());
            break;
        case ethernet::IPv4:
            std::cout << "IP packet\n";
            break;
        default:
            std::cout << std::format("Unknown packet {:0>2X}\n", ethertype);
            break;
        }
    }
}