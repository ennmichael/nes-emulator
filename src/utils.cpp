#include "utils.h"
#include "sdl++.h"
#include <climits>

using namespace std::string_literals;

namespace Emulator::Utils {

CantOpenFile::CantOpenFile(std::string const& path)
        : runtime_error("Can't open file "s + path)
{}

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

unsigned combine_little_endian(Byte low, Byte high) noexcept
{
        if constexpr (Sdl::endianness == Sdl::Endianness::big)
                std::swap(low, high);

        return static_cast<unsigned>(low) &
               (static_cast<unsigned>(high) >> CHAR_BIT);
}

int twos_complement(Byte b) noexcept
{
        return (b < 128) ? b : static_cast<int>(b) - byte_max;
}

}

