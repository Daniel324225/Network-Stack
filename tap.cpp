#include "tap.h"
#include <unistd.h>

Tap::Tap(std::string_view) : fd{} {}

Tap::~Tap() {
    if (fd >= 0) {
        close(fd);
    }
}