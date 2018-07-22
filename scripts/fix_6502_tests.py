#!/usr/bin/env python3.7


import io


TAB = ' ' * 8


def fix_tests(source_lines):
    pull_tabs = False
    for l in source_lines:
        if 'WHEN' in l:
            yield l.replace('WHEN', 'SECTION')
        elif l.startswith(f'{TAB * 2}{{'):
            pull_tabs = True
        elif l.startswith(f'{TAB * 2}}}') and pull_tabs:
            pull_tabs = False
        elif l.strip().startswith('execute_example_program'):
            yield f'{TAB * 2}Emulator::CPU cpu = execute_example_program(program);'
        elif 'THEN' not in l:
            l = ()
            yield l[len(TAB):] if pull_tabs else l


def read_lines():
    with io.open('./tests/cpu_6502_tests.cpp', 'r') as f:
        return f.read().splitlines(keepends=True)


if __name__ == '__main__':
    lines = read_lines()
    with io.open('./tests/cpu_6502_tests.cpp', 'w') as f:
        for l in fix_tests(lines):
            f.write(l)


