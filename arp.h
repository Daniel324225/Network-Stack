#pragma once

#include <vector>
#include <algorithm>
#include <cstddef>
#include <span>
#include <ranges>
#include <iterator>
#include <mutex>
#include <condition_variable>
#include <concepts>
#include <optional>
#include <chrono>

#include "types.h"
#include "packet.h"
#include "ethernet.h"

namespace arp {
    struct Entry {
        IPv4_t ip_address;
        MAC_t mac_address;
    };

    enum OpCode: uint16_t {
        REQUEST = 1,
        REPLY = 2
    };

    using Format = packet::Format<
        {"hardware_type", 16},
        {"protocol_type", 16},
        {"hardware_size", 8},
        {"protocol_size", 8},
        {"opcode", 16},
        {"source_mac", 48},
        {"source_ip", 32},
        {"destination_mac", 48},
        {"destination_ip", 32}
    >;

    template<typename Range>
    struct Packet : packet::Packet<Range, Format>{
        using packet::Packet<Range, Format>::Packet;

        bool is_valid() const {
            return 
                this->bytes.size() == Format::byte_size() && 
                this->template get<"hardware_type">() == 1 &&
                this->template get<"protocol_type">() == 0x0800 && 
                this->template get<"hardware_size">() == 6 &&
                this->template get<"protocol_size">() == 4;
        }
    };

    template<typename Range>
    Packet(Range&& r) -> Packet<Range>;

    template<typename InternetLayer>
    class Handler {
        std::vector<Entry> cache;
        std::mutex mutex;
        std::condition_variable cache_updated;

        InternetLayer& internet_layer() {
            return static_cast<InternetLayer&>(*this);
        }

        bool update(Entry entry) {
            auto it = std::ranges::find(cache, entry.ip_address, &Entry::ip_address);
            if (it != cache.end()) {
                it->mac_address = entry.mac_address;

                return true;
            }
            return false;
        }

        void insert(Entry entry) {
            cache.push_back(entry);
        }
    public:
        Handler() requires(std::derived_from<InternetLayer, Handler>) = default;

        void handle(arp::Packet<std::span<const std::byte>> packet);
        std::optional<MAC_t> resolve(IPv4_t ip);
    };

    template<typename InternetLayer>
    void Handler<InternetLayer>::handle(arp::Packet<std::span<const std::byte>> packet){
        if (!packet.is_valid()) {
            std::cout << "Invalid ARP packet\n";
            return;
        }

        arp::Entry entry = {
            packet.get<"source_ip">(),
            packet.get<"source_mac">()
        };

        std::unique_lock lock(mutex);

        bool merge = update(entry);

        if (internet_layer().get_ip() == packet.get<"destination_ip">()) {
            if (!merge) {
                insert(entry);
            }

            lock.unlock();
            cache_updated.notify_all();
            
            if (packet.get<"opcode">() == arp::REQUEST) {
                arp::Packet<std::array<std::byte, arp::Format::byte_size()>> reply;
                std::ranges::copy(packet.bytes, std::begin(reply.bytes));
                
                const auto source_mac = packet.get<"source_mac">();
                const auto source_ip = packet.get<"source_ip">();

                reply.set<"opcode">(arp::REPLY);
                reply.set<"destination_mac">(source_mac);
                reply.set<"destination_ip">(source_ip);
                reply.set<"source_mac">(internet_layer().get_mac());
                reply.set<"source_ip">(internet_layer().get_ip());

                internet_layer().send(source_mac, ethernet::ARP, reply.bytes);
            }
        } else if (merge) {
            lock.unlock();
            cache_updated.notify_all();
        }
    }

    template<typename InternetLayer>
    std::optional<MAC_t> Handler<InternetLayer>::resolve(IPv4_t ip) {
        std::unique_lock lock(mutex);

        auto it = std::ranges::find(cache, ip, &Entry::ip_address);

        if (it == cache.end()) {
            insert({ip, ethernet::mac_broadcast});

            Packet<std::array<std::byte, Format::byte_size()>> request;
            request.set<"hardware_type">(1);
            request.set<"protocol_type">(0x0800);
            request.set<"hardware_size">(6);
            request.set<"protocol_size">(4);
            request.set<"opcode">(OpCode::REQUEST);
            request.set<"source_mac">(internet_layer().get_mac());
            request.set<"source_ip">(internet_layer().get_ip());
            request.set<"destination_mac">(ethernet::mac_broadcast);
            request.set<"destination_ip">(ip);

            internet_layer().send(ethernet::mac_broadcast, ethernet::ARP, request.to_span());
        } else if (it->mac_address != ethernet::mac_broadcast) {
            return it->mac_address;
        }

        using namespace std::chrono_literals;

        auto success = cache_updated.wait_for(lock, 1s, [&]{
            it = std::ranges::find(cache, ip, &Entry::ip_address);
            return (it != cache.end() && it->mac_address != ethernet::mac_broadcast);
        });

        if (success) {
            return it->mac_address;
        } else {
            cache.erase(it);
            return std::nullopt;
        }
    }
}