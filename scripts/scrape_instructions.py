#!/usr/bin/env python3


'''
This script scrapes docs/6502opcodes2.html to generate C++ code used to
translate an opcode Byte object into an Instuction object.
'''


from pyquery import PyQuery


def translate_mnemonic(mnemonic_text):
    mnemonic = mnemonic_text.split(' - ')[0]
    mnemonic = mnemonic.lower()

    if mnemonic == 'and':
        return 'bitwise_and'

    return mnemonic


def translate_mode(mode):
    mode = mode.lower()
    return (mode.replace('(', '').replace(')', '')
                .replace(',', '_').replace(' ', '_'))


def parse_mnemonics(html):
    yield from (translate_mnemonic(h3.text()) for h3 in html.items('h3'))


def opcodes_and_modes(body):
    _, *rows = body.items('tr')
    for row in rows:
            tds = list(row.items('td'))
            mode = translate_mode(tds[0].text())
            opcode = f'0x{tds[1].text()[1:]}'
            yield opcode, mode


if __name__ == '__main__':
    html = PyQuery(filename='docs/6502opcodes2.html')
    mnemonics = parse_mnemonics(html)
    bodies = [
        b for b in html.items('tbody')
        if b('tr').eq(0).text().startswith('Addressing Mode')
    ]

    print('switch (opcode) {')

    for mnemonic, body in zip(mnemonics, bodies):
        for opcode, mode in opcodes_and_modes(body):
            print(f'        case {opcode}: return {mode}({mnemonic})');

    print('        default: throw UnknownOpcode(opcode);')

    print('}')

