#include "utils.h"
#include "sdl++.h"
#include <climits>

using namespace std::string_literals;

namespace Emulator::Utils {

namespace {

Byte constexpr sign_bit_mask = 0x80;
Byte constexpr zeroth_bit_mask = 0x01;
unsigned constexpr low_byte_mask  = 0x00FFu;
unsigned constexpr high_byte_mask = 0xFF00u;

}

CantOpenFile::CantOpenFile(std::string const& path)
        : runtime_error("Can't open file "s + path)
{}

int twos_complement(Byte byte) noexcept
{
        if (sign_bit(byte))
                return -1 - set_sign_bit(~byte, false);
        else
                return set_sign_bit(byte, false);
}

Byte to_byte(ByteBitset bitset) noexcept
{
        return static_cast<Byte>(bitset.to_ulong());
}

Bytes read_bytes(std::string const& path)
{
        std::ifstream ifstream(path,
                             std::ios_base::in |
                             std::ios_base::binary);

        if (!ifstream.is_open())
                throw CantOpenFile(path);

        return Utils::read_bytes(ifstream);
}

Bytes read_bytes(std::ifstream& ifstream)
{
        Bytes result;

        for (;;) {
                Byte const b = ifstream.get();
                if (ifstream.eof())
                        break;
                result.push_back(b);
        }

        // Maybe this code would be faster if I used reserve,
        // or if I read the file multiple bytes at a time.

        return result;
}

bool sign_bit(Byte byte) noexcept
{
        return byte & sign_bit_mask;
}

Byte set_sign_bit(Byte byte, bool value) noexcept
{
        return set_bits(byte, sign_bit_mask, value);
}

bool zeroth_bit(Byte byte) noexcept
{
        return byte & zeroth_bit_mask;
}

Byte set_zeroth_bit(Byte byte, bool value) noexcept
{
        return set_bits(byte, zeroth_bit_mask, value);
}

Byte set_bits(Byte byte, Byte mask, bool value) noexcept
{
        return (value) ?
                byte | mask :
                byte & ~mask;
}

BytePair split_address(unsigned pointer) noexcept
{
        return {
                .low = static_cast<Byte>(pointer & low_byte_mask),
                .high = static_cast<Byte>((pointer & high_byte_mask) >> CHAR_BIT)
        };
}

unsigned create_address(Byte low, Byte high) noexcept
{
        return static_cast<unsigned>(low) |
               (static_cast<unsigned>(high) << CHAR_BIT);
}

unsigned create_address(BytePair byte_pair) noexcept
{
        auto const& [low, high] = byte_pair;
        return create_address(low, high);
}

}

