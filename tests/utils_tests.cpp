#include "catch.hpp"
#include "../src/utils.h"
#include <utility>
#include <array>

namespace
{

struct TestExample {
        Emulator::Byte low;
        Emulator::Byte high;
        unsigned address;
};

std::array constexpr examples {
        TestExample {0x00, 0xA1, 0xA100},
        TestExample {0x12, 0x34, 0x3412},
        TestExample {0x11, 0xDF, 0xDF11},
        TestExample {0x7F, 0x21, 0x217F}
};

}

TEST_CASE("Utils::split_address works")
{
        for (auto const& [low, high, address] : examples) {
                auto const& [calculated_low, calculated_high] =
                        Emulator::Utils::split_address(address);

                CHECK(calculated_low == low);
                CHECK(calculated_high == high);
        }
}

TEST_CASE("Utils::create_address works")
{
        for (auto const& [low, high, address] : examples)
                CHECK(Emulator::Utils::create_address(low, high) == address);
}

