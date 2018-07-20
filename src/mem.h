#pragma once

#include "utils.h"
#include <memory>
#include <stdexcept>

// TODO Move this to utils? There's really no need for this file, this could
// fit into utils, and the name "mem" is quite awkward

namespace Emulator
{

class InvalidAddress : public std::runtime_error {
public:
        using runtime_error::runtime_error;
};

class Memory {
public:
        Memory() = default;

        Memory(Memory const&) = delete;
        Memory(Memory&&) = delete;

        Memory& operator=(Memory const&) = delete;
        Memory& operator=(Memory&&) = delete;

        virtual ~Memory() = default;

        virtual void write_byte(unsigned address, Byte byte) = 0;
        virtual Byte read_byte(unsigned address) const = 0;

        void write_pointer(unsigned address, unsigned pointer);
        unsigned read_pointer(unsigned address) const;
        Byte deref_byte(unsigned address) const;
        unsigned deref_pointer(unsigned address) const;
};

using UniqueMemory = std::unique_ptr<Memory>;

}

