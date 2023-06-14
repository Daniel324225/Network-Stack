#include <format>
#include <chrono>
#include <iostream>

#include "packet.h"
#include "network_device.h"
#include "ethernet.h"

void NetworkDevice::handle_arp_packet(std::span<const std::byte> packet) {
    if (!Arp::is_valid(packet)) {
        std::cout << "Invalid ARP packet\n";
        return;
    }

    Arp::Entry entry = {
        Arp::Format::get<"source_ip">(packet),
        Arp::Format::get<"source_mac">(packet)
    };

    bool merge = arp_cache.update(entry);

    if (ip_address == Arp::Format::get<"destination_ip">(packet)) {
        if (!merge) {
            arp_cache.insert(entry);
        }
        
        if (Arp::Format::get<"opcode">(packet) == static_cast<uint16_t>(Arp::OpCode::REQUEST)) {
            std::byte reply[Arp::Format::byte_size()];
            std::ranges::copy(packet, std::begin(reply));
            
            const auto source_mac =  Arp::Format::get<"source_mac">(packet);
            const auto source_ip =  Arp::Format::get<"source_ip">(packet);

            Arp::Format::set<"opcode">(reply, static_cast<uint16_t>(Arp::OpCode::REPLY));
            Arp::Format::set<"destination_mac">(reply, source_mac);
            Arp::Format::set<"destination_ip">(reply, source_ip);
            Arp::Format::set<"source_mac">(reply, mac_address);
            Arp::Format::set<"source_ip">(reply, ip_address);

            send(source_mac, Ethertype::ARP, reply);
        }
    }
}

static constexpr std::size_t ethernet_max_size = 1522;

void NetworkDevice::send(MAC_t destination, Ethertype ethertype, std::span<const std::byte> payload) {
    std::byte buffer[ethernet_max_size];

    EthernetHeader::set<"dmac">(buffer, destination);
    EthernetHeader::set<"smac">(buffer, mac_address);
    EthernetHeader::set<"ethertype">(buffer, static_cast<uint16_t>(ethertype));

    auto [_, last] = std::ranges::copy(payload, buffer + EthernetHeader::byte_size());

    tap_device.write({buffer, last});
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

        auto data = std::span{buffer + EthernetHeader::byte_size(), buffer + read};
        auto ethertype = EthernetHeader::get<"ethertype">(buffer);

        switch (static_cast<Ethertype>(ethertype))
        {
        case Ethertype::ARP:
            std::cout << "ARP packet\n";
            handle_arp_packet(data);
            break;
        case Ethertype::IPv4:
            std::cout << "IP packet\n";
            break;
        default:
            std::cout << std::format("Unknown packet {:0>2X}\n", ethertype);
            break;
        }
    }
}