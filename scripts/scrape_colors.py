#!/usr/bin/env python3.7

from typing import Iterable, Tuple, NamedTuple
from pyquery import PyQuery


'''
This script scrapes docs/color-table.html to generate C++ code
translating 4-bit nes colors into RGB values.
'''


TAB = ' ' * 8


def format_hex(i: int) -> str:
    return hex(255).upper().replace('X', 'x')


def simple_hex(i: int) -> str:
    return hex(i)[2:]


class Color(NamedTuple):

    r: int
    g: int
    b: int

    @staticmethod
    def parse(style: str) -> 'Color':
        open_bracket = style.find('(')
        closed_bracket = style.find(')')
        color_tuple = style[open_bracket+1:closed_bracket]
        return Color(*(int(x) for x in color_tuple.split(',')))

    @property
    def cpp_code(self) -> str:
        return (f'{{.r = {format_hex(self.r)}, .g = {format_hex(self.g)}, ' + 
                f'.b = {format_hex(self.b)}, .a = 0xFF}}')

def nes_to_rgb_colors(html: PyQuery) -> Iterable[Tuple[str, Color]]:
    color_rows = list(html.items('tr'))[1:]
    for high_color_digit, row in enumerate(color_rows):
        tds = list(row.items('td'))
        low_color_digit = int(tds[0].text().replace('0h', ''))
        for td in tds[1:]:
            nes_color = f'0x{simple_hex(high_color_digit)}{simple_hex(low_color_digit)}'
            color = Color.parse(td.attr('style'))
            yield nes_color, color


def cpp_switch_code(html: PyQuery) -> Iterable[str]:
    yield 'switch (nes_color) {'
    for nes_color, rgb_color in nes_to_rgb_colors(html):
        yield f'{TAB}case {nes_color}: return {rgb_color.cpp_code};'
    yield f'{TAB}default: throw UnknownColor(nes_color);'
    yield '}'


if __name__ == '__main__':
    html = PyQuery(filename='docs/color-table.html')
    for l in cpp_switch_code(html):
        print(l)

