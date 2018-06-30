#pragma once

#include "utils.h"
#include <memory>

namespace Emulator
{

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

class TestMemory : public Memory {
public:
        static unsigned constexpr ram_size = 0x600;

        explicit TestMemory(Bytes program) noexcept;

        void write_byte(unsigned address, Byte byte) override;
        Byte read_byte(unsigned address) const override;
        unsigned program_size() const noexcept;

private:
        std::array<Emulator::Byte, ram_size> ram_ {0};
        Bytes program_;
};

using UniqueMemory = std::unique_ptr<Memory>;

}

