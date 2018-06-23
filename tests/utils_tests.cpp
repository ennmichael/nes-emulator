#include "catch.hpp"
#include "../src/utils.h"
#include <vector>
#include <utility>

TEST_CASE("Utils::combine_little_endian works")
{
        struct Example {
                Emulator::Byte lower;
                Emulator::Byte higher;
                unsigned little_endian;
                unsigned big_endian;
        };

        std::vector<Example> const examples {
                {0x00, 0xA1, 161u, 41216u},
                {0x12, 0x34, 13330u, 4660u},
                {0x11, 0xDF, 57105u, 4575u},
                {0x7F, 0x21, 8575u, 32545u}
        };

        for (auto const& [lower, higher, little_endian, big_endian] : examples) {
                unsigned const value =
                        Emulator::Utils::combine_little_endian(lower, higher);

                if constexpr (Sdl::endianness == Sdl::Endianness::little)
                        REQUIRE(value == little_endian);
                else
                        REQUIRE(value == big_endian);
        }
}

