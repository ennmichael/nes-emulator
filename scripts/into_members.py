#!/usr/bin/env python3.7
# vim: set shiftwidth=4 tabstop=4:

from __future__ import annotations
from typing import Tuple, Optional, List, Iterable
import io


class NotAFunction(Exception):
    pass


def first_word(s: str) -> str:
    return s[:s.find(' ')]


class CppFunction:
    def __init__(self, source_lines: List[str]) -> None:
        if first_word(source_lines[0]) == 'template':
            self.template: Optional[str] = source_lines[0]
            source_lines = source_lines[1:]
        else:
            self.template = None

        try:
            body_start = next(
                k for k, v in enumerate(source_lines) if v.startswith('{')
            )
            body_end = next(
                k for k, v in enumerate(source_lines) if v.startswith('}')
            )
        except StopIteration:
            raise NotAFunction
        else:
            declaration = '\n'.join(source_lines[:body_start])
            open_bracket = declaration.find('(')
            closed_bracket = declaration.rfind(')')
            return_type = first_word(declaration)

            if (open_bracket == -1 or closed_bracket == -1 or
                return_type not in ('void', 'Instruction', 'Byte', 'bool')):
                raise NotAFunction

            self.return_type = return_type
            self.name = declaration[len(self.return_type)+1:open_bracket]
            self.arguments = [
                s.strip() for s in declaration[open_bracket+1:closed_bracket].split(',')
            ]
            self.decl_specifier = declaration[closed_bracket+2:]
            self.body = source_lines[body_start+1:body_end]
            self.static = False

    def is_member(self) -> bool:
        return '::' in self.name

    def to_member(self, target_class: str, target_object: str) -> None:
        try:
            this_argument = next(a for a in self.arguments if target_class in a)
        except StopIteration:
            self.static = True
        else:
            if 'const' in this_argument:
                self.decl_specifier = f'const {self.decl_specifier}'
            self.arguments = [a for a in self.arguments if target_class not in a]
            self.body = [
                l.replace(f'{target_object}.', '')
                 .replace(f'({target_object}, ', '(')
                 .replace(f'({target_object})', '()')
                for l in self.body
            ]

    def declaration(self, *, include_semicolon: bool=True) -> str:
        arguments = ', '.join(self.arguments)
        return ((f'{self.template}\n' if self.template else '') +
                ('static ' if self.static else '') +
                f'{self.return_type} {self.name}({arguments})' +
                (f' {self.decl_specifier}' if self.decl_specifier else '') +
                (';' if include_semicolon else ''))
                

    def source_code(self) -> str:
        body = '\n'.join(self.body)
        return (f'{self.declaration(include_semicolon=False)}\n' +
                f'{{\n{body}\n}}')


def load_functions(file_name: str) -> Iterable[CppFunction]:
    with io.open(file_name) as f:
        source_lines = f.read().splitlines(keepends=False)
        while source_lines:
            try:
                fun = CppFunction(source_lines)
                yield fun
            except NotAFunction:
                source_lines = source_lines[1:]
            else:
                source_lines = source_lines[fun.source_code().count('\n')+2:]


TARGET_CLASS = 'CPU'
TARGET_OBJECT = 'cpu'
TARGET_FILE_NAME = 'src/instruction_set.cpp'


if __name__ == '__main__':
    functions = list(load_functions(TARGET_FILE_NAME))
    for f in functions:
        if not f.is_member():
            f.to_member(TARGET_CLASS, TARGET_OBJECT)
    
    functions.sort(key=lambda f: f.template is None)

    for f in functions:
        print(f.source_code(), end='\n\n')

