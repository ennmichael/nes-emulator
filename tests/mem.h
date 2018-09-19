#pragma once

#include "../src/utils.h"

template <unsigned N>
class TestMemory : public Emulator::Memory {
public:
        static unsigned constexpr size = N;

        explicit TestMemory(Emulator::Address start)
                : start_(start)
        {}

protected:
        bool address_is_accesible(Emulator::Address address) const noexcept
        {
                address -= start_;
                return address < size;
        }

        Emulator::Address apply_mirroring(Emulator::Address address) const noexcept
        {
                return address - start_;
        }

        bool address_is_writable_impl(Emulator::Address address) const noexcept override
        {
                return address_is_accesible(address);
        }

        bool address_is_readable_impl(Emulator::Address address) const noexcept override
        {
                return address_is_accesible(address);
        }

        void write_byte_impl(Emulator::Address address, Emulator::Byte byte) override
        {
                memory_[apply_mirroring(address)] = byte;
        }

        Emulator::Byte read_byte_impl(Emulator::Address address) override
        {
                return memory_[apply_mirroring(address)];
        }

private:
        Emulator::Address start_;
        std::array<Emulator::Byte, size> memory_;
};
