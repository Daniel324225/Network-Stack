#include <iostream>
#include <expected>
#include <cstdint>
#include <format>

#include "packet.h"
#include "tap.h"
#include "network_device.h"

int main() {
    NetworkDevice dev{"00:0c:29:6d:50:25"_mac, "10.0.0.4"_ipv4, Tap{}};
    dev.run();
}