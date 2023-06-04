#include <string_view>

class Tap {
    int fd;

public:
    Tap(std::string_view device_name);
    Tap(const Tap&) = delete;
    Tap(Tap&&) = delete;
    Tap& operator=(const Tap&) = delete;
    Tap& operator=(Tap&&) = delete;
    ~Tap() noexcept;
};