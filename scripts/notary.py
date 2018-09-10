#!/usr/bin/env python3.6
# vim: set shiftwidth=4 tabstop=4:

import textwrap
import enum
import sys
import functools
from typing import NamedTuple, Any


# TODO Don't erase the config line

@enum.unique
class SectionKind(enum.Enum):
    
    TITLE = enum.auto()
    BODY = enum.auto()


class Section(NamedTuple):

    text: Any
    kind: Any

    def formatted_text(self, width):
        if self.kind == SectionKind.TITLE:
            return f'\n\n\n  {self.text.title()}\n\n'
        elif self.kind == SectionKind.BODY:
            return f'{textwrap.fill(self.text).strip()}\n'
        else:
            assert False


Section.title = functools.partial(Section, kind=SectionKind.TITLE)
Section.body = functools.partial(Section, kind=SectionKind.BODY)


def parse_sections(lines):
    body_text = ''
    for line in lines:
        if is_title(line):
            yield Section.title(line.strip())
        elif line.strip():
            body_text += line
        elif body_text:
            yield Section.body(body_text)
            body_text = ''
    if body_text:
        yield Section.body(body_text)


def is_title(line):
    return line.startswith('  ')


def parse_config(config_line):
    config_line = config_line.strip()
    config_start = '(notary width'
    config_end = ')'
    if not config_line.startswith(config_start) or not config_line.endswith(config_end):
        raise RuntimeError('Incorrect input text config')
    width = config_line[len(config_start):-len(config_end)]
    return int(width)


def read_lines(path):
    with open(path) as f:
        return f.read().splitlines(keepends=True)


def write_text(path, text):
    with open(path, 'w') as f:
        f.write(text)


if __name__ == '__main__':
    file_path = 'notes'
    config_line, *lines = read_lines(file_path)
    width = parse_config(config_line)
    sections = parse_sections(lines)
    formatted_text = ''.join(s.formatted_text(width) for s in sections)
    write_text(file_path, config_line + formatted_text)

