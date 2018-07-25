#include "catch.hpp"
#include "../src/utils.h"
#include "../src/cpu.h"
#include <utility>

/**
 * TODO Interrupts haven't been tested.
 */

namespace {

unsigned constexpr program_start = 0x0600u;

class TestMemory : public Emulator::CPU::RAM {
public:
        explicit TestMemory(Emulator::ByteVector const& program)
        {
                for (unsigned i = 0; i < program.size(); ++i) {
                        auto const byte = program[i];
                        write_byte(program_start + i, byte);
                }
        }

        bool address_is_readable(unsigned address) const noexcept override
        {
                using CPU = Emulator::CPU;
                unsigned const reset_handler_address =
                        CPU::interrupt_handler_address(CPU::Interrupt::reset);
                return address == reset_handler_address ||
                       address == reset_handler_address + 1 ||
                       RAM::address_is_readable(address);
        }

        Emulator::Byte read_byte(unsigned address) override
        {
                using CPU = Emulator::CPU;
                unsigned const reset_handler_address =
                        CPU::interrupt_handler_address(CPU::Interrupt::reset);
                if (address == reset_handler_address)
                        return Emulator::Utils::low_byte(program_start);
                else if (address == reset_handler_address + 1)
                        return Emulator::Utils::high_byte(program_start);
                return RAM::read_byte(address);
        }
};

Emulator::UniqueCPU execute_example_program(Emulator::ByteVector const& program)
{
        auto cpu = std::make_unique<Emulator::CPU>(
                std::make_unique<TestMemory>(program)
        );
        while (cpu->pc() != program_start + program.size())
                cpu->execute_instruction();
        return cpu;
}

}

