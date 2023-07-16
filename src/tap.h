#pragma once

#include <expected>
#include <system_error>
#include <cstddef>
#include <span>
#include <type_traits>
#include <string>
#include <chrono>

class Tap {
    int fd = -1;
    std::string name;
    Tap(int fd, std::string name);
public:
    Tap();
    static std::expected<Tap, std::system_error> try_new() noexcept;
    Tap(const Tap&) = delete;
    Tap(Tap&&) noexcept;
    Tap& operator=(Tap) noexcept;
    ~Tap() noexcept;

    std::make_signed_t<std::size_t> write(std::span<const std::byte>) noexcept;
    std::make_signed_t<std::size_t> read(std::span<std::byte>) noexcept;
    std::make_signed_t<std::size_t> try_read(std::span<std::byte>, std::chrono::duration<int, std::milli> timeout) noexcept;

    const std::string& get_name() const noexcept {
        return name;
    }
};