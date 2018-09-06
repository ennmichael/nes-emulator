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

Address ReadableMemory::read_pointer(Address address)
{
        Byte const low = read_byte(address);
        Byte const high = read_byte(address + 1);
        return Utils::combine_bytes(low, high);
}

Byte ReadableMemory::deref_byte(Address address)
{
        return read_byte(read_pointer(address));
}

Address ReadableMemory::deref_pointer(Address address)
{
        return read_pointer(read_pointer(address));
}

void Memory::write_pointer(Address address, Address pointer)
{
        auto const& [low, high] = Utils::split_bytes(pointer);
        write_byte(address, low);
        write_byte(address + 1, high);
}

namespace Utils {

CantOpenFile::CantOpenFile(std::string const& path)
        : runtime_error("Can't open file "s + path)
{}

std::string format_hex(unsigned value, int width)
{
        std::stringstream ss;
        ss << "0x"
           << std::setfill('0')
           << std::setw(width)
           << std::hex
           << value;
        return ss.str();
}

std::string format_address(Address address)
{
        return format_hex(address, 4);
}

std::vector<Byte> read_bytes(std::string const& path)
{
        std::ifstream ifstream(path,
                               std::ios_base::in |
                               std::ios_base::binary);

        if (!ifstream.is_open())
                throw CantOpenFile(path);

        return Utils::read_bytes(ifstream);
}

std::vector<Byte> read_bytes(std::ifstream& ifstream)
{
        std::vector<Byte> result;

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

Byte low_byte(Address address) noexcept
{
        return address & 0x00FFu;
}

Byte high_byte(Address address) noexcept
{
        return address >> CHAR_BIT;
}

BytePair split_bytes(Address address) noexcept
{
        return {
                .low = low_byte(address),
                .high = high_byte(address)
        };
}

Address combine_bytes(Byte low, Byte high) noexcept
{
        return static_cast<Address>(low) |
               (static_cast<Address>(high) << CHAR_BIT);
}

Address combine_bytes(BytePair byte_pair) noexcept
{
        auto const& [low, high] = byte_pair;
        return combine_bytes(low, high);
}

}

}

