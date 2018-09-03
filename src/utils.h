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

namespace Emulator {

enum class Endianness {
        big = SDL_BIG_ENDIAN,
        little = SDL_LIL_ENDIAN
};

Endianness constexpr endianness {SDL_BYTEORDER}; 

static_assert(endianness == Endianness::little, "Not little-endian.");

/**
 * TODO Using unsigned for addresses instead of std::uint_16 is a big mistake.
 * Fix it.
 */

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
using Address = std::uint16_t; // TODO Start using this (???)
using ByteBitset = std::bitset<CHAR_BIT>;
template <class T, auto W, auto H>
using Matrix = std::array<std::array<T, W>, H>;

Byte constexpr byte_max = std::numeric_limits<Byte>::max();
SignedByte constexpr signed_byte_max = std::numeric_limits<SignedByte>::max();
SignedByte constexpr signed_byte_min = std::numeric_limits<SignedByte>::min();

std::size_t constexpr sign_bit = CHAR_BIT - 1;

namespace TwosComplement {

int encode(Byte value) noexcept;
Byte decode(int value) noexcept;

}

class InvalidAddress : public std::runtime_error {
public:
        using runtime_error::runtime_error;
};

class ReadableMemory {
public:
        ReadableMemory() = default;
        ReadableMemory(ReadableMemory const&) = delete;
        ReadableMemory(ReadableMemory&&) = delete;
        ReadableMemory& operator=(ReadableMemory const&) = delete;
        ReadableMemory& operator=(ReadableMemory&&) = delete;
        virtual ~ReadableMemory() = default;

        virtual bool address_is_readable(unsigned address) const noexcept = 0;
        virtual Byte read_byte(unsigned address) = 0;
        unsigned read_pointer(unsigned address);
        unsigned deref_pointer(unsigned address);
        Byte deref_byte(unsigned address);
};

class Memory : public ReadableMemory {
public:
        virtual bool address_is_writable(unsigned address) const noexcept = 0;
        virtual void write_byte(unsigned address, Byte byte) = 0;
        void write_pointer(unsigned address, unsigned pointer);
        // TODO instead of doing checks in read_memory and write_memory,
        // how about read_memory_safe and write_memory_safe member functions?
};

using UniqueReadableMemory = std::unique_ptr<ReadableMemory>;
using UniqueMemory = std::unique_ptr<Memory>;

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

Byte low_byte(unsigned two_bytes) noexcept;
Byte high_byte(unsigned two_bytes) noexcept;
BytePair split_bytes(unsigned two_bytes) noexcept;
unsigned combine_bytes(Byte low, Byte high) noexcept;
unsigned combine_bytes(BytePair byte_pair) noexcept;

}

}

