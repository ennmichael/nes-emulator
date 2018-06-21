#include "utils.h"

using namespace std::string_literals;

namespace Emulator {

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

        return read_bytes(ifstream);
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

}

