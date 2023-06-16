#include <format>
#include <chrono>
#include <iostream>

#include "packet.h"
#include "network_device.h"
#include "ethernet.h"

void NetworkDevice::handle(Arp::Packet<const std::byte> packet) {
    if (not packet.is_valid()) {
        std::cout << "Invalid ARP packet\n";
        return;
    }

    Arp::Entry entry = {
        packet.get<"source_ip">(),
        packet.get<"source_mac">()
    };

    bool merge = arp_cache.update(entry);

    if (ip_address == packet.get<"destination_ip">()) {
        if (!merge) {
            arp_cache.insert(entry);
        }
        
        if (packet.get<"opcode">() == Arp::REQUEST) {
            std::array<std::byte, Arp::Format::byte_size()> reply_buffer;
            std::ranges::copy(packet.bytes, std::begin(reply_buffer));

            Arp::Packet reply(reply_buffer);
            
            const auto source_mac = packet.get<"source_mac">();
            const auto source_ip = packet.get<"source_ip">();

            reply.set<"opcode">(Arp::REPLY);
            reply.set<"destination_mac">(source_mac);
            reply.set<"destination_ip">(source_ip);
            reply.set<"source_mac">(mac_address);
            reply.set<"source_ip">(ip_address);

            send(source_mac, Ethernet::ARP, reply_buffer);
        }
    }
}

static constexpr std::size_t ethernet_max_size = 1522;

void NetworkDevice::send(MAC_t destination, Ethernet::Ethertype ethertype, std::span<const std::byte> payload) {
    std::array<std::byte, ethernet_max_size> buffer;

    Ethernet::Packet packet{buffer};

    packet.set<"dmac">(destination);
    packet.set<"smac">(mac_address);
    packet.set<"ethertype">(ethertype);

    auto [_, last] = std::ranges::copy(payload, packet.data().begin());

    tap_device.write({packet.bytes.begin(), last});
}

void NetworkDevice::run() {
    std::byte buffer[ethernet_max_size];

    while (true) {
        auto read = tap_device.read(buffer);

        std::cout << std::format("[{:%T}] read {} bytes\n", std::chrono::system_clock::now(), read);
        if (read < 0) {
            break;
        }
        if (read < 38) {
            continue;
        }

        Ethernet::Packet packet{std::span{buffer, buffer + read}};

        auto ethertype = packet.get<"ethertype">();

        switch (ethertype) {
        case Ethernet::ARP:
            std::cout << "ARP packet\n";
            handle(packet.data<Arp::Packet>());
            break;
        case Ethernet::IPv4:
            std::cout << "IP packet\n";
            break;
        default:
            std::cout << std::format("Unknown packet {:0>2X}\n", ethertype);
            break;
        }
    }
}