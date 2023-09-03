#pragma once

#include <thread>
#include <algorithm>
#include <span>
#include <cstddef>
#include <concepts>

#include "types.h"
#include "ethernet.h"
#include "arp.h"
#include "ipv4.h"
#include "tap.h"

template <typename InternetLayer>
class LinkLayer {
    MAC_t mac_address;
    Tap tap_device;

    InternetLayer& internet_layer() {
        return static_cast<InternetLayer&>(*this);
    }
public:
    LinkLayer(MAC_t mac_address, Tap tap_device)
        requires(std::derived_from<InternetLayer, LinkLayer>)
        : mac_address{mac_address}, 
          tap_device{std::move(tap_device)}
        {}

    MAC_t get_mac() {return mac_address;}
    void send(MAC_t destination, ethernet::Ethertype ethertype, std::span<const std::byte> payload);
    void run(std::stop_token stop_token);
};

template <typename InternetLayer>
void LinkLayer<InternetLayer>::send(MAC_t destination, ethernet::Ethertype ethertype, std::span<const std::byte> payload) {
    ethernet::Packet<std::array<std::byte, ethernet::max_size>> packet;

    packet.set<"destination_mac">(destination);
    packet.set<"source_mac">(mac_address);
    packet.set<"ethertype">(ethertype);

    auto [_, last] = std::ranges::copy(payload, packet.data().begin());

    tap_device.write({packet.to_span().begin(), last});
}

template <typename InternetLayer>
void LinkLayer<InternetLayer>::run(std::stop_token stop_token) {
        std::byte buffer[ethernet::max_size];

    while (!stop_token.stop_requested()) {
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

        if (auto destination = packet.get<"destination_mac">(); destination != mac_address && destination != ethernet::mac_broadcast) {
            continue;
        }

        auto ethertype = ethernet::Ethertype{packet.get<"ethertype">()};

        switch (ethertype) {
        case ethernet::Ethertype::ARP:
            std::cout << "ARP packet\n";
            internet_layer().handle(packet.data<arp::Packet>());
            break;
        case ethernet::Ethertype::IPv4:
            std::cout << "IP packet\n";
            internet_layer().handle(packet.data<ipv4::Packet>());
            break;
        default:
            std::cout << std::format("Unknown packet {:0>2X}\n", std::to_underlying(ethertype));
            break;
        }
    }
}
