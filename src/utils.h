#pragma once

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

using SignedByte = signed char;
using Byte = unsigned char;
using Bytes = std::vector<Byte>;
using ByteBitset = std::bitset<CHAR_BIT>;

Byte constexpr byte_max = std::numeric_limits<Byte>::max();
SignedByte constexpr signed_byte_max = std::numeric_limits<SignedByte>::max();
SignedByte constexpr signed_byte_min = std::numeric_limist<SignedByte>::min();

namespace Utils {

template <class F, class... Args>
bool constexpr returns_void =
        std::is_same_v<std::invoke_result_t<F, Args...>, void>;

class CantOpenFile : public std::runtime_error {
public:
        explicit CantOpenFile(std::string const& path);
};

Bytes read_bytes(std::string const& path);
Bytes read_bytes(std::ifstream& ifstream);

unsigned combine_little_endian(Byte low, Byte high) noexcept;

bool sign_bit(unsigned value) noexcept;
unsigned sign_bit(unsigned old_value, bool new_bit) noexcept;
bool zeroth_bit(unsigned value) noexcept;
unsigned zeroth_bit(unsigned old_value, bool new_bit) noexcept;

unsigned set_bit(unsigned old_value, unsigned mask, bool new_bit) noexcept;

}

}

