#!/usr/bin/env python3.6
# vim: set shiftwidth=4 tabstop=4:

from pyquery import PyQuery
from typing import NamedTuple, Any


'''
This script scrapes docs/color-table.html to generate C++ code
translating 4-bit nes colors into RGB values.
'''


TAB = ' ' * 8


def color_mappings(html):
    return (
        (td.text(), Color.parse(td.attr('style'))) for td in html.items('td')
    )


class Color(NamedTuple):

    r: Any
    g: Any
    b: Any

    @staticmethod
    def parse(style):
        start = 'border:0px;background-color:#'
        color_length = 6
        color = style[len(start):len(start)+color_length];
        return Color(r=color[:2], g=color[2:4], b=color[4:])

    @property
    def cpp_code(self):
        return f'{{.r = 0x{self.r}, .g = 0x{self.g}, .b = 0x{self.b}, .a = 0xFF}}'


def cpp_switch_code(colors):
    def lines():
        yield 'switch (nes_color) {'
        for nes_color, rgb_color in colors:
            yield f'{TAB}case {nes_color}: return {rgb_color.cpp_code};'
        yield f'{TAB}default: throw UnknownColor(nes_color);'
        yield '}'
    return '\n'.join(lines())


if __name__ == '__main__':
    html = PyQuery(filename='docs/color-table.html')
    colors = color_mappings(html)
    print(cpp_switch_code(colors))

