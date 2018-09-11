// vim: set shiftwidth=8 tabstop=8:

#pragma once

#include "SDL2/SDL_endian.h"
#include <climits>
#include <cstdint>
#include <limits>
#include <bitset>
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <type_traits>
#include <array>
#include <memory>
#include <iomanip>
#include <sstream>

namespace Emulator {

enum class Endianness {
        big = SDL_BIG_ENDIAN,
        little = SDL_LIL_ENDIAN
};

Endianness constexpr endianness {SDL_BYTEORDER}; 

static_assert(endianness == Endianness::little, "Not little-endian.");

/**
 * I could probably add big-endian support. Currently, I have no way to
 * test this working, so I'm not adding it yet. Ideally, I'd only have to change
 * Utils::split_byte and Utils::combine_bytes. The changes should
 * be fairly easy.
 */

template <class T>
using AutoBitset = std::bitset<sizeof(T) * CHAR_BIT>;
using SignedByte = std::int8_t;
using Byte = std::uint8_t;
using Address = std::uint16_t;
using ByteBitset = std::bitset<CHAR_BIT>;
template <class T, std::size_t W, std::size_t H>
using Matrix = std::array<std::array<T, W>, H>;

Byte constexpr byte_max = std::numeric_limits<Byte>::max();
SignedByte constexpr signed_byte_max = std::numeric_limits<SignedByte>::max();
SignedByte constexpr signed_byte_min = std::numeric_limits<SignedByte>::min();

std::size_t constexpr sign_bit = CHAR_BIT - 1;

namespace TwosComplement {

int encode(Byte value) noexcept;
Byte decode(int value) noexcept;

}

class InvalidWrite : public std::runtime_error {
public:
        explicit InvalidWrite(Address address) noexcept;
};

class InvalidRead : public std::runtime_error {
public:
        explicit InvalidRead(Address address) noexcept;
};

class ReadableMemory {
public:
        ReadableMemory() = default;
        ReadableMemory(ReadableMemory const&) = delete;
        ReadableMemory(ReadableMemory&&) = delete;
        ReadableMemory& operator=(ReadableMemory const&) = delete;
        ReadableMemory& operator=(ReadableMemory&&) = delete;
        virtual ~ReadableMemory() = default;

        bool address_is_readable(Address address) const noexcept;
        Byte read_byte(Address address);
        Address read_pointer(Address address);

protected:
        virtual bool address_is_readable_impl(Address address) const noexcept = 0;
        virtual Byte read_byte_impl(Address address) = 0;
};

class Memory : public ReadableMemory {
public:
        bool address_is_writable(Address address) const noexcept;
        void write_byte(Address address, Byte byte);
        void write_pointer(Address address, Address pointer);

protected:
        virtual bool address_is_writable_impl(Address address) const noexcept = 0;
        virtual void write_byte_impl(Address, Byte byte) = 0;
};

namespace Utils {

class CantOpenFile : public std::runtime_error {
public:
        explicit CantOpenFile(std::string const& path);
};

template <class T>
std::string format_hex(T value, int width = sizeof(T)*2)
{
        std::stringstream ss;
        ss << "0x"
           << std::setfill('0')
           << std::setw(width)
           << std::hex
           << value;
        return ss.str();
}

std::vector<Byte> read_bytes(std::string const& path);
std::vector<Byte> read_bytes(std::ifstream& ifstream);

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

Byte low_byte(Address address) noexcept;
Byte high_byte(Address address) noexcept;
BytePair split_bytes(Address address) noexcept;
Address combine_bytes(Byte low, Byte high) noexcept;
Address combine_bytes(BytePair byte_pair) noexcept;

}

}

