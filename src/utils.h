#pragma once

#include <climits>
#include <limits>
#include <bitset>
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>

namespace Emulator {

static_assert(CHAR_BIT == 8, "Byte not 8 bits.");

// TODO Perhaps some of these names should be in a `Utils` namespace?

// TODO Have a Bytes typedef
using Byte = unsigned char;
using Bytes = std::vector<Byte>;
using ByteBitset = std::bitset<CHAR_BIT>;

auto constexpr byte_max = std::numeric_limits<Byte>::max();

namespace Utils {

class CantOpenFile : public std::runtime_error {
public:
        explicit CantOpenFile(std::string const& path);
};

Bytes read_bytes(std::string const& path);
Bytes read_bytes(std::ifstream& ifstream);

}

}

