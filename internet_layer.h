#pragma once

#include <thread>

#include "ethernet.h"
#include "arp.h"
#include "types.h"
#include "link_layer.h"

class InternetLayer : public LinkLayer<InternetLayer> {
    IPv4_t ip_address;
    arp::Cache arp_cache;
public:
    InternetLayer(IPv4_t ip_address, LinkLayer link_layer)
        : LinkLayer{std::move(link_layer)},
          ip_address{ip_address}
        {}

    void handle(arp::Packet<const std::byte> packet);
    void run(std::stop_token stop_token) {
        LinkLayer::run(stop_token);
    }
};

inline void InternetLayer::handle(arp::Packet<const std::byte> packet) {
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
            reply.set<"source_mac">(get_mac());
            reply.set<"source_ip">(ip_address);

            send(source_mac, ethernet::ARP, reply_buffer);
        }
    }
}