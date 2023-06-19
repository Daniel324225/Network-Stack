#pragma once

#include <thread>
#include <chrono>

#include "arp.h"
#include "types.h"
#include "link_layer.h"

class InternetLayer : public LinkLayer<InternetLayer>, public arp::Handler<InternetLayer> {
    IPv4_t ip_address;
    IPv4_t gateway;
public:
    InternetLayer(IPv4_t ip_address, IPv4_t gateway, LinkLayer link_layer)
        : LinkLayer{std::move(link_layer)},
          ip_address{ip_address},
          gateway{gateway}
        {}

    IPv4_t get_ip() {return ip_address;}
    void run(std::stop_token stop_token) {
        std::jthread link_thread([&]{
            LinkLayer::run(stop_token);
        });
        
        using namespace std::chrono_literals;
        while (!stop_token.stop_requested())
        {
            std::this_thread::sleep_for(5s);
            auto mac = resolve(gateway);
            if (mac.has_value()) {
                std::cout << std::format("[{:%T}] IP {} is at MAC {}\n", std::chrono::system_clock::now(), format_ipv4(gateway), format_mac(*mac));
                break;
            } else {
                std::cout << std::format("[{:%T}] Could not resolve IP {}\n", std::chrono::system_clock::now(), format_ipv4(gateway));
            }
        }
        while (!stop_token.stop_requested()) {
            std::this_thread::sleep_for(1s);
        }
    }
};