#include <iostream>
#include <expected>
#include <cstdint>
#include "bitformat.h"

int main() {

    Format
    <
        {"dmac", 8},
        {"a",    12}
    >{}.get<"dmac">({});

}