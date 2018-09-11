// vim: set shiftwidth=8 tabstop=8:

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

TEST_CASE("Utils::split_bytes works")
{
        for (auto const& [low, high, address] : address_examples) {
                auto const& [calculated_low, calculated_high] =
                        Emulator::Utils::split_bytes(address);

                CHECK(calculated_low == low);
                CHECK(calculated_high == high);
        }
}

TEST_CASE("Utils::combine_bytes works")
{
        for (auto const& [low, high, address] : address_examples)
                CHECK(Emulator::Utils::combine_bytes(low, high) == address);
}

TEST_CASE("TwosComplement::encode works")
{
        struct Example {
                Emulator::Byte byte;
                int value;
        };

        std::array constexpr examples {
                Example {.byte = 0b10000001, .value = -127},
                Example {.byte = 0b10000100, .value = -124},
                Example {.byte = 0b11010101, .value = -43},
                Example {.byte = 0b11111111, .value = -1},
                Example {.byte = 0b00000000, .value = 0},
                Example {.byte = 0b00000001, .value = 1},
                Example {.byte = 0b00000101, .value = 5},
                Example {.byte = 0b01011010, .value = 90}
        };

        for (auto const& [byte, value] : examples) {
                CHECK(Emulator::TwosComplement::encode(byte) == value);
                CHECK(Emulator::TwosComplement::decode(value) == byte);
        }
}

