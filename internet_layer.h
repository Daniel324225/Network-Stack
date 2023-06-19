#pragma once

#include <thread>

#include "arp.h"
#include "types.h"
#include "link_layer.h"

class InternetLayer : public LinkLayer<InternetLayer>, public arp::Handler<InternetLayer> {
    IPv4_t ip_address;
public:
    InternetLayer(IPv4_t ip_address, LinkLayer link_layer)
        : LinkLayer{std::move(link_layer)},
          ip_address{ip_address}
        {}

    IPv4_t get_ip() {return ip_address;}
    void run(std::stop_token stop_token) {
        LinkLayer::run(stop_token);
    }
};