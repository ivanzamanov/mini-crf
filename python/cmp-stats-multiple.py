import sys, numpy as np
import matplotlib.pyplot as plt
import itertools
from tabulate import tabulate as table

from utils import *

def normalizeExperiment(allValues, e):
    maxVals = {}
    for metric in METRICS:
        # find the maximum
        try:
            maxValue = max(
                map(
                    lambda x: x.value,
                    filter(lambda x: x.metric == metric and x.experiment == e,
                           allValues)
                )
            )
        except:
            maxValue = 1

        maxVals[metric] = maxValue

        for x in allValues:
            if x.metric == metric and x.experiment == e:
                x.value = x.value / maxValue

    return maxVals

def normalize(allValues):
    maxVals = {}
    for e in EXPERIMENTS:
        maxVals[e] = normalizeExperiment(allValues, e)
    return maxVals

def getMetricColor(metric):
    return {
        'LogSpectrumCritical': 'blue',
        'LogSpectrum': 'black',
        'MFCC': 'red',
        'WSS': 'yellow',
        'SegSNR': 'green',
        'baseline': 'cyan'
    }[metric]

def getExperimentColor(e):
    return {
        'baseline-test': 'black',
        'baseline-eval': 'blue',
        'e2-test': 'yellow',
        'e2-eval': 'green',
        'e3-test': 'magenta',
        'e3-eval': 'cyan',
        'e4-test': 'grey',
        'e4-eval': 'brown',
    }[e]

def getHatch(metric):
    return ''
    return {
        'LogSpectrumCritical': '/',
        'LogSpectrum': '\\',
        'MFCC': '|',
        'WSS': '-',
        'SegSNR': 'o',
        'baseline': '0'
    }[metric]

def getSubplot(fig, target):
    p = fig.add_subplot(1, 1, 1)

    #p.grid()
    p.axes.set_ylim([-0.01, 0.1])
    p.axes.set_xlabel(r'')
    p.axes.set_ylabel(r'')

    tickLabels = list(map(shortenMetric, TARGETS))
    p.set_xticks(calculateXticks())
    p.set_xticklabels(tickLabels, rotation=-75)
    print(calculateXticks())
    print(tickLabels)

    p.set_title(target)
    return p

def plotMetricBars(m,
                   allValues,
                   subplot):
    for target in METRICS:
        for e in filter(lambda x: not 'baseline' in x, EXPERIMENTS):
            metricValues = list(filter(lambda x: x.metric == m and
                                       x.experiment == e and
                                       x.target == target,
                                       allValues))
            if not metricValues:
                print('No value for', e, m, 'in', target)
                value = 0
            else:
                value = metricValues[0].value
            plotSingleBar(e, target, value, plot=subplot)

spacer = 2
def calculateXticks():
    return list(map(lambda i: i * (len(TARGETS) + spacer), range(1, len(TARGETS) + 1)))

barWidth = 1
def plotSingleBar(e, t, v, plot):
    targetIndex = TARGETS.index(t)
    barIndex = EXPERIMENTS.index(e)
    color = getExperimentColor(e)

    offset = (targetIndex * (len(TARGETS) + spacer))
    subOffset = barIndex * barWidth

    barOffset = offset + subOffset
    #print(barOffset)
    barValue = 1 - v if v != 0 else 0.0001
    plot.bar(barOffset, # offset from origin
             barValue, # value
             barWidth, # width
             color=color)

def main(args):
    if not args:
        print('''Creates graphics per metric from outputs''')

    allValues = collectAllTotalsValues(args)
    #printTable(allValues)
    normalize(allValues)

    F = 1

    baselineValues = []
    for m in METRICS:
        fig = plt.figure(figsize=(8 * F, 4 * F))
        subplot = getSubplot(fig, m)

        plotMetricBars(m, allValues,
                       subplot = subplot)

        graphicName = 'exp' + '-' + m + '.jpg'
        print('Saving', graphicName)
        fig.savefig(graphicName, bbox_inches='tight')
        plt.close()

def printTable(allValues):
    tableHeaders = [ 'E?', 'Metric' ] + TARGETS
    rows = []
    for e, m in itertools.product(EXPERIMENTS, METRICS):
        metricValues = [ val.value for val
                         in filter(lambda x: x.metric == m and x.experiment == e,
                                   allValues)
        ]
        if not metricValues:
            continue
        row = [ e, m ]
        for value in metricValues:
            row.append('%0.5f' % value)
        while len(row) < len(METRICS) + 2:
            row.append('0')
        rows.append(row)

    print(table(rows, headers=tableHeaders, tablefmt='latex'))

if __name__ == '__main__':
    main(sys.argv[1:])
