#pragma once

#include "utils.h"

namespace Emulator {

class Joypads : public Memory {
public:
        static Byte constexpr base_joypad_value = 0x40;
        static unsigned constexpr first_joypad_address = 0x4016;
        static unsigned constexpr second_joypad_address = first_joypad_address + 1;

        bool address_is_writable(unsigned address) const noexcept override;
        bool address_is_readable(unsigned address) const noexcept override;
        void write_byte(unsigned address, Byte byte) override;
        Byte read_byte(unsigned address) override;

private:
        bool strobe() const noexcept;
        Byte read_first_joypad() noexcept;

        Byte last_write_ = 0;
        int num_reads_ = 0;
};

}

