#pragma once

#include <deque>
#include <thread>
#include <condition_variable>
#include <concepts>
#include <optional>

template <typename T>
class Channel {
    std::deque<T> queue;
    bool is_open = true;
    std::mutex mutex;
    std::condition_variable cv;
public:
    bool push(T value);
    
    template <typename... U>
    bool emplace(U&&... args);

    void close();
    
    std::optional<T> pop();

    class InputIter;
    class Sentinel{};

    InputIter begin();
    Sentinel end();
};

template <typename T>
bool Channel<T>::push(T value) {
    {
        std::lock_guard lock(mutex);
        if (is_open) {
            queue.push_back(std::move(value));
        } else {
            return false;
        }
    }
    cv.notify_one();
    return true;
}

template <typename T>
template <typename... U>
bool Channel<T>::emplace(U&&... args) {
    {
        std::lock_guard lock(mutex);
        if (is_open) {
            queue.emplace_back(std::forward<U>(args)...);
        } else {
            return false;
        }
    }
    cv.notify_one();
    return true;
}

template <typename T>
void Channel<T>::close() {
    {
        std::lock_guard lock(mutex);
        is_open = false;
    }
    cv.notify_all();
}

template <typename T>
std::optional<T> Channel<T>::pop() {
    std::unique_lock lock(mutex);
    cv.wait(lock, [&]{return !(is_open && queue.empty());});

    if (queue.empty()) {
        return std::nullopt;
    } else {
        std::optional<T> value = std::move(queue.front());
        queue.pop_front();

        return value;
    }
}

template <typename T>
class Channel<T>::InputIter {
    friend class Channel;

    Channel& channel;
    std::optional<T> value;

    InputIter(Channel& channel) : channel{channel}, value{channel.pop()} {}
public:
    T& operator*() {
        return *value;
    }
    bool operator==(Channel::Sentinel) {
        return !value.has_value();
    }
    InputIter& operator++() {
        value = channel.pop();
        return *this;
    }
};

template <typename T>
auto Channel<T>::begin() -> InputIter {
    return InputIter(*this);
}

template <typename T>
auto Channel<T>::end() -> Sentinel {
    return Sentinel{};
}
