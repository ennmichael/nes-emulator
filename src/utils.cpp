// vim: set shiftwidth=8 tabstop=8:

#include "utils.h"
#include "sdl++.h"
#include <climits>

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

InvalidRead::InvalidRead(Address address) noexcept
        : runtime_error("Can't read byte at address "s + Utils::format_hex(address))
{}

InvalidWrite:: InvalidWrite(Address address) noexcept
        : runtime_error("Can't write byte to address "s + Utils::format_hex(address))
{}

bool ReadableMemory::address_is_readable(Address address) const noexcept
{
        return address_is_readable_impl(address);
}

Byte ReadableMemory::read_byte(Address address)
{
        if (!address_is_readable(address))
                throw InvalidRead(address);
        return read_byte_impl(address);
}

Address ReadableMemory::read_pointer(Address address)
{
        Byte const low = read_byte(address);
        Byte const high = read_byte(address + 1);
        return Utils::combine_bytes(low, high);
}

bool Memory::address_is_writable(Address address) const noexcept
{
        return address_is_writable_impl(address);
}

void Memory::write_byte(Address address, Byte byte)
{
        if (!address_is_writable(address))
                throw InvalidWrite(address);
        write_byte_impl(address, byte);
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

