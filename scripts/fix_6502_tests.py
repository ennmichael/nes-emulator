#!/usr/bin/env python3.7


import io


TAB = ' ' * 8


def fix_tests(source_lines):
    return [
        (l.replace('Emulator::CPU', 'Emulator::UniqueCPU')
          .replace('cpu.a()', 'cpu->a()')
          .replace('cpu.x()', 'cpu->x()')
          .replace('cpu.y()', 'cpu->y()')
          .replace('cpu.pc() ', 'cpu->pc() ')
          .replace('cpu.p()', 'cpu->p() ')
          .replace('cpu.sp()', 'cpu->sp()')
          .replace('cpu.read_byte', 'cpu->read_byte'))
        for l in source_lines
    ]


def read_lines():
    with io.open('./tests/cpu_6502_tests.cpp', 'r') as f:
        return f.read().splitlines(keepends=True)


if __name__ == '__main__':
    lines = read_lines()
    with io.open('./tests/cpu_6502_tests.cpp', 'w') as f:
        for l in fix_tests(lines):
            f.write(l)


