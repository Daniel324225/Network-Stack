#include <algorithm>

#include "ipv4.h"

namespace ipv4 {
    void Assembler::remove_older_then(clock::duration age) {
        auto time_point = clock::now() - age;
        while (!queue.empty() && queue.front().first < time_point)
        {
            datagrams.erase(queue.front().second);
            queue.pop();
        }
    }

    std::optional<Datagram> Assembler::assemble(Packet<std::span<const std::byte>> packet) {
        using namespace std::chrono_literals;
        remove_older_then(1s);

        std::optional<std::vector<std::byte>> data;

        if (packet.get<"more_fragments">() == 0 && packet.get<"fragment_offset">() == 0) {
            data = std::vector<std::byte>{packet.payload().begin(), packet.payload().end()};
        } else {
            Key key {
                packet.get<"source_address">(),
                packet.get<"destination_address">(),
                Protocol{packet.get<"protocol">()},
                packet.get<"id">()
            };

            auto& datagram = datagrams[key];

            if (datagram.offsets_received.empty()) {
                queue.push({clock::now(), key});
            }

            auto fragment_offset = packet.get<"fragment_offset">();
            if (!datagram.offsets_received.insert(fragment_offset).second) {
                return std::nullopt;
            }

            std::size_t begin = fragment_offset * 8;
            std::size_t length = packet.payload().size();
            std::size_t end = begin + length;

            datagram.data.resize(std::max(datagram.data.size(), end));
            std::ranges::copy(packet.payload(), datagram.data.begin() + begin);
            datagram.bytes_received += length;

            if (packet.get<"more_fragments">() == 0) {
                datagram.total_size = end;
            }

            if (datagram.total_size == datagram.bytes_received) {
                data = std::move(datagram.data);
                datagrams.erase(key);
            }
        }

        return data.transform([&](auto& data) {
            return Datagram {
                packet.get<"source_address">(),
                packet.get<"destination_address">(),
                Protocol{packet.get<"protocol">()},
                std::move(data)
            };
        });
    }
}