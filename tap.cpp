#include <utility>
#include <exception>
#include <system_error>
#include <algorithm>
#include <cstring>
#include <cerrno>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <poll.h>

#include "tap.h"

Tap::Tap() {
    auto tap = try_new();
    if (tap.has_value()) {
        *this = std::move(*tap);
    } else {
        throw tap.error();
    }
}

std::expected<Tap, std::system_error> Tap::try_new() noexcept {
    int fd = open("/dev/net/tap", O_RDWR);
    if (fd < 0) {
        return std::unexpected{
            std::system_error{errno, std::system_category(), "Cannot open TUN/TAP device"}
        };
    }

    ifreq ifr{};
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    if ((ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
        close(fd);
        return std::unexpected{
            std::system_error{errno, std::system_category(), "Could not ioctl TUN device"}
        };
    }

    return Tap{fd, ifr.ifr_name};
}

Tap::Tap(int fd, std::string name) : fd{fd}, name{name} {}

Tap::Tap(Tap&& other) noexcept : name{std::move(other.name)} {
    fd = std::exchange(other.fd, -1);
}

Tap& Tap::operator=(Tap other) noexcept {
    std::swap(fd, other.fd);
    std::swap(name, other.name);

    return *this;
}

Tap::~Tap() {
    if (fd >= 0) {
        close(fd);
    }
}

std::make_signed_t<std::size_t> Tap::write(std::span<const std::byte> buffer) noexcept {
    return ::write(fd, buffer.data(), buffer.size());
}

std::make_signed_t<std::size_t> Tap::read(std::span<std::byte> buffer) noexcept {
    return ::read(fd, buffer.data(), buffer.size());
}

std::make_signed_t<std::size_t> Tap::try_read(std::span<std::byte> buffer, std::chrono::duration<int, std::milli> timeout) noexcept {
    pollfd poll_fd{fd, POLLIN, 0};
    if (auto result = poll(&poll_fd, 1, timeout.count()); result <= 0) {
        return result;
    }
    if ((poll_fd.revents & POLLIN) == 0) {
        return -1;
    }
    return ::read(fd, buffer.data(), buffer.size());
}