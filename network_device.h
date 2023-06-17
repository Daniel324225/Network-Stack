#pragma once

#include <span>
#include <atomic>

#include "utils.h"
#include "tap.h"
#include "types.h"
#include "arp.h"
#include "ethernet.h"

class NetworkDevice {
    MAC_t mac_address;
    IPv4_t ip_address;
    Tap tap_device;
    arp::Cache arp_cache;

    void handle(arp::Packet<const std::byte> packet);
    void send(MAC_t destination, ethernet::Ethertype ethertype, std::span<const std::byte> payload);
public:
    void run(std::atomic_flag& stop_requested);
    NetworkDevice(MAC_t mac_address, IPv4_t ip_address, Tap tap_device) : mac_address{mac_address}, ip_address{ip_address}, tap_device{std::move(tap_device)} {}
};