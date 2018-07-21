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

class ReadableMemory {
public:
        ReadableMemory() = default;
        ReadableMemory(ReadableMemory const&) = delete;
        ReadableMemory(ReadableMemory&&) = delete;
        ReadableMemory& operator=(ReadableMemory const&) = delete;
        ReadableMemory& operator=(ReadableMemory&&) = delete;
        virtual ~ReadableMemory() = default;

        virtual bool address_is_readable(unsigned address) const noexcept = 0;
        virtual Byte read_byte(unsigned address) = 0;
        unsigned read_pointer(unsigned address);
        unsigned deref_pointer(unsigned address);
        Byte deref_byte(unsigned address);
};

class Memory : public ReadableMemory {
public:
        virtual bool address_is_writable(unsigned address) const noexcept = 0;
        virtual void write_byte(unsigned address, Byte byte) = 0;
        void write_pointer(unsigned address, unsigned pointer);
};

using UniqueReadableMemory = std::unique_ptr<ReadableMemory>;
using UniqueMemory = std::unique_ptr<Memory>;

}