TEST_CASE("6502 instructions tests")
{
        SECTION("Some values are loaded into the regisers")
        {
                /**
                  LDA #$01
                  LDX #$02
                  LDY #$03
                 */

                Emulator::ByteVector program {
                        0xA9, 0x01, 0xA2, 0x02, 0xA0, 0x03
                };
                
                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0x01);
                CHECK(cpu->x() == 0x02);
                CHECK(cpu->y() == 0x03);
                CHECK(cpu->p()  == 0x20);
                CHECK(cpu->pc() == 0x0606);
                CHECK(cpu->sp() == 0xFF);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        CHECK(cpu->read_byte(i) == 0x00);
                }
        }

        SECTION("Some values are loaded and stored")
        {
                /**
                 ; Load example values
                 LDA #$01
                 LDX #$05
                 LDY #$0A
                 
                 ; Test storing A in different modes
                 STA $00
                 STA $01,X
                 STA $0300
                 STA $0311,X
                 STA $0311,Y
                 
                 ; Test storing A in indirect modes
                 LDA #$70
                 STA $70
                 LDA #$03
                 STA $71
                 LDA #$DF
                 STA ($70),Y
                 LDA #$DD
                 STA ($6B,X)
                 
                 ; Test storing X in different modes
                 STX $35
                 STX $35,Y
                 STX $0450
                 
                 ; Test storing Y in different modes
                 STY $45
                 STY $45,X
                 STY $0460
                */

                Emulator::ByteVector program {
                        0xA9, 0x01, 0xA2, 0x05, 0xA0, 0x0A, 0x85, 0x00, 
                        0x95, 0x01, 0x8D, 0x00, 0x03, 0x9D, 0x11, 0x03,
                        0x99, 0x11, 0x03, 0xA9, 0x70, 0x85, 0x70, 0xA9, 
                        0x03, 0x85, 0x71, 0xA9, 0xDF, 0x91, 0x70, 0xA9,
                        0xDD, 0x81, 0x6B, 0x86, 0x35, 0x96, 0x35, 0x8E, 
                        0x50, 0x04, 0x84, 0x45, 0x94, 0x45, 0x8C, 0x60,
                        0x04
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0xDD);
                CHECK(cpu->x() == 0x05);
                CHECK(cpu->y() == 0x0A);
                CHECK(cpu->p()  == 0xA0);
                CHECK(cpu->pc() == 0x0631);
                CHECK(cpu->sp() == 0xFF);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x00:
                                CHECK(cpu->read_byte(i) == 0x01);
                                break;
                        case 0x06:
                                CHECK(cpu->read_byte(i) == 0x01);
                                break;
                        case 0x35:
                                CHECK(cpu->read_byte(i) == 0x05);
                                break;
                        case 0x3F:
                                CHECK(cpu->read_byte(i) == 0x05);
                                break;
                        case 0x45:
                                CHECK(cpu->read_byte(i) == 0x0A);
                                break;
                        case 0x4A:
                                CHECK(cpu->read_byte(i) == 0x0A);
                                break;
                        case 0x70:
                                CHECK(cpu->read_byte(i) == 0x70);
                                break;
                        case 0x71:
                                CHECK(cpu->read_byte(i) == 0x03);
                                break;
                        case 0x0300:
                                CHECK(cpu->read_byte(i) == 0x01);
                                break;
                        case 0x0316:
                                CHECK(cpu->read_byte(i) == 0x01);
                                break;
                        case 0x031B:
                                CHECK(cpu->read_byte(i) == 0x01);
                                break;
                        case 0x0370:
                                CHECK(cpu->read_byte(i) == 0xDD);
                                break;
                        case 0x037A:
                                CHECK(cpu->read_byte(i) == 0xDF);
                                break;
                        case 0x0450:
                                CHECK(cpu->read_byte(i) == 0x05);
                                break;
                        case 0x0460:
                                CHECK(cpu->read_byte(i) == 0x0A);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("Some P and A register values are pushed "
             "and pulled from the stack")
        {
                /**
                 LDA #$11
                 PHA
                 LDA #$00
                 PHA
                 PHP
                 PLA
                 PLP
                */

                Emulator::ByteVector program {
                        0xA9, 0x11, 0x48, 0xA9, 0x00, 0x48, 0x08, 0x68, 0x28
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0x22);
                CHECK(cpu->x() == 0x00);
                CHECK(cpu->y() == 0x00);
                CHECK(cpu->p()  == 0x20);
                CHECK(cpu->pc() == 0x0609);
                CHECK(cpu->sp() == 0xFE);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x01FD:
                                CHECK(cpu->read_byte(i) == 0x22);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0x11);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("Some values are added to the accumulator")
        {
                /**
                 ; Don't set the carry flag
                 LDA #$02
                 STA $04
                 STA $0200
                 LDA #$05
                 
                 ADC #$06
                 PHA
                 PHP
                 
                 ADC $04
                 PHA
                 PHP
                 
                 LDX #$02
                 LDY #$03
                 
                 ADC $02,X
                 PHA
                 PHP
                 
                 ADC $0200
                 PHA
                 PHP
                 
                 ADC $01F8,X
                 PHA
                 PHP
                 
                 ADC $01F7,Y
                 PHA
                 PHP
                 
                 ; Indirect addressing modes
                 
                 ; Store the address $0200 at $50
                 LDX $00
                 STX $50
                 LDX $02
                 STX $51
                 
                 LDX $05
                 ADC ($4B,X)
                 PHA
                 PHP
                 
                 ; Store the address $01FF at $50
                 LDX $FF
                 STX $50
                 LDX $01
                 STX $51
                 
                 LDY $01
                 ADC ($50),Y
                 PHA
                 PHP
                 
                 ; Set the carry flag
                 LDA #$FF
                 ADC #$01
                 PHA
                 PHP
                 
                 CLC
                 
                 LDA #$FF
                 ADC #$A0
                 PHA
                 PHP
                 
                 ADC #$01
                 PHA
                 PHP
                 
                 ; Without resetting the carry flag
                 
                 ADC #$02
                */

                Emulator::ByteVector program {
                        0xA9, 0x02, 0x85, 0x04, 0x8D, 0x00, 0x02, 0xA9, 
                        0x05, 0x69, 0x06, 0x48, 0x08, 0x65, 0x04, 0x48,
                        0x08, 0xA2, 0x02, 0xA0, 0x03, 0x75, 0x02, 0x48, 
                        0x08, 0x6D, 0x00, 0x02, 0x48, 0x08, 0x7D, 0xF8,
                        0x01, 0x48, 0x08, 0x79, 0xF7, 0x01, 0x48, 0x08, 
                        0xA6, 0x00, 0x86, 0x50, 0xA6, 0x02, 0x86, 0x51,
                        0xA6, 0x05, 0x61, 0x4B, 0x48, 0x08, 0xA6, 0xFF, 
                        0x86, 0x50, 0xA6, 0x01, 0x86, 0x51, 0xA4, 0x01,
                        0x71, 0x50, 0x48, 0x08, 0xA9, 0xFF, 0x69, 0x01, 
                        0x48, 0x08, 0x18, 0xA9, 0xFF, 0x69, 0xA0, 0x48,
                        0x08, 0x69, 0x01, 0x48, 0x08, 0x69, 0x02
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0xA3);
                CHECK(cpu->x() == 0x00);
                CHECK(cpu->y() == 0x00);
                CHECK(cpu->p()  == 0xA0);
                CHECK(cpu->pc() == 0x0657);
                CHECK(cpu->sp() == 0xE9);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x04:
                                CHECK(cpu->read_byte(i) == 0x02);
                                break;
                        case 0x01EA:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01EB:
                                CHECK(cpu->read_byte(i) == 0xA1);
                                break;
                        case 0x01EC:
                                CHECK(cpu->read_byte(i) == 0xA1);
                                break;
                        case 0x01ED:
                                CHECK(cpu->read_byte(i) == 0x9F);
                                break;
                        case 0x01EE:
                                CHECK(cpu->read_byte(i) == 0x23);
                                break;
                        case 0x01F0:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F1:
                                CHECK(cpu->read_byte(i) == 0x51);
                                break;
                        case 0x01F2:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F3:
                                CHECK(cpu->read_byte(i) == 0x51);
                                break;
                        case 0x01F4:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F5:
                                CHECK(cpu->read_byte(i) == 0x51);
                                break;
                        case 0x01F6:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F7:
                                CHECK(cpu->read_byte(i) == 0x31);
                                break;
                        case 0x01F8:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F9:
                                CHECK(cpu->read_byte(i) == 0x11);
                                break;
                        case 0x01FA:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FB:
                                CHECK(cpu->read_byte(i) == 0x0F);
                                break;
                        case 0x01FC:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FD:
                                CHECK(cpu->read_byte(i) == 0x0D);
                                break;
                        case 0x01FE:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0x0B);
                                break;
                        case 0x0200:
                                CHECK(cpu->read_byte(i) == 0x02);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("Some values are subtracted from the accumulator")
        {
                /**
                 LDA #$02
                 STA $04
                 STA $0200
                 LDA #$05
                 
                 SEC
                 
                 SBC #$06
                 PHP
                 PHA
                 
                 SBC $04
                 PHP
                 PHA
                 
                 LDX #$02
                 LDY #$03
                 
                 SBC $02,X
                 PHP
                 PHA
                 
                 SBC $0200
                 PHP
                 PHA
                 
                 SBC $01F8,X
                 PHP
                 PHA
                 
                 SBC $01F7,Y
                 PHP
                 PHA
                 
                 ; Indirect addressing modes
                 
                 ; Store the address $0200 at $50
                 LDX $00
                 STX $50
                 LDX $02
                 STX $51
                 
                 LDX $05
                 SBC ($4B,X)
                 PHP
                 PHA
                 
                 ; Store the address $01FF at $50
                 LDX $FF
                 STX $50
                 LDX $01
                 STX $51
                 
                 LDY $01
                 SBC ($50),Y
                 PHP
                 PHA
                 
                 ; Set the carry flag
                 LDA #$FF
                 SBC #$01
                 PHP
                 PHA
                 
                 CLC
                 
                 LDA #$FF
                 SBC #$A0
                 PHP
                 PHA
                 
                 SBC #$01
                 PHP
                 PHA
                 
                 ; Without resetting the carry flag
                 
                 SBC #$02
                */

                Emulator::ByteVector program {
                        0xA9, 0x02, 0x85, 0x04, 0x8D, 0x00, 0x02, 0xA9, 
                        0x05, 0x38, 0xE9, 0x06, 0x08, 0x48, 0xE5, 0x04,
                        0x08, 0x48, 0xA2, 0x02, 0xA0, 0x03, 0xF5, 0x02, 
                        0x08, 0x48, 0xED, 0x00, 0x02, 0x08, 0x48, 0xFD,
                        0xF8, 0x01, 0x08, 0x48, 0xF9, 0xF7, 0x01, 0x08, 
                        0x48, 0xA6, 0x00, 0x86, 0x50, 0xA6, 0x02, 0x86,
                        0x51, 0xA6, 0x05, 0xE1, 0x4B, 0x08, 0x48, 0xA6, 
                        0xFF, 0x86, 0x50, 0xA6, 0x01, 0x86, 0x51, 0xA4,
                        0x01, 0xF1, 0x50, 0x08, 0x48, 0xA9, 0xFF, 0xE9, 
                        0x01, 0x08, 0x48, 0x18, 0xA9, 0xFF, 0xE9, 0xA0,
                        0x08, 0x48, 0xE9, 0x01, 0x08, 0x48, 0xE9, 0x02
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0x5B);
                CHECK(cpu->x() == 0x00);
                CHECK(cpu->y() == 0x00);
                CHECK(cpu->p()  == 0x21);
                CHECK(cpu->pc() == 0x0658);
                CHECK(cpu->sp() == 0xE9);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x04:
                                CHECK(cpu->read_byte(i) == 0x02);
                                break;
                        case 0x01EA:
                                CHECK(cpu->read_byte(i) == 0x5D);
                                break;
                        case 0x01EB:
                                CHECK(cpu->read_byte(i) == 0x21);
                                break;
                        case 0x01EC:
                                CHECK(cpu->read_byte(i) == 0x5E);
                                break;
                        case 0x01ED:
                                CHECK(cpu->read_byte(i) == 0x21);
                                break;
                        case 0x01EE:
                                CHECK(cpu->read_byte(i) == 0xFE);
                                break;
                        case 0x01EF:
                                CHECK(cpu->read_byte(i) == 0xA1);
                                break;
                        case 0x01F0:
                                CHECK(cpu->read_byte(i) == 0x03);
                                break;
                        case 0x01F1:
                                CHECK(cpu->read_byte(i) == 0x21);
                                break;
                        case 0x01F2:
                                CHECK(cpu->read_byte(i) == 0x03);
                                break;
                        case 0x01F3:
                                CHECK(cpu->read_byte(i) == 0x21);
                                break;
                        case 0x01F4:
                                CHECK(cpu->read_byte(i) == 0x03);
                                break;
                        case 0x01F5:
                                CHECK(cpu->read_byte(i) == 0x21);
                                break;
                        case 0x01F6:
                                CHECK(cpu->read_byte(i) == 0xFE);
                                break;
                        case 0x01F7:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01F8:
                                CHECK(cpu->read_byte(i) == 0xF8);
                                break;
                        case 0x01F9:
                                CHECK(cpu->read_byte(i) == 0xA1);
                                break;
                        case 0x01FA:
                                CHECK(cpu->read_byte(i) == 0xFA);
                                break;
                        case 0x01FB:
                                CHECK(cpu->read_byte(i) == 0xA1);
                                break;
                        case 0x01FC:
                                CHECK(cpu->read_byte(i) == 0xFC);
                                break;
                        case 0x01FD:
                                CHECK(cpu->read_byte(i) == 0xA1);
                                break;
                        case 0x01FE:
                                CHECK(cpu->read_byte(i) == 0xFF);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x0200:
                                CHECK(cpu->read_byte(i) == 0x02);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("Some values are AND'ed")
        {
                /**
                 LDA #$22
                 STA $05
                 STA $0300
                 
                 LDA #$33
                 AND #$F0
                 PHP
                 PHA
                 
                 AND $05
                 PHP
                 PHA
                 
                 LDX #$03
                 LDY #$02
                 AND $02, X
                 PHP
                 PHA
                 
                 AND $0300
                 PHP
                 PHA
                 
                 AND $02FD,X
                 PHP
                 PHA
                 
                 AND $02FE,Y
                 PHP
                 PHA
                 
                 ; Indirect addressing modes
                 
                 LDA #$00
                 STA $40
                 LDA #$03
                 STA $41
                 LDX #$30
                 AND ($10,X)
                 PHP
                 PHA
                 
                 LDY #$02
                 AND ($41),Y
                 PHP
                 PHA
                */

                Emulator::ByteVector program {
                        0xA9, 0x22, 0x85, 0x05, 0x8D, 0x00, 0x03, 0xA9, 
                        0x33, 0x29, 0xF0, 0x08, 0x48, 0x25, 0x05, 0x08,
                        0x48, 0xA2, 0x03, 0xA0, 0x02, 0x35, 0x02, 0x08, 
                        0x48, 0x2D, 0x00, 0x03, 0x08, 0x48, 0x3D, 0xFD,
                        0x02, 0x08, 0x48, 0x39, 0xFE, 0x02, 0x08, 0x48, 
                        0xA9, 0x00, 0x85, 0x40, 0xA9, 0x03, 0x85, 0x41,
                        0xA2, 0x30, 0x21, 0x10, 0x08, 0x48, 0xA0, 0x02, 
                        0x31, 0x41, 0x08, 0x48
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0x02);
                CHECK(cpu->x() == 0x30);
                CHECK(cpu->y() == 0x02);
                CHECK(cpu->p()  == 0x20);
                CHECK(cpu->pc() == 0x063C);
                CHECK(cpu->sp() == 0xEF);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x05:
                                CHECK(cpu->read_byte(i) == 0x22);
                                break;
                        case 0x41:
                                CHECK(cpu->read_byte(i) == 0x03);
                                break;
                        case 0x01F0:
                                CHECK(cpu->read_byte(i) == 0x02);
                                break;
                        case 0x01F1:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F2:
                                CHECK(cpu->read_byte(i) == 0x02);
                                break;
                        case 0x01F3:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F4:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F5:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F6:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F7:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F8:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F9:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FA:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FB:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FC:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FD:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FE:
                                CHECK(cpu->read_byte(i) == 0x30);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x0300:
                                CHECK(cpu->read_byte(i) == 0x22);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }
        
        SECTION("Some memory is DEC'ed")
        {
                /**
                 LDA #$42
                 STA $05
                 STA $0400
                 STA $0401
                 
                 DEC $00
                 PHP
                 
                 LDX #$02
                 DEC $03,X
                 PHP
                 DEC $03,X
                 PHP
                 
                 DEC $0400
                 PHP
                 
                 DEC $03FD,X
                 PHP
                */

                Emulator::ByteVector program {
                        0xA9, 0x42, 0x85, 0x05, 0x8D, 0x00, 0x04, 0x8D, 
                        0x01, 0x04, 0xC6, 0x00, 0x08, 0xA2, 0x02, 0xD6,
                        0x03, 0x08, 0xD6, 0x03, 0x08, 0xCE, 0x00, 0x04, 
                        0x08, 0xDE, 0xFD, 0x03, 0x08
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0x42);
                CHECK(cpu->x() == 0x02);
                CHECK(cpu->y() == 0x00);
                CHECK(cpu->p()  == 0xA0);
                CHECK(cpu->pc() == 0x061D);
                CHECK(cpu->sp() == 0xFA);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x00:
                                CHECK(cpu->read_byte(i) == 0xFF);
                                break;
                        case 0x05:
                                CHECK(cpu->read_byte(i) == 0x40);
                                break;
                        case 0x01FB:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01FC:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FD:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FE:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x03FF:
                                CHECK(cpu->read_byte(i) == 0xFF);
                                break;
                        case 0x0400:
                                CHECK(cpu->read_byte(i) == 0x41);
                                break;
                        case 0x0401:
                                CHECK(cpu->read_byte(i) == 0x42);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("Some memory is INC'ed")
        {
                /**
                 LDA #$42
                 STA $05
                 STA $0400
                 STA $0401
                 
                 INC $00
                 PHP
                 
                 LDX #$02
                 INC $03,X
                 PHP
                 INC $03,X
                 PHP
                 
                 INC $0400
                 PHP
                 
                 INC $03FD,X
                 PHP
                */

                Emulator::ByteVector program {
                        0xA9, 0x42, 0x85, 0x05, 0x8D, 0x00, 0x04, 0x8D, 
                        0x01, 0x04, 0xE6, 0x00, 0x08, 0xA2, 0x02, 0xF6,
                        0x03, 0x08, 0xF6, 0x03, 0x08, 0xEE, 0x00, 0x04, 
                        0x08, 0xFE, 0xFD, 0x03, 0x08
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0x42);
                CHECK(cpu->x() == 0x02);
                CHECK(cpu->y() == 0x00);
                CHECK(cpu->p()  == 0x20);
                CHECK(cpu->pc() == 0x061D);
                CHECK(cpu->sp() == 0xFA);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x00:
                                CHECK(cpu->read_byte(i) == 0x01);
                                break;
                        case 0x05:
                                CHECK(cpu->read_byte(i) == 0x44);
                                break;
                        case 0x01FB:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FC:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FD:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FE:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x03FF:
                                CHECK(cpu->read_byte(i) == 0x01);
                                break;
                        case 0x0400:
                                CHECK(cpu->read_byte(i) == 0x43);
                                break;
                        case 0x0401:
                                CHECK(cpu->read_byte(i) == 0x42);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }
       
        SECTION("The stack pointer is manipulated")
        {
                /**
                 LDA #$22
                 PHA
                 
                 TSX
                 PHP
                 STX $00
                 
                 LDX #$42
                 TXS
                 PHP
                 PHA
                */

                Emulator::ByteVector program {
                        0xA9, 0x22, 0x48, 0xBA, 0x08, 0x86, 0x00, 0xA2, 
                        0x42, 0x9A, 0x08, 0x48
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0x22);
                CHECK(cpu->x() == 0x42);
                CHECK(cpu->y() == 0x00);
                CHECK(cpu->p()  == 0x20);
                CHECK(cpu->pc() == 0x060C);
                CHECK(cpu->sp() == 0x40);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x00:
                                CHECK(cpu->read_byte(i) == 0xFE);
                                break;
                        case 0x0141:
                                CHECK(cpu->read_byte(i) == 0x22);
                                break;
                        case 0x0142:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FE:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0x22);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("A and some memory is ASL'ed")
        {
                /**
                 LDA #$22
                 STA $25
                 STA $0202
                 
                 LDA #$96
                 ASL A
                 PHA
                 PHP
                 
                 LDX #$02
                 
                 ASL $25
                 PHP
                 ASL $23,X
                 PHP
                 ASL $0202
                 PHP
                 ASL $0200,X
                */

                Emulator::ByteVector program {
                        0xA9, 0x22, 0x85, 0x25, 0x8D, 0x02, 0x02, 0xA9, 
                        0x96, 0x0A, 0x48, 0x08, 0xA2, 0x02, 0x06, 0x25,
                        0x08, 0x16, 0x23, 0x08, 0x0E, 0x02, 0x02, 0x08, 
                        0x1E, 0x00, 0x02
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0x2C);
                CHECK(cpu->x() == 0x02);
                CHECK(cpu->y() == 0x00);
                CHECK(cpu->p()  == 0xA0);
                CHECK(cpu->pc() == 0x061B);
                CHECK(cpu->sp() == 0xFA);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x25:
                                CHECK(cpu->read_byte(i) == 0x88);
                                break;
                        case 0x01FB:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FC:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01FD:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FE:
                                CHECK(cpu->read_byte(i) == 0x21);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0x2C);
                                break;
                        case 0x0202:
                                CHECK(cpu->read_byte(i) == 0x88);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("BIT is applied to some values")
        {
                /**
                 LDX #$22
                 STX $00
                 STX $0200
                 LDX #$8F
                 STX $01
                 STX $0201
                 
                 LDA #$FF
                 
                 BIT $00
                 PHP
                 BIT $0200
                 PHP
                 BIT $01
                 PHP
                 BIT $0201
                 PHP
                 
                 LDA #$00
                 
                 BIT $00
                 PHP
                 BIT $0200
                 PHP
                 BIT $01
                 PHP
                 BIT $0201
                 PHP
                 
                 LDA #$A3
                 
                 BIT $00
                 PHP
                 BIT $0200
                 PHP
                 BIT $01
                 PHP
                 BIT $0201
                 PHP
                */

                Emulator::ByteVector program {
                        0xA2, 0x22, 0x86, 0x00, 0x8E, 0x00, 0x02, 0xA2, 
                        0x8F, 0x86, 0x01, 0x8E, 0x01, 0x02, 0xA9, 0xFF,
                        0x24, 0x00, 0x08, 0x2C, 0x00, 0x02, 0x08, 0x24, 
                        0x01, 0x08, 0x2C, 0x01, 0x02, 0x08, 0xA9, 0x00,
                        0x24, 0x00, 0x08, 0x2C, 0x00, 0x02, 0x08, 0x24, 
                        0x01, 0x08, 0x2C, 0x01, 0x02, 0x08, 0xA9, 0xA3,
                        0x24, 0x00, 0x08, 0x2C, 0x00, 0x02, 0x08, 0x24, 
                        0x01, 0x08, 0x2C, 0x01, 0x02, 0x08
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0xA3);
                CHECK(cpu->x() == 0x8F);
                CHECK(cpu->y() == 0x00);
                CHECK(cpu->p()  == 0xA0);
                CHECK(cpu->pc() == 0x063E);
                CHECK(cpu->sp() == 0xF3);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x00:
                                CHECK(cpu->read_byte(i) == 0x22);
                                break;
                        case 0x01:
                                CHECK(cpu->read_byte(i) == 0x8F);
                                break;
                        case 0x01F4:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01F5:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01F6:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F7:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F8:
                                CHECK(cpu->read_byte(i) == 0xA2);
                                break;
                        case 0x01F9:
                                CHECK(cpu->read_byte(i) == 0xA2);
                                break;
                        case 0x01FA:
                                CHECK(cpu->read_byte(i) == 0x22);
                                break;
                        case 0x01FB:
                                CHECK(cpu->read_byte(i) == 0x22);
                                break;
                        case 0x01FC:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01FD:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01FE:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x0200:
                                CHECK(cpu->read_byte(i) == 0x22);
                                break;
                        case 0x0201:
                                CHECK(cpu->read_byte(i) == 0x8F);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("Some values are EOR'ed")
        {
                /**
                 LDA #$22
                 STA $05
                 STA $0300
                 
                 LDA #$33
                 EOR #$F0
                 PHP
                 PHA
                 
                 EOR $05
                 PHP
                 PHA
                 
                 LDX #$03
                 LDY #$02
                 EOR $02, X
                 PHP
                 PHA
                 
                 EOR $0300
                 PHP
                 PHA
                 
                 EOR $02FD,X
                 PHP
                 PHA
                 
                 EOR $02FE,Y
                 PHP
                 PHA
                 
                 ; Indirect addressing modes
                 
                 LDA #$00
                 STA $40
                 LDA #$03
                 STA $41
                 LDX #$30
                 EOR ($10,X)
                 PHP
                 PHA
                 
                 LDY #$02
                 EOR ($41),Y
                 PHP
                 PHA
                 
                */

                Emulator::ByteVector program {
                        0xA9, 0x22, 0x85, 0x05, 0x8D, 0x00, 0x03, 0xA9, 
                        0x33, 0x49, 0xF0, 0x08, 0x48, 0x45, 0x05, 0x08,
                        0x48, 0xA2, 0x03, 0xA0, 0x02, 0x55, 0x02, 0x08, 
                        0x48, 0x4D, 0x00, 0x03, 0x08, 0x48, 0x5D, 0xFD,
                        0x02, 0x08, 0x48, 0x59, 0xFE, 0x02, 0x08, 0x48, 
                        0xA9, 0x00, 0x85, 0x40, 0xA9, 0x03, 0x85, 0x41,
                        0xA2, 0x30, 0x41, 0x10, 0x08, 0x48, 0xA0, 0x02, 
                        0x51, 0x41, 0x08, 0x48
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0x03);
                CHECK(cpu->x() == 0x30);
                CHECK(cpu->y() == 0x02);
                CHECK(cpu->p()  == 0x20);
                CHECK(cpu->pc() == 0x063C);
                CHECK(cpu->sp() == 0xEF);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x05:
                                CHECK(cpu->read_byte(i) == 0x22);
                                break;
                        case 0x41:
                                CHECK(cpu->read_byte(i) == 0x03);
                                break;
                        case 0x01F0:
                                CHECK(cpu->read_byte(i) == 0x03);
                                break;
                        case 0x01F1:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F2:
                                CHECK(cpu->read_byte(i) == 0x21);
                                break;
                        case 0x01F3:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F4:
                                CHECK(cpu->read_byte(i) == 0xE1);
                                break;
                        case 0x01F5:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01F6:
                                CHECK(cpu->read_byte(i) == 0xC3);
                                break;
                        case 0x01F7:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01F8:
                                CHECK(cpu->read_byte(i) == 0xE1);
                                break;
                        case 0x01F9:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01FA:
                                CHECK(cpu->read_byte(i) == 0xC3);
                                break;
                        case 0x01FB:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01FC:
                                CHECK(cpu->read_byte(i) == 0xE1);
                                break;
                        case 0x01FD:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01FE:
                                CHECK(cpu->read_byte(i) == 0xC3);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x0300:
                                CHECK(cpu->read_byte(i) == 0x22);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("Some flags are set and cleared")
        {
                /**
                 SEC
                 PHP
                 CLC
                 PHP
                 SEI
                 PHP
                 CLI
                 PHP
                 
                 SEC
                 SEI
                 PHP
                 
                 CLC
                 PHP
                 
                 CLI
                 PHP
                 
                 LDA #$7F
                 ADC #$01 ; Sets the overflow flag
                 PHP
                 CLV
                 PHP
                */

                Emulator::ByteVector program {
                        0x38, 0x08, 0x18, 0x08, 0x78, 0x08, 0x58, 0x08, 
                        0x38, 0x78, 0x08, 0x18, 0x08, 0x58, 0x08, 0xA9,
                        0x7F, 0x69, 0x01, 0x08, 0xB8, 0x08
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0x80);
                CHECK(cpu->x() == 0x00);
                CHECK(cpu->y() == 0x00);
                CHECK(cpu->p()  == 0xA0);
                CHECK(cpu->pc() == 0x0616);
                CHECK(cpu->sp() == 0xF6);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x01F7:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01F8:
                                CHECK(cpu->read_byte(i) == 0xE0);
                                break;
                        case 0x01F9:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FA:
                                CHECK(cpu->read_byte(i) == 0x24);
                                break;
                        case 0x01FB:
                                CHECK(cpu->read_byte(i) == 0x25);
                                break;
                        case 0x01FC:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FD:
                                CHECK(cpu->read_byte(i) == 0x24);
                                break;
                        case 0x01FE:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0x21);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("A and some memory is LSR'ed")
        {
                /**
                 LDA #$22
                 STA $25
                 STA $0202
                 
                 LDA #$96
                 LSR A
                 PHA
                 PHP
                 
                 LDX #$02
                 
                 LSR $25
                 PHP
                 LSR $23,X
                 PHP
                 LSR $0202
                 PHP
                 LSR $0200,X
                */

                Emulator::ByteVector program {
                        0xA9, 0x22, 0x85, 0x25, 0x8D, 0x02, 0x02, 0xA9, 
                        0x96, 0x4A, 0x48, 0x08, 0xA2, 0x02, 0x46, 0x25,
                        0x08, 0x56, 0x23, 0x08, 0x4E, 0x02, 0x02, 0x08, 
                        0x5E, 0x00, 0x02
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0x4B);
                CHECK(cpu->x() == 0x02);
                CHECK(cpu->y() == 0x00);
                CHECK(cpu->p()  == 0x21);
                CHECK(cpu->pc() == 0x061B);
                CHECK(cpu->sp() == 0xFA);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x25:
                                CHECK(cpu->read_byte(i) == 0x08);
                                break;
                        case 0x01FB:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FC:
                                CHECK(cpu->read_byte(i) == 0x21);
                                break;
                        case 0x01FD:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FE:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0x4B);
                                break;
                        case 0x0202:
                                CHECK(cpu->read_byte(i) == 0x08);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("Some values are ORA'd")
        {
                /**
                 LDA #$22
                 STA $05
                 STA $0300
                 
                 LDA #$33
                 ORA #$F0
                 PHP
                 PHA
                 
                 ORA $05
                 PHP
                 PHA
                 
                 LDX #$03
                 LDY #$02
                 ORA $02, X
                 PHP
                 PHA
                 
                 ORA $0300
                 PHP
                 PHA
                 
                 ORA $02FD,X
                 PHP
                 PHA
                 
                 ORA $02FE,Y
                 PHP
                 PHA
                 
                 ; Indirect addressing modes
                 
                 LDA #$00
                 STA $40
                 LDA #$03
                 STA $41
                 LDX #$30
                 ORA ($10,X)
                 PHP
                 PHA
                 
                 LDY #$02
                 ORA ($41),Y
                 PHP
                 PHA
                */

                Emulator::ByteVector program {
                        0xA9, 0x22, 0x85, 0x05, 0x8D, 0x00, 0x03, 0xA9, 
                        0x33, 0x09, 0xF0, 0x08, 0x48, 0x05, 0x05, 0x08,
                        0x48, 0xA2, 0x03, 0xA0, 0x02, 0x15, 0x02, 0x08, 
                        0x48, 0x0D, 0x00, 0x03, 0x08, 0x48, 0x1D, 0xFD,
                        0x02, 0x08, 0x48, 0x19, 0xFE, 0x02, 0x08, 0x48, 
                        0xA9, 0x00, 0x85, 0x40, 0xA9, 0x03, 0x85, 0x41,
                        0xA2, 0x30, 0x01, 0x10, 0x08, 0x48, 0xA0, 0x02, 
                        0x11, 0x41, 0x08, 0x48
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0x23);
                CHECK(cpu->x() == 0x30);
                CHECK(cpu->y() == 0x02);
                CHECK(cpu->p()  == 0x20);
                CHECK(cpu->pc() == 0x063C);
                CHECK(cpu->sp() == 0xEF);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x05:
                                CHECK(cpu->read_byte(i) == 0x22);
                                break;
                        case 0x41:
                                CHECK(cpu->read_byte(i) == 0x03);
                                break;
                        case 0x01F0:
                                CHECK(cpu->read_byte(i) == 0x23);
                                break;
                        case 0x01F1:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F2:
                                CHECK(cpu->read_byte(i) == 0x23);
                                break;
                        case 0x01F3:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F4:
                                CHECK(cpu->read_byte(i) == 0xF3);
                                break;
                        case 0x01F5:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01F6:
                                CHECK(cpu->read_byte(i) == 0xF3);
                                break;
                        case 0x01F7:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01F8:
                                CHECK(cpu->read_byte(i) == 0xF3);
                                break;
                        case 0x01F9:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01FA:
                                CHECK(cpu->read_byte(i) == 0xF3);
                                break;
                        case 0x01FB:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01FC:
                                CHECK(cpu->read_byte(i) == 0xF3);
                                break;
                        case 0x01FD:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01FE:
                                CHECK(cpu->read_byte(i) == 0xF3);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x0300:
                                CHECK(cpu->read_byte(i) == 0x22);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("Registers are transfered all around")
        {
                /**
                 LDA #$E2
                 TAX
                 LDA #$00
                 TXA
                 PHA
                 
                 DEX
                 TXA
                 PHA
                 DEX
                 TXA
                 PHA
                 
                 INX
                 TXA
                 PHA
                 INX
                 TXA
                 PHA
                 
                 TAY
                 LDA #$00
                 TYA
                 PHA
                 
                 DEY
                 TYA
                 PHA
                 DEY
                 TYA
                 PHA
                 
                 INY
                 TYA
                 PHA
                 INY
                 TYA
                 PHA
                */

                Emulator::ByteVector program {
                        0xA9, 0xE2, 0xAA, 0xA9, 0x00, 0x8A, 0x48, 0xCA, 
                        0x8A, 0x48, 0xCA, 0x8A, 0x48, 0xE8, 0x8A, 0x48,
                        0xE8, 0x8A, 0x48, 0xA8, 0xA9, 0x00, 0x98, 0x48, 
                        0x88, 0x98, 0x48, 0x88, 0x98, 0x48, 0xC8, 0x98,
                        0x48, 0xC8, 0x98, 0x48
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0xE2);
                CHECK(cpu->x() == 0xE2);
                CHECK(cpu->y() == 0xE2);
                CHECK(cpu->p()  == 0xA0);
                CHECK(cpu->pc() == 0x0624);
                CHECK(cpu->sp() == 0xF5);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x01F6:
                                CHECK(cpu->read_byte(i) == 0xE2);
                                break;
                        case 0x01F7:
                                CHECK(cpu->read_byte(i) == 0xE1);
                                break;
                        case 0x01F8:
                                CHECK(cpu->read_byte(i) == 0xE0);
                                break;
                        case 0x01F9:
                                CHECK(cpu->read_byte(i) == 0xE1);
                                break;
                        case 0x01FA:
                                CHECK(cpu->read_byte(i) == 0xE2);
                                break;
                        case 0x01FB:
                                CHECK(cpu->read_byte(i) == 0xE2);
                                break;
                        case 0x01FC:
                                CHECK(cpu->read_byte(i) == 0xE1);
                                break;
                        case 0x01FD:
                                CHECK(cpu->read_byte(i) == 0xE0);
                                break;
                        case 0x01FE:
                                CHECK(cpu->read_byte(i) == 0xE1);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0xE2);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("A and some memory is ROL'ed")
        {
                /**
                 LDA #$22
                 STA $25
                 STA $0202
                 
                 LDA #$96
                 ROL A
                 PHA
                 PHP
                 
                 LDX #$02
                 
                 ROL $25
                 PHP
                 ROL $23,X
                 PHP
                 ROL $0202
                 PHP
                 ROL $0200,X
                */

                Emulator::ByteVector program {
                        0xA9, 0x22, 0x85, 0x25, 0x8D, 0x02, 0x02, 0xA9, 
                        0x96, 0x2A, 0x48, 0x08, 0xA2, 0x02, 0x26, 0x25,
                        0x08, 0x36, 0x23, 0x08, 0x2E, 0x02, 0x02, 0x08, 
                        0x3E, 0x00, 0x02
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0x2C);
                CHECK(cpu->x() == 0x02);
                CHECK(cpu->y() == 0x00);
                CHECK(cpu->p()  == 0xA0);
                CHECK(cpu->pc() == 0x061B);
                CHECK(cpu->sp() == 0xFA);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x25:
                                CHECK(cpu->read_byte(i) == 0x8A);
                                break;
                        case 0x01FB:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FC:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01FD:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FE:
                                CHECK(cpu->read_byte(i) == 0x21);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0x2C);
                                break;
                        case 0x0202:
                                CHECK(cpu->read_byte(i) == 0x88);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("A and some memory is ROR'ed")
        {
                /**
                 LDA #$22
                 STA $25
                 STA $0202
                 
                 LDA #$96
                 ROR A
                 PHA
                 PHP
                 
                 LDX #$02
                 
                 ROR $25
                 PHP
                 ROR $23,X
                 PHP
                 ROR $0202
                 PHP
                 ROR $0200,X
                */

                Emulator::ByteVector program {
                        0xA9, 0x22, 0x85, 0x25, 0x8D, 0x02, 0x02, 0xA9, 
                        0x96, 0x6A, 0x48, 0x08, 0xA2, 0x02, 0x66, 0x25,
                        0x08, 0x76, 0x23, 0x08, 0x6E, 0x02, 0x02, 0x08, 
                        0x7E, 0x00, 0x02
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0x4B);
                CHECK(cpu->x() == 0x02);
                CHECK(cpu->y() == 0x00);
                CHECK(cpu->p()  == 0x21);
                CHECK(cpu->pc() == 0x061B);
                CHECK(cpu->sp() == 0xFA);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x25:
                                CHECK(cpu->read_byte(i) == 0x08);
                                break;
                        case 0x01FB:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01FC:
                                CHECK(cpu->read_byte(i) == 0x21);
                                break;
                        case 0x01FD:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FE:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0x4B);
                                break;
                        case 0x0202:
                                CHECK(cpu->read_byte(i) == 0x48);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("Values are CMP'ed")
        {
                /**
                 LDA #$22
                 STA $05
                 STA $0300
                 
                 LDA #$33
                 CMP #$F0
                 PHP
                 CMP #$0B
                 PHP
                 CMP #$33
                 PHP
                 
                 CMP $05
                 PHP
                 LDA #$22
                 CMP $05
                 PHP
                 LDA #$0B
                 CMP $05
                 PHP
                 
                 LDX #$03
                 LDY #$02
                 CMP $02, X
                 PHP
                 
                 CMP $0300
                 PHP
                 
                 CMP $02FD,X
                 PHP
                 
                 CMP $02FE,Y
                 PHP
                 
                 ; Indirect addressing modes
                 
                 LDA #$00
                 STA $40
                 LDA #$03
                 STA $41
                 LDX #$30
                 CMP ($10,X)
                 PHP
                 
                 LDY #$02
                 CMP ($41),Y
                 PHP
                */

                Emulator::ByteVector program {
                        0xA9, 0x22, 0x85, 0x05, 0x8D, 0x00, 0x03, 0xA9, 
                        0x33, 0xC9, 0xF0, 0x08, 0xC9, 0x0B, 0x08, 0xC9,
                        0x33, 0x08, 0xC5, 0x05, 0x08, 0xA9, 0x22, 0xC5, 
                        0x05, 0x08, 0xA9, 0x0B, 0xC5, 0x05, 0x08, 0xA2,
                        0x03, 0xA0, 0x02, 0xD5, 0x02, 0x08, 0xCD, 0x00, 
                        0x03, 0x08, 0xDD, 0xFD, 0x02, 0x08, 0xD9, 0xFE,
                        0x02, 0x08, 0xA9, 0x00, 0x85, 0x40, 0xA9, 0x03, 
                        0x85, 0x41, 0xA2, 0x30, 0xC1, 0x10, 0x08, 0xA0,
                        0x02, 0xD1, 0x41, 0x08
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0x03);
                CHECK(cpu->x() == 0x30);
                CHECK(cpu->y() == 0x02);
                CHECK(cpu->p()  == 0xA0);
                CHECK(cpu->pc() == 0x0644);
                CHECK(cpu->sp() == 0xF3);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x05:
                                CHECK(cpu->read_byte(i) == 0x22);
                                break;
                        case 0x41:
                                CHECK(cpu->read_byte(i) == 0x03);
                                break;
                        case 0x01F4:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01F5:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01F6:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01F7:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01F8:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01F9:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01FA:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01FB:
                                CHECK(cpu->read_byte(i) == 0x23);
                                break;
                        case 0x01FC:
                                CHECK(cpu->read_byte(i) == 0x21);
                                break;
                        case 0x01FD:
                                CHECK(cpu->read_byte(i) == 0x23);
                                break;
                        case 0x01FE:
                                CHECK(cpu->read_byte(i) == 0x21);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x0300:
                                CHECK(cpu->read_byte(i) == 0x22);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("Some values are CPX'ed")
        {
                /**
                 LDX #$10
                 CPX #$10
                 PHP
                 CPX #$0F
                 PHP
                 CPX #$FF
                 PHP
                 
                 LDA #$10
                 STA $00
                 CPX $00
                 PHP
                 LDA #$20
                 STA $01
                 CPX $01
                 PHP
                 LDA #$00
                 STA $02
                 CPX $02
                 PHP
                 
                 LDA #$10
                 STA $0200
                 CPX $0200
                 PHP
                 LDA #$20
                 STA $0201
                 CPX $0201
                 PHP
                 LDA #$00
                 STA $0202
                 CPX $0202
                 PHP
                */

                Emulator::ByteVector program {
                        0xA2, 0x10, 0xE0, 0x10, 0x08, 0xE0, 0x0F, 0x08, 
                        0xE0, 0xFF, 0x08, 0xA9, 0x10, 0x85, 0x00, 0xE4,
                        0x00, 0x08, 0xA9, 0x20, 0x85, 0x01, 0xE4, 0x01, 
                        0x08, 0xA9, 0x00, 0x85, 0x02, 0xE4, 0x02, 0x08,
                        0xA9, 0x10, 0x8D, 0x00, 0x02, 0xEC, 0x00, 0x02, 
                        0x08, 0xA9, 0x20, 0x8D, 0x01, 0x02, 0xEC, 0x01,
                        0x02, 0x08, 0xA9, 0x00, 0x8D, 0x02, 0x02, 0xEC, 
                        0x02, 0x02, 0x08
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0x00);
                CHECK(cpu->x() == 0x10);
                CHECK(cpu->y() == 0x00);
                CHECK(cpu->p()  == 0x21);
                CHECK(cpu->pc() == 0x063B);
                CHECK(cpu->sp() == 0xF6);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x00:
                                CHECK(cpu->read_byte(i) == 0x10);
                                break;
                        case 0x01:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F7:
                                CHECK(cpu->read_byte(i) == 0x21);
                                break;
                        case 0x01F8:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01F9:
                                CHECK(cpu->read_byte(i) == 0x23);
                                break;
                        case 0x01FA:
                                CHECK(cpu->read_byte(i) == 0x21);
                                break;
                        case 0x01FB:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01FC:
                                CHECK(cpu->read_byte(i) == 0x23);
                                break;
                        case 0x01FD:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FE:
                                CHECK(cpu->read_byte(i) == 0x21);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0x23);
                                break;
                        case 0x0200:
                                CHECK(cpu->read_byte(i) == 0x10);
                                break;
                        case 0x0201:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("Some values are CPY'ed")
        {
                /**
                 LDY #$10
                 CPY #$10
                 PHP
                 CPY #$0F
                 PHP
                 CPY #$FF
                 PHP
                 
                 LDA #$10
                 STA $00
                 CPY $00
                 PHP
                 LDA #$20
                 STA $01
                 CPY $01
                 PHP
                 LDA #$00
                 STA $02
                 CPY $02
                 PHP
                 
                 LDA #$10
                 STA $0200
                 CPY $0200
                 PHP
                 LDA #$20
                 STA $0201
                 CPY $0201
                 PHP
                 LDA #$00
                 STA $0202
                 CPY $0202
                 PHP
                */

                Emulator::ByteVector program {
                        0xA0, 0x10, 0xC0, 0x10, 0x08, 0xC0, 0x0F, 0x08, 
                        0xC0, 0xFF, 0x08, 0xA9, 0x10, 0x85, 0x00, 0xC4,
                        0x00, 0x08, 0xA9, 0x20, 0x85, 0x01, 0xC4, 0x01, 
                        0x08, 0xA9, 0x00, 0x85, 0x02, 0xC4, 0x02, 0x08,
                        0xA9, 0x10, 0x8D, 0x00, 0x02, 0xCC, 0x00, 0x02, 
                        0x08, 0xA9, 0x20, 0x8D, 0x01, 0x02, 0xCC, 0x01,
                        0x02, 0x08, 0xA9, 0x00, 0x8D, 0x02, 0x02, 0xCC, 
                        0x02, 0x02, 0x08
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0x00);
                CHECK(cpu->x() == 0x00);
                CHECK(cpu->y() == 0x10);
                CHECK(cpu->p()  == 0x21);
                CHECK(cpu->pc() == 0x063B);
                CHECK(cpu->sp() == 0xF6);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x00:
                                CHECK(cpu->read_byte(i) == 0x10);
                                break;
                        case 0x01:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01F7:
                                CHECK(cpu->read_byte(i) == 0x21);
                                break;
                        case 0x01F8:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01F9:
                                CHECK(cpu->read_byte(i) == 0x23);
                                break;
                        case 0x01FA:
                                CHECK(cpu->read_byte(i) == 0x21);
                                break;
                        case 0x01FB:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        case 0x01FC:
                                CHECK(cpu->read_byte(i) == 0x23);
                                break;
                        case 0x01FD:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FE:
                                CHECK(cpu->read_byte(i) == 0x21);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0x23);
                                break;
                        case 0x0200:
                                CHECK(cpu->read_byte(i) == 0x10);
                                break;
                        case 0x0201:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("JMP is used")
        {
                /**
                 JMP label
                 LDA #$01
                 label:
                   LDA #$00
                */

                Emulator::ByteVector program {
                        0x4C, 0x05, 0x06, 0xA9, 0x01, 0xA9, 0x00
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0x00);
                CHECK(cpu->x() == 0x00);
                CHECK(cpu->y() == 0x00);
                CHECK(cpu->p()  == 0x22);
                CHECK(cpu->pc() == 0x0607);
                CHECK(cpu->sp() == 0xFF);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("Some branches are selected")
        {
                /**
                 ; BPL/BMI testing
                 
                 LDA #$FE
                 ADC #$01 ; Set the negative flag
                 
                 BPL skip1
                 STA $00
                 
                 skip1:
                   BMI skip2
                   STA $01
                 
                 skip2:
                   STA $02
                 
                 ; Calculate 5 * 10 and store it in $03
                 ; Use $04 as the counter
                 
                 LDA #$00
                 
                 loop:
                   ADC #$05
                   INC $04 ; Increment the counter
                   LDX $04
                   CPX #$0A
                   BNE loop
                   BEQ somewhereElse
                 
                 ; These two lines should be dead code
                 LDA #$22
                 STA $0200
                 
                 somewhereElse:
                   STA $03 ; Store 5 * 10 in $03
                   SEC
                   BCC somewhereElse ; Would be an infinite loop
                   BCS someNop
                   ; More dead code
                   LDA #$22
                   STA $0200
                 
                 someNop:
                   NOP ; :-)
                   CLC
                   BCC ahead
                   LDA #$22 ; Dead
                   STA $0200
                 
                 ahead:
                   LDA #$7F
                   ADC #$01 ; Set overflow flag
                   BVC dontSkip
                   BVS skip69
                 
                 dontSkip:
                   LDA #$22 ; Dead
                   STA $0200
                 
                 skip69:
                   LDA #$00
                   ADC #$02 ; Reset overflow flag
                   BVS hahalol
                   BVC hahalolno
                 
                 hahalol:
                   LDA #$22 ; Dead
                   STA $0200
                 
                 hahalolno:
                   NOP ; Alive
                */

                Emulator::ByteVector program {
                        0xA9, 0xFE, 0x69, 0x01, 0x10, 0x02, 0x85, 0x00, 
                        0x30, 0x02, 0x85, 0x01, 0x85, 0x02, 0xA9, 0x00,
                        0x69, 0x05, 0xE6, 0x04, 0xA6, 0x04, 0xE0, 0x0A, 
                        0xD0, 0xF6, 0xF0, 0x05, 0xA9, 0x22, 0x8D, 0x00,
                        0x02, 0x85, 0x03, 0x38, 0x90, 0xFB, 0xB0, 0x05, 
                        0xA9, 0x22, 0x8D, 0x00, 0x02, 0xEA, 0x18, 0x90,
                        0x05, 0xA9, 0x22, 0x8D, 0x00, 0x02, 0xA9, 0x7F, 
                        0x69, 0x01, 0x50, 0x02, 0x70, 0x05, 0xA9, 0x22,
                        0x8D, 0x00, 0x02, 0xA9, 0x00, 0x69, 0x02, 0x70, 
                        0x02, 0x50, 0x05, 0xA9, 0x22, 0x8D, 0x00, 0x02,
                        0xEA
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0x02);
                CHECK(cpu->x() == 0x0A);
                CHECK(cpu->y() == 0x00);
                CHECK(cpu->p()  == 0x20);
                CHECK(cpu->pc() == 0x0651);
                CHECK(cpu->sp() == 0xFF);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x00:
                                CHECK(cpu->read_byte(i) == 0xFF);
                                break;
                        case 0x02:
                                CHECK(cpu->read_byte(i) == 0xFF);
                                break;
                        case 0x03:
                                CHECK(cpu->read_byte(i) == 0x32);
                                break;
                        case 0x04:
                                CHECK(cpu->read_byte(i) == 0x0A);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }

        SECTION("The program jumps to some subroutines")
        {
                /**
                 LDA #$FF
                 STA $00
                 JMP after
                 
                 sub:
                   INC $00
                   INC $01
                   INC $02
                   RTS
                 
                 after:
                   PHP
                   JSR sub
                   PHP
                   JSR sub
                   PHP
                   JSR end
                 
                 end:
                   NOP
                */

                Emulator::ByteVector program {
                        0xA9, 0xFF, 0x85, 0x00, 0x4C, 0x0E, 0x06, 0xE6, 
                        0x00, 0xE6, 0x01, 0xE6, 0x02, 0x60, 0x08, 0x20,
                        0x07, 0x06, 0x08, 0x20, 0x07, 0x06, 0x08, 0x20, 
                        0x1A, 0x06, 0xEA
                };

                Emulator::UniqueCPU cpu = execute_example_program(program);
                CHECK(cpu->a() == 0xFF);
                CHECK(cpu->x() == 0x00);
                CHECK(cpu->y() == 0x00);
                CHECK(cpu->p()  == 0x20);
                CHECK(cpu->pc() == 0x061B);
                CHECK(cpu->sp() == 0xFA);
                for (unsigned i = 0;
                     i < program_start;
                     ++i) {
                        switch (i) {
                        case 0x00:
                                CHECK(cpu->read_byte(i) == 0x01);
                                break;
                        case 0x01:
                                CHECK(cpu->read_byte(i) == 0x02);
                                break;
                        case 0x02:
                                CHECK(cpu->read_byte(i) == 0x02);
                                break;
                        case 0x01FB:
                                CHECK(cpu->read_byte(i) == 0x19);
                                break;
                        case 0x01FC:
                                CHECK(cpu->read_byte(i) == 0x06);
                                break;
                        case 0x01FD:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FE:
                                CHECK(cpu->read_byte(i) == 0x20);
                                break;
                        case 0x01FF:
                                CHECK(cpu->read_byte(i) == 0xA0);
                                break;
                        default:
                                CHECK(cpu->read_byte(i) == 0x00);
                                break;
                        }
                }
        }
        
        // TODO Test BRK, if that's even possible
}

