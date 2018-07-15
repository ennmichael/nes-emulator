#pragma once

#include "sdl++.h"
#include <climits>
#include <limits>
#include <bitset>
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <type_traits>

namespace Emulator {

static_assert(CHAR_BIT == 8, "Byte not 8 bits.");
static_assert(Sdl::endianness == Sdl::Endianness::little, "Not little-endian.");

/**
 * I could probably add big-endian support. Currently, I have no way to
 * test this working, so I'm not adding it yet. Ideally, I'd only have to change
 * Utils::split_pointer and Utils::create_pointer. The changes should
 * be fairly easy.
 */

template <class T>
using AutoBitset = std::bitset<sizeof(T) * CHAR_BIT>;

using SignedByte = signed char;
using Byte = unsigned char;
using Bytes = std::vector<Byte>;

using ByteBitset = std::bitset<CHAR_BIT>;

Byte constexpr byte_max = std::numeric_limits<Byte>::max();
SignedByte constexpr signed_byte_max = std::numeric_limits<SignedByte>::max();
SignedByte constexpr signed_byte_min = std::numeric_limits<SignedByte>::min();

std::size_t constexpr sign_bit = CHAR_BIT - 1;

namespace TwosComplement {

int encode(Byte value) noexcept;
Byte decode(int value) noexcept;

}

namespace Utils {

// TODO Do I need this?
template <std::size_t N>
auto add_bits(std::bitset<N> b1, std::bitset<N> b2)
{
        return std::bitset<N + 1>(b1.to_ullong() + b2.to_ullong());
}

template <class F, class... Args>
bool constexpr returns_void =
        std::is_same_v<std::invoke_result_t<F, Args...>, void>;

class CantOpenFile : public std::runtime_error {
public:
        explicit CantOpenFile(std::string const& path);
};

std::string format_hex(Byte byte, int width);

template <std::size_t N>
auto to_byte(std::bitset<N> bits) noexcept
{
        return static_cast<Byte>(bits.to_ullong());
}

Bytes read_bytes(std::string const& path);
Bytes read_bytes(std::ifstream& ifstream);

template <class T>
bool bit(T t, unsigned bit_num) noexcept
{
        static_assert(std::is_unsigned_v<T>);

        return AutoBitset<T>(t).test(bit_num);
}

template <class T>
T set_bit(T t, unsigned bit_num, bool value=true) noexcept
{
        static_assert(std::is_unsigned_v<T>);

        AutoBitset<T> bits(t);
        bits.set(bit_num, value);
        return bits.to_ullong();
}

struct BytePair {
        Byte low;
        Byte high;
};

BytePair split_address(unsigned address) noexcept;
unsigned create_address(Byte low, Byte high) noexcept;
unsigned create_address(BytePair byte_pair) noexcept;

}

}

