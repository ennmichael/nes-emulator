#include "mem.h"
#include <utility>

namespace Emulator {

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

