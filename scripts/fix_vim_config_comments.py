#!/usr/bin/env python3.6
# vim: set shiftwidth=4 tabstop=4:


'''
This script adds vim config comments to files. It won't fix already existing config comments.
'''


import itertools
import os.path
from glob import glob


SETTINGS = {
    ('py',): '# vim: set shiftwidth=4 tabstop=4:',
    ('cpp', 'c', 'h'): '// vim: set shiftwidth=8 tabstop=8:'
}


def vim_config_comment(extname):
    for extnames, comment in SETTINGS.items():
        if extname in extnames:
            return comment
    return None


def get_extname(path):
    return path.split('.')[-1]


def source_files():
    entries = itertools.chain(glob('src/*'), glob('tests/*'), glob('scripts/*'))
    return (e for e in entries if os.path.isfile(e))


def fix_file(path):
    extname = get_extname(path)
    lines = read_lines(path)
    if not has_vim_config_comment(extname, lines):
        if add_vim_config_comment(extname, lines):
            write_lines(path, lines)


def has_vim_config_comment(extname, lines):
    try:
        config_line = lines[1] if extname == 'py' else lines[0]
    except IndexError:
        return False
    else:
        return config_line.strip() == vim_config_comment(extname)


def add_vim_config_comment(extname, lines):
    config_comment = vim_config_comment(extname)
    if not config_comment:
        return False
    config_line = config_comment + '\n'
    if extname == 'py':
        lines.insert(1, config_line)
    else:
        lines.insert(0, config_line + '\n')
    return True


def fix_vim_config_comment(extname, lines):
    i = 1 if extname == 'py' else 0
    lines[i] = vim_config_comment(extname)


def read_lines(path):
    with open(path) as f:
        return f.read().splitlines(keepends=True)


def write_lines(path, lines):
    with open(path, 'w') as f:
        return f.write(''.join(lines))


if __name__ == '__main__':
    for path in source_files():
        fix_file(path)

