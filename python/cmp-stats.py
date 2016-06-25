import argparse
import matplotlib.pyplot as plt

import utils

def main(inputFiles,
         mean = ('m', False, ''),
         corr = ('c', False, ''),
         tableFmt = ('t', 'psql', '')):
    if tableFmt:
        utils.setDefaultTableFormat(tableFmt)

    for inputFile in inputFiles:
        metric, e = utils.guessMetric(inputFile), utils.guessExperiment(inputFile)
        print(metric, e)
        valsDict = utils.collectTotalsValues(inputFile)
        items = sorted(valsDict.items())
        if mean: utils.printStats(items)
        if corr: utils.printCorrelation(items)

ap = argparse.ArgumentParser(
    description='''Outputs in a table statistics about the metrics' output''')
ap.add_argument('files', metavar='files', nargs='+', help='Files to process')
ap.add_argument('--mean', '-m', help='Calculate mean', action='store_true')
ap.add_argument('--correlation', '-c', help='Calculate correlations', action='store_true')
ap.add_argument('--tableFmt', '-t', nargs=1, help='Table format')

args = ap.parse_args()

if __name__ == '__main__':
    main(args.files,
         args.mean,
         args.correlation,
         args.tableFmt)
