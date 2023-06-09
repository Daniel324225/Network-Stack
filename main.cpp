#include <iostream>
#include <expected>
#include <cstdint>
#include <format>

#include "packet.h"
#include "tap.h"

int main() {
    Tap tap{};
    Tap tap2{};
    std::byte buffer[100];
    auto read = tap.read(buffer);

    for (auto b : std::span(buffer).subspan(0, read)) {
        std::cout << std::format("|{:0>2x}", int(b));
    }
}