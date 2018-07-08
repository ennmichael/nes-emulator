#include "utils.h"
#include "sdl++.h"
#include <climits>

using namespace std::string_literals;

namespace Emulator::Utils {

namespace {

unsigned constexpr sign_bit_num = 7u;
unsigned constexpr low_byte_mask  = 0x00FFu;
unsigned constexpr high_byte_mask = 0xFF00u;

}

CantOpenFile::CantOpenFile(std::string const& path)
        : runtime_error("Can't open file "s + path)
{}

std::string format_hex(Byte byte)
{
        std::stringstream ss;

        if (byte <= 0x0F)
                ss << "0x0" << std::hex << static_cast<unsigned>(byte);
        else
                ss << "0x" << std::hex << static_cast<unsigned>(byte);

        return ss.str();
}

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
        return bit(byte, sign_bit_num);
}

Byte set_sign_bit(Byte byte, bool value) noexcept
{
        return set_bit(byte, sign_bit_num, value);
}

bool zeroth_bit(Byte byte) noexcept
{
        return bit(byte, 0);
}

Byte set_zeroth_bit(Byte byte, bool value) noexcept
{
        return set_bit(byte, 0, value);
}

bool bit(Byte byte, unsigned bit_num) noexcept
{
        return ByteBitset(byte).test(bit_num);
}

Byte set_bit(Byte byte, unsigned bit_num, bool value) noexcept
{
        ByteBitset bits = byte;
        bits.set(bit_num, value);
        return to_byte(bits);
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

