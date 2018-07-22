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
        elif l.startswith(f'{TAB * 2}}}'):
            pull_tabs = False
        elif l.startswith(TAB * 3):
            yield l[len(TAB):]
        elif 'THEN' not in l:
            yield l[len(TAB):] if pull_tabs else l


if __name__ == '__main__':
    with io.open('./tests/cpu_6502_tests.cpp') as f:
        for l in fix_tests(f.read().splitlines(keepends=False)):
            print(l)


