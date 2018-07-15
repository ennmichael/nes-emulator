#include "mem.h"
#include <utility>

using namespace std::string_literals;

namespace Emulator {

Memory::InvalidAddress::InvalidAddress(unsigned address,
                                       std::string const& access) noexcept
        : runtime_error("Can't "s + access + " "s + Utils::format_hex(address, 4))
{}

void Memory::write_byte(unsigned address, Byte byte)
{
        if (!address_is_writable(address))
                throw InvalidAddress(address, "read"s);
        do_write_byte(address, byte);
}

Byte Memory::read_byte(unsigned address) const
{
        if (!address_is_readable(address))
                throw InvalidAddress(address, "write"s);
        return do_read_byte(address);
}

void Memory::write_pointer(unsigned address, unsigned pointer)
{
        auto const& [low, high] = Utils::split_address(pointer);
        write_byte(address, low);
        write_byte(address + 1, high);
}

unsigned Memory::read_pointer(unsigned address) const
{
        Byte const low = read_byte(address);
        Byte const high = read_byte(address + 1);
        return Utils::create_address(low, high);
}

Byte Memory::deref_byte(unsigned address) const
{
        return read_byte(read_pointer(address));
}

unsigned Memory::deref_pointer(unsigned address) const
{
        return read_pointer(read_pointer(address));
}

}

