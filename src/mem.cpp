#include "mem.h"
#include <utility>

using namespace std::string_literals;

namespace Emulator {

unsigned ReadableMemory::read_pointer(unsigned address)
{
        Byte const low = read_byte(address);
        Byte const high = read_byte(address + 1);
        return Utils::create_address(low, high);
}

Byte ReadableMemory::deref_byte(unsigned address)
{
        return read_byte(read_pointer(address));
}

unsigned ReadableMemory::deref_pointer(unsigned address)
{
        return read_pointer(read_pointer(address));
}

void Memory::write_pointer(unsigned address, unsigned pointer)
{
        auto const& [low, high] = Utils::split_address(pointer);
        write_byte(address, low);
        write_byte(address + 1, high);
}

}

