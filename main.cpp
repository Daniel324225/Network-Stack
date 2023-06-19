#include <iostream>
#include <expected>
#include <cstdint>
#include <format>
#include <atomic>
#include <thread>
#include <csignal>

#include "packet.h"
#include "tap.h"
#include "internet_layer.h"

static std::atomic_flag request_stop;

int main(int argc, char* argv[]) {
    std::signal(SIGINT, [](int){
        request_stop.test_and_set();
        request_stop.notify_all();
    });

    auto ip = parse_ipv4(argc > 1 ? argv[1] : "10.0.0.4");
    if (!ip.has_value()) {
        std::cout << "Invalid IP\n";
        return 1;
    }
    auto gateway = parse_ipv4(argc > 2 ? argv[2] : "10.0.0.1");
    if (!ip.has_value()) {
        std::cout << "Invalid gateway IP\n";
        return 1;
    }
    auto mac = parse_mac(argc > 3 ? argv[3] : "00:0c:29:6d:50:25");
    if (!mac.has_value()) {
        std::cout << "Invalid MAC\n";
        return 1;
    }
    
    auto tap_device = Tap::try_new();

    if (!tap_device.has_value()) {
        std::cout << std::format("Failed to create TAP device\n{}\n", tap_device.error().what());
        return 1;
    }
    std::cout << std::format("Created TAP device {}\n", tap_device->get_name());

    InternetLayer dev(*ip, *gateway, {*mac, std::move(*tap_device)});

    std::jthread thread([&](std::stop_token stop_token){
        dev.run(stop_token);
    });

    request_stop.wait(false);
}