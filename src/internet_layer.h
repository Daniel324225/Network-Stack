#pragma once

#include <thread>
#include <chrono>

#include "types.h"
#include "link_layer.h"
#include "arp.h"
#include "ipv4.h"
#include "channel.h"

class InternetLayer : public LinkLayer<InternetLayer>, public arp::Handler<InternetLayer>, public ipv4::Handler<InternetLayer> {
    IPv4_t ip_address;
    IPv4_t gateway;

    Channel<ipv4::Datagram> datagrams;
public:
    InternetLayer(IPv4_t ip_address, IPv4_t gateway, LinkLayer link_layer)
        : LinkLayer{std::move(link_layer)},
          ip_address{ip_address},
          gateway{gateway}
        {}

    IPv4_t get_ip() {return ip_address;}

    template<std::same_as<ipv4::Datagram> T>
    void handle(T&& datagram) {
        datagrams.emplace(std::forward<T>(datagram));
    }
    using arp::Handler<InternetLayer>::handle;
    using ipv4::Handler<InternetLayer>::handle;

    void run(std::stop_token stop_token) {
        std::jthread link_thread([&]{
            LinkLayer::run(stop_token);
            datagrams.close();
        });
        
        using namespace std::chrono_literals;
        using clock = std::chrono::system_clock;
        while (false) {
            std::this_thread::sleep_for(5s);
            auto mac = resolve(gateway);
            if (mac.has_value()) {
                std::cout << std::format("[{:%T}] IP {} is at MAC {}\n", clock::now(), format_ipv4(gateway), format_mac(*mac));
                break;
            } else {
                std::cout << std::format("[{:%T}] Could not resolve IP {}\n", clock::now(), format_ipv4(gateway));
            }
        }
        for (auto datagram : datagrams) {
            std::cout << std::format("[{:%T}] IPv4 datagram with protocol {:0>2x}\n", clock::now(), std::to_underlying(datagram.protocol));
        }
    }
};