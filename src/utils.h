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
 * TODO Using unsigned for addresses instead of std::uint_16 is a big mistake.
 * Fix it.
 */

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
using Bytes = std::vector<Byte>; // TODO Rename -> ByteVector
// TODO Have ByteArray

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

template <class F, class... Args>
bool constexpr returns_void =
        std::is_same_v<std::invoke_result_t<F, Args...>, void>;

class CantOpenFile : public std::runtime_error {
public:
        explicit CantOpenFile(std::string const& path);
};

std::string format_hex(unsigned value, int width);
std::string format_address(unsigned address);

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

unsigned constexpr low_byte_mask  = 0x00FFu;
unsigned constexpr high_byte_mask = 0xFF00u;

struct BytePair {
        Byte low;
        Byte high;
};

BytePair split_bytes(unsigned two_bytes) noexcept;
unsigned combine_bytes(Byte low, Byte high) noexcept;
unsigned combine_bytes(BytePair byte_pair) noexcept;

inline BytePair split_address(unsigned address) noexcept
{
        return split_bytes(address);
}

inline unsigned create_address(Byte low, Byte high) noexcept
{
        return combine_bytes(low, high);
}

inline unsigned create_address(BytePair byte_pair) noexcept
{
        return combine_bytes(byte_pair);
}

}

}

