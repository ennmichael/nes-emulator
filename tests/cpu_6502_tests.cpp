#include "catch.hpp"
#include "../src/cpu.h"
#include "../src/mem.h"
#include <utility>
#include <array>

namespace {

unsigned constexpr program_start = 0x600u;

void write_program(Emulator::CPU& cpu, Emulator::Bytes const& program) noexcept
{
        for (unsigned i = 0; i < program.size(); ++i) {
                auto const byte = program[i];
                cpu.ram.write_byte(program_start + i, byte);
        }
}

}

SCENARIO("6502 instructions work")
{
        GIVEN("The CPU")
        {
                Emulator::CPU cpu {
                        .pc = program_start
                };

                WHEN("Some values are loaded into the regisers")
                {
                        /**
                          LDA #$01
                          LDX #$02
                          LDY #$03
                         */

                        Emulator::Bytes program {
                                0xA9, 0x01, 0xA2, 0x02, 0xA0, 0x03
                        };
                        
                        write_program(cpu, program);
                        cpu.execute_program(program.size());

                        THEN("The results are correct")
                        {
                                CHECK(cpu.a == 0x01);
                                CHECK(cpu.x == 0x02);
                                CHECK(cpu.y == 0x03);
                                CHECK(cpu.p == 0x20);
                                CHECK(cpu.pc == 0x0606);
                                CHECK(cpu.sp == 0xFF);
                                for (unsigned i = 0;
                                     i < program_start;
                                     ++i) {
                                        CHECK(cpu.ram.read_byte(i) == 0x00);
                                }
                        }
                }

                WHEN("Some values are loaded and stored")
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

                        Emulator::Bytes program {
                                0xA9, 0x01, 0xA2, 0x05, 0xA0, 0x0A, 0x85, 0x00, 
                                0x95, 0x01, 0x8D, 0x00, 0x03, 0x9D, 0x11, 0x03,
                                0x99, 0x11, 0x03, 0xA9, 0x70, 0x85, 0x70, 0xA9, 
                                0x03, 0x85, 0x71, 0xA9, 0xDF, 0x91, 0x70, 0xA9,
                                0xDD, 0x81, 0x6B, 0x86, 0x35, 0x96, 0x35, 0x8E, 
                                0x50, 0x04, 0x84, 0x45, 0x94, 0x45, 0x8C, 0x60,
                                0x04
                        };

                        write_program(cpu, program);
                        cpu.execute_program(program.size());

                        THEN("The results are correct")
                        {
                                CHECK(cpu.a == 0xDD);
                                CHECK(cpu.x == 0x05);
                                CHECK(cpu.y == 0x0A);
                                CHECK(cpu.p == 0xA0);
                                CHECK(cpu.pc == 0x0631);
                                CHECK(cpu.sp == 0xFF);
                                for (unsigned i = 0;
                                     i < program_start;
                                     ++i) {
                                        switch (i) {
                                        case 0x00:
                                                CHECK(cpu.ram.read_byte(i) == 0x01);
                                                break;
                                        case 0x06:
                                                CHECK(cpu.ram.read_byte(i) == 0x01);
                                                break;
                                        case 0x35:
                                                CHECK(cpu.ram.read_byte(i) == 0x05);
                                                break;
                                        case 0x3F:
                                                CHECK(cpu.ram.read_byte(i) == 0x05);
                                                break;
                                        case 0x45:
                                                CHECK(cpu.ram.read_byte(i) == 0x0A);
                                                break;
                                        case 0x4A:
                                                CHECK(cpu.ram.read_byte(i) == 0x0A);
                                                break;
                                        case 0x70:
                                                CHECK(cpu.ram.read_byte(i) == 0x70);
                                                break;
                                        case 0x71:
                                                CHECK(cpu.ram.read_byte(i) == 0x03);
                                                break;
                                        case 0x0300:
                                                CHECK(cpu.ram.read_byte(i) == 0x01);
                                                break;
                                        case 0x0316:
                                                CHECK(cpu.ram.read_byte(i) == 0x01);
                                                break;
                                        case 0x031B:
                                                CHECK(cpu.ram.read_byte(i) == 0x01);
                                                break;
                                        case 0x0370:
                                                CHECK(cpu.ram.read_byte(i) == 0xDD);
                                                break;
                                        case 0x037A:
                                                CHECK(cpu.ram.read_byte(i) == 0xDF);
                                                break;
                                        case 0x0450:
                                                CHECK(cpu.ram.read_byte(i) == 0x05);
                                                break;
                                        case 0x0460:
                                                CHECK(cpu.ram.read_byte(i) == 0x0A);
                                                break;
                                        default:
                                                CHECK(cpu.ram.read_byte(i) == 0x00);
                                                break;
                                        }
                                }
                        }
                }

                WHEN("Some P and A register values are pushed "
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

                        Emulator::Bytes program {
                                0xA9, 0x11, 0x48, 0xA9, 0x00, 0x48, 0x08, 0x68, 0x28
                        };

                        write_program(cpu, program);
                        cpu.execute_program(program.size());

                        THEN("The results are correct")
                        {
                                CHECK(cpu.a == 0x22);
                                CHECK(cpu.x == 0x00);
                                CHECK(cpu.y == 0x00);
                                CHECK(cpu.p == 0x20);
                                CHECK(cpu.pc == 0x0609);
                                CHECK(cpu.sp == 0xFE);
                                for (unsigned i = 0;
                                     i < program_start;
                                     ++i) {
                                        switch (i) {
                                        case 0x01FD:
                                                CHECK(cpu.ram.read_byte(i) == 0x22);
                                                break;
                                        case 0x01FF:
                                                CHECK(cpu.ram.read_byte(i) == 0x11);
                                                break;
                                        default:
                                                CHECK(cpu.ram.read_byte(i) == 0x00);
                                                break;
                                        }
                                }
                        }
                }

                WHEN("Some values are added to the accumulator")
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

                        Emulator::Bytes program {
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

                        write_program(cpu, program);
                        cpu.execute_program(program.size());

                        THEN("The results are correct")
                        {
                                CHECK(cpu.a == 0xA3);
                                CHECK(cpu.x == 0x00);
                                CHECK(cpu.y == 0x00);
                                CHECK(cpu.p == 0xA0);
                                CHECK(cpu.pc == 0x0657);
                                CHECK(cpu.sp == 0xE9);
                                for (unsigned i = 0;
                                     i < program_start;
                                     ++i) {
                                        switch (i) {
                                        case 0x04:
                                                CHECK(cpu.ram.read_byte(i) == 0x02);
                                                break;
                                        case 0x01EA:
                                                CHECK(cpu.ram.read_byte(i) == 0xA0);
                                                break;
                                        case 0x01EB:
                                                CHECK(cpu.ram.read_byte(i) == 0xA1);
                                                break;
                                        case 0x01EC:
                                                CHECK(cpu.ram.read_byte(i) == 0xA1);
                                                break;
                                        case 0x01ED:
                                                CHECK(cpu.ram.read_byte(i) == 0x9F);
                                                break;
                                        case 0x01EE:
                                                CHECK(cpu.ram.read_byte(i) == 0x23);
                                                break;
                                        case 0x01F0:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01F1:
                                                CHECK(cpu.ram.read_byte(i) == 0x51);
                                                break;
                                        case 0x01F2:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01F3:
                                                CHECK(cpu.ram.read_byte(i) == 0x51);
                                                break;
                                        case 0x01F4:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01F5:
                                                CHECK(cpu.ram.read_byte(i) == 0x51);
                                                break;
                                        case 0x01F6:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01F7:
                                                CHECK(cpu.ram.read_byte(i) == 0x31);
                                                break;
                                        case 0x01F8:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01F9:
                                                CHECK(cpu.ram.read_byte(i) == 0x11);
                                                break;
                                        case 0x01FA:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01FB:
                                                CHECK(cpu.ram.read_byte(i) == 0x0F);
                                                break;
                                        case 0x01FC:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01FD:
                                                CHECK(cpu.ram.read_byte(i) == 0x0D);
                                                break;
                                        case 0x01FE:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01FF:
                                                CHECK(cpu.ram.read_byte(i) == 0x0B);
                                                break;
                                        case 0x0200:
                                                CHECK(cpu.ram.read_byte(i) == 0x02);
                                                break;
                                        default:
                                                CHECK(cpu.ram.read_byte(i) == 0x00);
                                                break;
                                        }
                                }
                        }
                }

                WHEN("Some values are AND'ed")
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

                        Emulator::Bytes program {
                                0xA9, 0x22, 0x85, 0x05, 0x8D, 0x00, 0x03, 0xA9, 
                                0x33, 0x29, 0xF0, 0x08, 0x48, 0x25, 0x05, 0x08,
                                0x48, 0xA2, 0x03, 0xA0, 0x02, 0x35, 0x02, 0x08, 
                                0x48, 0x2D, 0x00, 0x03, 0x08, 0x48, 0x3D, 0xFD,
                                0x02, 0x08, 0x48, 0x39, 0xFE, 0x02, 0x08, 0x48, 
                                0xA9, 0x00, 0x85, 0x40, 0xA9, 0x03, 0x85, 0x41,
                                0xA2, 0x30, 0x21, 0x10, 0x08, 0x48, 0xA0, 0x02, 
                                0x31, 0x41, 0x08, 0x48
                        };

                        write_program(cpu, program);
                        cpu.execute_program(program.size());

                        THEN("The results are correct")
                        {
                                CHECK(cpu.a == 0x02);
                                CHECK(cpu.x == 0x30);
                                CHECK(cpu.y == 0x02);
                                CHECK(cpu.p == 0x20);
                                CHECK(cpu.pc == 0x063C);
                                CHECK(cpu.sp == 0xEF);
                                for (unsigned i = 0;
                                     i < program_start;
                                     ++i) {
                                        switch (i) {
                                        case 0x05:
                                                CHECK(cpu.ram.read_byte(i) == 0x22);
                                                break;
                                        case 0x41:
                                                CHECK(cpu.ram.read_byte(i) == 0x03);
                                                break;
                                        case 0x01F0:
                                                CHECK(cpu.ram.read_byte(i) == 0x02);
                                                break;
                                        case 0x01F1:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01F2:
                                                CHECK(cpu.ram.read_byte(i) == 0x02);
                                                break;
                                        case 0x01F3:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01F4:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01F5:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01F6:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01F7:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01F8:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01F9:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01FA:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01FB:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01FC:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01FD:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01FE:
                                                CHECK(cpu.ram.read_byte(i) == 0x30);
                                                break;
                                        case 0x01FF:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x0300:
                                                CHECK(cpu.ram.read_byte(i) == 0x22);
                                                break;
                                        default:
                                                CHECK(cpu.ram.read_byte(i) == 0x00);
                                                break;
                                        }
                                }
                        }
                }
                
                WHEN("Some memory is DEC'ed")
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

                        Emulator::Bytes program {
                                0xA9, 0x42, 0x85, 0x05, 0x8D, 0x00, 0x04, 0x8D, 
                                0x01, 0x04, 0xC6, 0x00, 0x08, 0xA2, 0x02, 0xD6,
                                0x03, 0x08, 0xD6, 0x03, 0x08, 0xCE, 0x00, 0x04, 
                                0x08, 0xDE, 0xFD, 0x03, 0x08
                        };

                        write_program(cpu, program);
                        cpu.execute_program(program.size());

                        THEN("The results are correct")
                        {
                                CHECK(cpu.a == 0x42);
                                CHECK(cpu.x == 0x02);
                                CHECK(cpu.y == 0x00);
                                CHECK(cpu.p == 0xA0);
                                CHECK(cpu.pc == 0x061D);
                                CHECK(cpu.sp == 0xFA);
                                for (unsigned i = 0;
                                     i < program_start;
                                     ++i) {
                                        switch (i) {
                                        case 0x00:
                                                CHECK(cpu.ram.read_byte(i) == 0xFF);
                                                break;
                                        case 0x05:
                                                CHECK(cpu.ram.read_byte(i) == 0x40);
                                                break;
                                        case 0x01FB:
                                                CHECK(cpu.ram.read_byte(i) == 0xA0);
                                                break;
                                        case 0x01FC:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01FD:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01FE:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01FF:
                                                CHECK(cpu.ram.read_byte(i) == 0xA0);
                                                break;
                                        case 0x03FF:
                                                CHECK(cpu.ram.read_byte(i) == 0xFF);
                                                break;
                                        case 0x0400:
                                                CHECK(cpu.ram.read_byte(i) == 0x41);
                                                break;
                                        case 0x0401:
                                                CHECK(cpu.ram.read_byte(i) == 0x42);
                                                break;
                                        default:
                                                CHECK(cpu.ram.read_byte(i) == 0x00);
                                                break;
                                        }
                                }
                        }
                }

                WHEN("Some memory is INC'ed")
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

                        Emulator::Bytes program {
                                0xA9, 0x42, 0x85, 0x05, 0x8D, 0x00, 0x04, 0x8D, 
                                0x01, 0x04, 0xE6, 0x00, 0x08, 0xA2, 0x02, 0xF6,
                                0x03, 0x08, 0xF6, 0x03, 0x08, 0xEE, 0x00, 0x04, 
                                0x08, 0xFE, 0xFD, 0x03, 0x08
                        };

                        write_program(cpu, program);
                        cpu.execute_program(program.size());

                        THEN("The results are correct")
                        {
                                CHECK(cpu.a == 0x42);
                                CHECK(cpu.x == 0x02);
                                CHECK(cpu.y == 0x00);
                                CHECK(cpu.p == 0x20);
                                CHECK(cpu.pc == 0x061D);
                                CHECK(cpu.sp == 0xFA);
                                for (unsigned i = 0;
                                     i < program_start;
                                     ++i) {
                                        switch (i) {
                                        case 0x00:
                                                CHECK(cpu.ram.read_byte(i) == 0x01);
                                                break;
                                        case 0x05:
                                                CHECK(cpu.ram.read_byte(i) == 0x44);
                                                break;
                                        case 0x01FB:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01FC:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01FD:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01FE:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x01FF:
                                                CHECK(cpu.ram.read_byte(i) == 0x20);
                                                break;
                                        case 0x03FF:
                                                CHECK(cpu.ram.read_byte(i) == 0x01);
                                                break;
                                        case 0x0400:
                                                CHECK(cpu.ram.read_byte(i) == 0x43);
                                                break;
                                        case 0x0401:
                                                CHECK(cpu.ram.read_byte(i) == 0x42);
                                                break;
                                        default:
                                                CHECK(cpu.ram.read_byte(i) == 0x00);
                                                break;
                                        }
                                }
                        }
                }
                
        }
}

