from opster import command
import csv, numpy as np
import scipy.stats as stats
import math, matplotlib.pyplot as plt
from tabulate import tabulate as table

def someColor():
    someColor.colors = np.roll(someColor.colors, 1)
    return someColor.colors[0]
someColor.colors = np.array([ x for x in 'rgbcmyk'])

fftSize = 512.0
sampleRate = 24000.0
timeStep = 0.01
frameTimeStep = sampleRate / fftSize

@command()
def main(inputFile,
         mean = ('m', False, ''),
         corr = ('c', False, '')):
    with open(inputFile) as f:
        reader = csv.reader(f, delimiter = '\t')
        headers = reader.__next__()[1:]
        #print(headers)
        measures = [ (x, []) for x in headers ]
        rowVals = []
        for row in map(lambda x: map(float, x), reader):
            row = [ x for x in row ]
            for x in row:
                if math.isnan(x):
                    print(row)
            rowVals.append(row[1:])

        valsDict = {}
        totalValsDict = {}
        for i, h in enumerate(headers):
            vals = [ x[i] for x in rowVals]
            totalValsDict[h] = vals[0]
            valsDict[h] = vals[1:]
            #print(h, len(valsDict[h]), '=', totalValsDict[h])

        duration = totalValsDict['FrameStart']
        del totalValsDict['FrameStart']
        del valsDict['FrameStart']

        items = sorted(valsDict.items())
        if mean: printStats(items)
        if corr: printCorrelation(items)

        #plt.show()

def printStats(items):
    for h, v in items:
        plt.plot(range(0, len(v)), v, color=someColor())

    rows = ([
        [ h, np.mean(vals), np.median(vals), stats.tstd(vals) ] for h, vals in items
    ])
    print(table(rows,
                headers = [ 'mean', 'median', 'std dev' ],
                tablefmt = "psql"))

def printCorrelation(items):
    rows = ([[h] + [
        stats.pearsonr(v, v2)[0] for a, v2 in items
    ] for h, v in items])

    print(table(rows,
                headers = [ key for key, val in items],
                tablefmt = "psql"))


main.command()
