#include "catch.hpp"
#include "../src/utils.h"
#include <utility>
#include <array>

namespace
{

struct AddressExample {
        Emulator::Byte low;
        Emulator::Byte high;
        unsigned address;
};

std::array constexpr address_examples {
        AddressExample {.low = 0x00, .high = 0xA1, .address = 0xA100},
        AddressExample {.low = 0x12, .high = 0x34, .address = 0x3412},
        AddressExample {.low = 0x11, .high = 0xDF, .address = 0xDF11},
        AddressExample {.low = 0x7F, .high = 0x21, .address = 0x217F}
};

}

TEST_CASE("Utils::split_address works")
{
        for (auto const& [low, high, address] : address_examples) {
                auto const& [calculated_low, calculated_high] =
                        Emulator::Utils::split_address(address);

                CHECK(calculated_low == low);
                CHECK(calculated_high == high);
        }
}

TEST_CASE("Utils::create_address works")
{
        for (auto const& [low, high, address] : address_examples)
                CHECK(Emulator::Utils::create_address(low, high) == address);
}

TEST_CASE("Utils::twos_complement works")
{
        struct Example {
                Emulator::Byte byte;
                int twos_complement;
        };

        std::array constexpr examples {
                Example {.byte = 0b10000001, .twos_complement = -127},
                Example {.byte = 0b10000100, .twos_complement = -124},
                Example {.byte = 0b11010101, .twos_complement = -43},
                Example {.byte = 0b11111111, .twos_complement = -1},
                Example {.byte = 0b00000000, .twos_complement = 0},
                Example {.byte = 0b00000001, .twos_complement = 1},
                Example {.byte = 0b00000101, .twos_complement = 5},
                Example {.byte = 0b01011010, .twos_complement = 90}
        };

        for (auto const& [byte, twos_complement] : examples)
                CHECK(twos_complement == Emulator::Utils::twos_complement(byte));
}

TEST_CASE("Utils::add_bits works")
{
        // TODO
}

