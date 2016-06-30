from opster import command
from tabulate import tabulate as table

tableFormat = 'psql'

@command()
def main(confFile,
         tableFmt = ('t', 'psql', '')):
    global tableFormat
    tableFormat = tableFmt

    with open(confFile) as f:
        h = []
        v = []
        for line in f.readlines():
            if line.startswith('trans-') or line.startswith('state-'):
                ind = line.index('=')
                feature = line[:ind]
                value = line[ind + 1:]

                h.append(feature)
                v.append(value)
    print(table([v], headers=h, tablefmt=tableFmt))

main.command()
