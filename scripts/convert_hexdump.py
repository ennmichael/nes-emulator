def each(n, l):
    for i, e in enumerate(l):
        res = l[i*n:(i+1)*n]

        if not res:
            break

        yield res


if __name__ == '__main__':
    hexdump = input()
    converted = [f'0x{b}' for b in hexdump.split()]

    result = 'Emulator::Bytes const program {\n'
    for e in each(5, converted):
        tab = ' ' * 8
        content = ', '.join(e)
        result += f'{tab}{content},\n'
    result = result[:-2] + '\n};\n'

    print(result)

