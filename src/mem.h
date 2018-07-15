#pragma once

#include "utils.h"
#include <memory>
#include <stdexcept>

namespace Emulator
{

// TODO Ditch the whole translate_* bullshit

class Memory {
public:
        class InvalidAddress : public std::runtime_error {
        public:
                InvalidAddress(unsigned address,
                               std::string const& access) noexcept;
        };

        Memory() = default;

        Memory(Memory const&) = delete;
        Memory(Memory&&) = delete;

        Memory& operator=(Memory const&) = delete;
        Memory& operator=(Memory&&) = delete;

        virtual ~Memory() = default;

        virtual bool address_is_writable(unsigned address) const noexcept = 0;
        virtual bool address_is_readable(unsigned address) const noexcept = 0;
        
        void write_byte(unsigned address, Byte byte);
        Byte read_byte(unsigned address) const;
        void write_pointer(unsigned address, unsigned pointer);
        unsigned read_pointer(unsigned address) const;
        Byte deref_byte(unsigned address) const;
        unsigned deref_pointer(unsigned address) const;

private:
        virtual void do_write_byte(unsigned address, Byte byte) = 0;
        virtual Byte do_read_byte(unsigned address) const = 0;
};

using UniqueMemory = std::unique_ptr<Memory>;

}

