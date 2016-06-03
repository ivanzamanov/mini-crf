from opster import command
import csv, numpy as np
import scipy.stats as stats
import math, matplotlib.pyplot as plt
from tabulate import tabulate as table

colors = np.array([ x for x in 'rgbcmyk'])
def someColor():
    global colors
    colors = np.roll(colors, 1)
    return colors[0]

fftSize = 512.0
sampleRate = 24000.0
timeStep = 0.01
frameTimeStep = sampleRate / fftSize

@command()
def main(inputFile):
    with open(inputFile) as f:
        reader = csv.reader(f, delimiter = '\t')
        headers = reader.__next__()[1:]
        print(headers)
        measures = [ (x, []) for x in headers ]
        rowVals = []
        for row in map(lambda x: map(float, x), reader):
            row = [x for x in row]
            for x in row:
                if math.isnan(x):
                    print(row)
            rowVals.append(row[1:])

        valsDict = {}
        for i, h in enumerate(headers):
            valsDict[h] = [ x[i] for x in rowVals]
            print(h, len(valsDict[h]))

        for h, v in valsDict.items():
            plt.plot(range(0, len(v)), v, color=someColor())
            #print(h, v)
            vals = np.array(v)
            print(h, 'mean=', np.mean(vals), 'std=', stats.tstd(vals))

        items = sorted(valsDict.items())
        rows = ([[h] + [
            stats.pearsonr(v, v2)[0] for a, v2 in items
        ] for h, v in items])

        print(table(rows,
                    headers=[ key for key, val in items],
                    tablefmt="latex"))

        #plt.show()
main.command()
