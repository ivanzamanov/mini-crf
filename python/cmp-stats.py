from opster import command
import matplotlib.pyplot as plt

import utils

def someColor():
    someColor.colors = np.roll(someColor.colors, 1)
    return someColor.colors[0]
someColor.colors = np.array([ x for x in 'rgbcmyk'])

@command()
def main(inputFile,
         mean = ('m', False, ''),
         corr = ('c', False, ''),
         tableFmt = ('t', 'psql', ''),
         plot=('p', True, ''),
         output=('o', '', '')):
    if tableFmt:
        utils.setDefaultTableFormat(tableFmt)

    valsDict = utils.collectTotalsValues(inputFile)
    items = sorted(valsDict.items())
    if mean: utils.printStats(items)
    if corr: utils.printCorrelation(items)
    

if __name__ == '__main__':
    main.command()
