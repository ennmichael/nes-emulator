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
static_assert(-1 == ~0, "Two's complement not used.");
static_assert(Sdl::endianness == Sdl::Endianness::little, "Not little-endian.");

/**
 * TODO I could probably add big-endian support. Currently, I have no way to
 * test this working, so I'm not adding it yet. Ideally, I'd only have to change
 * functions Utils::split_pointer and Utils::create_pointer. The changes should
 * be fairly easy.
 */

/**
 * TODO I shouldn't need to enforce two's complement, I should just write functions
 * for two's complement conversion.
 */

using SignedByte = signed char;
using Byte = unsigned char;
using Bytes = std::vector<Byte>;
using ByteBitset = std::bitset<CHAR_BIT>;

Byte constexpr byte_max = std::numeric_limits<Byte>::max();
SignedByte constexpr signed_byte_max = std::numeric_limits<SignedByte>::max();
SignedByte constexpr signed_byte_min = std::numeric_limits<SignedByte>::min();

namespace Utils {

template <class F, class... Args>
bool constexpr returns_void =
        std::is_same_v<std::invoke_result_t<F, Args...>, void>;

class CantOpenFile : public std::runtime_error {
public:
        explicit CantOpenFile(std::string const& path);
};

int twos_complement(Byte byte) noexcept;

Byte to_byte(ByteBitset bitset) noexcept;

Bytes read_bytes(std::string const& path);
Bytes read_bytes(std::ifstream& ifstream);

bool sign_bit(Byte byte) noexcept;
Byte set_sign_bit(Byte byte, bool value=true) noexcept;
bool zeroth_bit(Byte byte) noexcept;
Byte set_zeroth_bit(Byte byte, bool value=true) noexcept;

bool bit(Byte byte, unsigned bit_num) noexcept;
Byte set_bit(Byte byte, unsigned bit_num, bool value=true) noexcept;

struct BytePair {
        Byte low;
        Byte high;
};

BytePair split_address(unsigned address) noexcept;
unsigned create_address(Byte low, Byte high) noexcept;
unsigned create_address(BytePair byte_pair) noexcept;

}

}

