#pragma once

#include "utils.h"
#include "ppu.h"
#include "cartridge.h"
#include <array>
#include <utility>
#include <vector>
#include <unordered_map>
#include <functional>
#include <variant>

namespace Emulator {

class CPU {
public:
        class Memory {
        public:
                static std::size_t constexpr adressable_size = 0xFFFF;
                static std::size_t constexpr internal_ram_size = 0x800;
                static std::size_t constexpr ram_mirrors_size =
                        0x2000 - internal_ram_size;

                Memory(Cartridge& cartridge, PPU& ppu) noexcept;

                void write(std::size_t address, Byte byte) noexcept;
                Byte read(std::size_t address) const noexcept;

        private:
                Cartridge* cartridge_;
                PPU* ppu_;
                std::array<Byte, internal_ram_size> ram_ {0};
        }; 
        
        std::size_t constexpr carry_flag = 0;
        std::size_t constexpr zero_flag = 1;
        std::size_t constexpr interrupt_disable_flag = 2;
        std::size_t constexpr break_flag = 4;
        std::size_t constexpr overflow_flag = 6;
        std::size_t constexpr negative_flag = 7; 

        using Instruction = std::variant<std::function<void(CPU&)>,
                                         std::function<void(CPU&, Byte)>,
                                         std::function<void(CPU&, Byte, Byte)>>;
        using InstructionSet = std::unordered_map<Byte, Instruction>;

        static InstructionSet standard_instruction_set();

        CPU(Cartridge& cartridge, PPU& ppu, InstructionSet instruction_set) noexcept;
        CPU(Cartridge& cartridge, PPU& ppu);

        void execute(Bytes const& program);

        unsigned pc() const noexcept;
        Byte sp() const noexcept;
        Byte a() const noexcept;
        Byte x() const noexcept;
        Byte y() const noexcept;

        bool status(std::size_t flag) const noexcept;

private:
        using RawInstruction = void (*)(CPU& cpu, Byte operand);

        void status(std::size_t flag, bool value) noexcept;
        void modify_carry_flag(unsigned operation_result) noexcept;
        void modify_zero_flag(unsigned operation_result) noexcept;
        void modify_overflow_flag(unsigned operation_result) noexcept;
        void modify_negative_flag(unsigned operation_result) noexcept;

        static void adc(CPU& cpu, Byte operand) noexcept;
        static void and(CPU& cpu, Byte operand) noexcept;

        static Instruction immediate(RawInstruction raw);
        static Instruction zero_page(RawInstruction raw);
        static Instruction zero_page_x(RawInstruction raw);
        static Instruction absolute(RawInstruction raw);

        static Instruction instruction_for_opcode(Byte opcode);

        Memory memory_;
        unsigned pc_ = 0;
        Byte sp_ = 0;
        Byte a_ = 0;
        Byte x_ = 0;
        Byte y_ = 0;
        ByteBitset p_(0x34); // TODO Is this correct?
};

}

