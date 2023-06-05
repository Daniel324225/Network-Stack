#include <iostream>
#include <expected>
#include <cstdint>
#include "bitformat.h"
#include <format>

void print_bytes(const auto& obj) {
    for (auto it = (unsigned char*)&obj; it != (unsigned char*)(&obj) + sizeof(obj); ++it) {
        std::cout << std::format("|{:0>8b}", *it);
    }

    std::cout << "|";
    if constexpr(std::integral<decltype(obj)>) {
        std::cout << std::format(" {:d}", obj);
    }
    std::cout << "\n";
}

int main() {
    std::array<std::byte, 20> packet{std::byte{0b111'1'1010}, std::byte{0b1111'0000}};
    packet[10] = std::byte{0xAF};

    print_bytes(packet);

    Format
    <
        {"flags", 3},
        {"ofset", 8*10 + 7}
    >format{};

    auto ofset = format.get<"ofset">(packet);
    print_bytes(ofset);

    print_bytes(format.get<"flags">(packet));

    //std::cout << (int)ofset << "\n";
}