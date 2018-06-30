#include "catch.hpp"
#include "../src/cpu.h"
#include "../src/mem.h"
#include <utility>
#include <array>

SCENARIO("6502 instructions work")
{
        GIVEN("The CPU")
        {
                Emulator::CPU cpu {
                        .pc = Emulator::TestMemory::ram_size
                };

                WHEN("Some values are loaded into the regisers")
                {
                        /**
                         * LDA #$01
                         * LDX #$02
                         * LDY #$03
                         */

                        Emulator::TestMemory memory({
                                0xA9, 0x01, 0xA2, 0x02, 0xA0, 0x03
                        });

                        cpu.execute_program(memory, memory.program_size());

                        THEN("The results are correct")
                        {
                                CHECK(cpu.a == 0x01);
                                CHECK(cpu.x == 0x02);
                                CHECK(cpu.y == 0x03);
                                CHECK(cpu.p == 0x30);
                                CHECK(cpu.pc == 0x0606);
                                CHECK(cpu.sp == 0xFF);
                                for (unsigned i = 0;
                                     i < Emulator::TestMemory::ram_size;
                                     ++i) {
                                        switch (i) {
                                        default:
                                                CHECK(memory.read_byte(i) == 0x00);
                                                break;
                                        }
                                }
                        }
                }

                WHEN("Some values are loaded and stored")
                {
                        /**
                        * ; Load example values
                        * LDA #$01
                        * LDX #$05
                        * LDY #$0A
                        * 
                        * ; Test storing A in different modes
                        * STA $00
                        * STA $01,X
                        * STA $0300
                        * STA $0311,X
                        * STA $0311,Y
                        * 
                        * ; Test storing A in indirect modes
                        * LDA #$70
                        * STA $70
                        * LDA #$03
                        * STA $71
                        * LDA #$DF
                        * STA ($70),Y
                        * LDA #$DD
                        * STA ($6B,X)
                        * 
                        * ; Test storing X in different modes
                        * STX $35
                        * STX $35,Y
                        * STX $0450
                        * 
                        * ; Test storing Y in different modes
                        * STY $45
                        * STY $45,X
                        * STY $0460
                        */

                        Emulator::TestMemory memory({
                                0xA9, 0x01, 0xA2, 0x05, 0xA0, 0x0A, 0x85, 0x00, 
                                0x95, 0x01, 0x8D, 0x00, 0x03, 0x9D, 0x11, 0x03,
                                0x99, 0x11, 0x03, 0xA9, 0x70, 0x85, 0x70, 0xA9, 
                                0x03, 0x85, 0x71, 0xA9, 0xDF, 0x91, 0x70, 0xA9,
                                0xDD, 0x81, 0x6B, 0x86, 0x35, 0x96, 0x35, 0x8E, 
                                0x50, 0x04, 0x84, 0x45, 0x94, 0x45, 0x8C, 0x60,
                                0x04
                        });

                        cpu.execute_program(memory, memory.program_size());

                        THEN("The results are correct")
                        {
                                CHECK(cpu.a == 0xDD);
                                CHECK(cpu.x == 0x05);
                                CHECK(cpu.y == 0x0A);
                                CHECK(cpu.p == 0xB0);
                                CHECK(cpu.pc == 0x0631);
                                CHECK(cpu.sp == 0xFF);
                                for (unsigned i = 0;
                                     i < Emulator::TestMemory::ram_size;
                                     ++i) {
                                        switch (i) {
                                        case 0x00:
                                                CHECK(memory.read_byte(i) == 0x01);
                                                break;
                                        case 0x06:
                                                CHECK(memory.read_byte(i) == 0x01);
                                                break;
                                        case 0x35:
                                                CHECK(memory.read_byte(i) == 0x05);
                                                break;
                                        case 0x3F:
                                                CHECK(memory.read_byte(i) == 0x05);
                                                break;
                                        case 0x45:
                                                CHECK(memory.read_byte(i) == 0x0A);
                                                break;
                                        case 0x4A:
                                                CHECK(memory.read_byte(i) == 0x0A);
                                                break;
                                        case 0x70:
                                                CHECK(memory.read_byte(i) == 0x70);
                                                break;
                                        case 0x71:
                                                CHECK(memory.read_byte(i) == 0x03);
                                                break;
                                        case 0x0300:
                                                CHECK(memory.read_byte(i) == 0x01);
                                                break;
                                        case 0x0316:
                                                CHECK(memory.read_byte(i) == 0x01);
                                                break;
                                        case 0x031B:
                                                CHECK(memory.read_byte(i) == 0x01);
                                                break;
                                        case 0x0370:
                                                CHECK(memory.read_byte(i) == 0xDD);
                                                break;
                                        case 0x037A:
                                                CHECK(memory.read_byte(i) == 0xDF);
                                                break;
                                        case 0x0450:
                                                CHECK(memory.read_byte(i) == 0x05);
                                                break;
                                        case 0x0460:
                                                CHECK(memory.read_byte(i) == 0x0A);
                                                break;
                                        default:
                                                CHECK(memory.read_byte(i) == 0x00);
                                                break;
                                        }
                                }
                        }
                }
        }
}

