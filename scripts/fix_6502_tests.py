#!/usr/bin/env python3
# vim: set shiftwidth=4 tabstop=4:

import io


TAB = ' ' * 8


def fix_tests(source_lines):
    return [fix_line(l) for l in source_lines]


def fix_line(l):
    if l.count('Emulator::UniqueCPU cpu = execute_example_program(program);'):
        return f'{TAB * 2}TestMemory test_memory(program);\n{l.replace("program", "test_memory")}\n'
    else:
        return l


def read_lines():
    with io.open('./tests/cpu_6502_tests.cpp', 'r') as f:
        return f.read().splitlines(keepends=True)


if __name__ == '__main__':
    lines = read_lines()
    with io.open('./tests/cpu_6502_tests.cpp', 'w') as f:
        for l in fix_tests(lines):
            f.write(l)

