#include "utils.h"
#include "sdl++.h"
#include <climits>
#include <iomanip>

using namespace std::string_literals;

namespace Emulator {

namespace TwosComplement {

int encode(Byte value) noexcept
{
        if (Utils::bit(value, sign_bit))
                return -1 - static_cast<Byte>(~static_cast<unsigned>(value));
        else
                return value;
}

Byte decode(int value) noexcept
{
        if (value < 0)
                return ~static_cast<unsigned>(-value) + 1;
        else
                return value;
}

}

namespace Utils {

namespace {

unsigned constexpr low_byte_mask  = 0x00FFu;
unsigned constexpr high_byte_mask = 0xFF00u;

}

CantOpenFile::CantOpenFile(std::string const& path)
        : runtime_error("Can't open file "s + path)
{}

std::string format_hex(Byte byte, int width)
{
        std::stringstream ss;
        ss << "0x"
           << std::setfill('0')
           << std::setw(width)
           << std::hex
           << static_cast<unsigned>(byte);
        return ss.str();
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

}

