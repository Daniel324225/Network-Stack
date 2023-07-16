#include <thread>
#include <utility>
#include <vector>
#include <latch>
#include <algorithm>
#include <set>

#include "gtest/gtest.h"

#include "channel.h"

struct Type {
    int thread_id;
    int value;

    auto operator<=>(const Type&) const = default;
};

struct ChannelTest : public testing::TestWithParam<std::pair<int, int>> {
    Channel<Type> channel;
    int writing_threads;
    int reading_threads;

    ChannelTest() {
        std::tie(writing_threads, reading_threads) = GetParam();
    }
};

TEST_P(ChannelTest, All) {
    int values_per_thread = 100;

    std::vector<std::thread> writing;
    std::vector<std::thread> reading;

    std::vector<std::vector<Type>> read_values(reading_threads);
    std::vector<Type> expected;

    std::latch latch(writing_threads + reading_threads);

    for (int id = 0; id < writing_threads; ++id) {
        writing.push_back(std::thread{[&, id]{
            latch.arrive_and_wait();

            for (int i = 0; i < values_per_thread; i += 2) {
                channel.push({id, i});
                channel.emplace(id, i + 1);
            }
        }});

        for (int i = 0; i < values_per_thread; i += 2) {
            expected.push_back({id, i});
            expected.push_back({id, i + 1});
        }
    }

    for (int id = 0; id < reading_threads; ++id) {
        reading.push_back(std::thread{[&, id]{
            latch.arrive_and_wait();

            for (auto v : channel) {
                read_values[id].push_back(v);
            }
        }});
    }

    for (auto& t : writing) t.join();
    channel.close();
    for (auto& t : reading) t.join();

    std::set<Type> set;

    for (auto& values : read_values) {
        std::ranges::stable_sort(values, {}, &Type::thread_id);
        EXPECT_TRUE(std::ranges::is_sorted(values));
        set.insert(values.begin(), values.end());
    }

    EXPECT_TRUE(
        std::ranges::equal(
            set,
            expected
        )
    );
}

INSTANTIATE_TEST_SUITE_P(
    Channel,
    ChannelTest,
    testing::Values(std::pair{1, 1}, std::pair{1, 4}, std::pair{4, 1}, std::pair{4, 4})
);
