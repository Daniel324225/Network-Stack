#pragma once

#include <expected>
#include <system_error>
#include <cstddef>
#include <span>
#include <type_traits>

class Tap {
    int fd;
    Tap(int fd);
public:
    Tap();
    std::expected<Tap, std::system_error> try_new() noexcept;
    Tap(const Tap&) = delete;
    Tap(Tap&&) noexcept;
    Tap& operator=(const Tap&) = delete;
    Tap& operator=(Tap&&) noexcept;
    ~Tap() noexcept;

    std::make_signed_t<std::size_t> write(std::span<const std::byte>) noexcept;
    std::make_signed_t<std::size_t> read(std::span<std::byte>) noexcept;
};