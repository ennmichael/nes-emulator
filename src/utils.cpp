#include "utils.h"
#include "sdl++.h"
#include <climits>

using namespace std::string_literals;

namespace Emulator::Utils {

CantOpenFile::CantOpenFile(std::string const& path)
        : runtime_error("Can't open file "s + path)
{}

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

namespace
{
        unsigned constexpr sign_bit_mask = 0x80u;
        unsigned constexpr zeroth_bit_mask = 0x01u;
}

bool sign_bit(unsigned value) noexcept
{
        return value & sign_bit_mask;
}

unsigned set_sign_bit(unsigned old_value, bool new_bit) noexcept
{
        return set_bits(old_value, sign_bit_mask, new_bit);
}

bool zeroth_bit(unsigned value) noexcept
{
        return value & zeroth_bit_mask;
}

unsigned set_zeroth_bit(unsigned old_value, bool new_bit) noexcept
{
        return set_bits(old_value, zeroth_bit_mask, new_bit);
}

unsigned set_bits(unsigned old_value, unsigned mask, bool value) noexcept
{
        return (value) ?
                old_value | mask :
                old_value & ~mask;
}

namespace
{
        unsigned constexpr low_byte_mask  = 0x00FFu;
        unsigned constexpr high_byte_mask = 0xFF00u;
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

