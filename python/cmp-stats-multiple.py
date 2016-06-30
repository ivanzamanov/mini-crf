import sys, numpy as np
import matplotlib.pyplot as plt
import itertools
from tabulate import tabulate as table

import utils

def normalizeExperiment(allValues, e):
    maxVals = {}
    for metric in utils.METRICS:
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
    for e in utils.EXPERIMENTS:
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

    p.grid()
    p.axes.set_ylim([0, 0.1])
    p.axes.set_xlabel(r'')
    p.axes.set_ylabel(r'')

    p.set_xticks(list(map(lambda x: 2 * x, range(0, len(utils.METRICS)))))
    p.set_xticklabels([ x for x in map(utils.shortenMetric, utils.METRICS)], rotation=-75)

    p.set_title(target)
    return p

barWidth = 0.1
expCount = len(utils.EXPERIMENTS)
def plotMetricBars(m,
                   allValues,
                   subplot,
                   fig):
    def plotBarsForTarget(t):
        for barIndex, e in enumerate(utils.EXPERIMENTS):
            metricValues = list(filter(lambda x: x.metric == m and
                                       x.experiment == e and
                                       x.target == t,
                                       allValues))
            if not metricValues:
                print('No values for ', e, m)
                return
            value = metricValues[0]
            print('Calculating', e, m)

            for offset, value in zip(range(0, len(metricValues)), metricValues):
                offset = offset * expCount
                barOffset = offset + barIndex * (2 / 3 * barWidth)
                color = getExperimentColor(e)
                print(e, barOffset, barWidth, color, value.target)
                subplot.bar(barOffset, # offset from origin
                            (1 - value.value), # value
                            barWidth, # width
                            color=color)

    for target in utils.TARGETS:
        plotBarsForTarget(target)


def main(args):
    if not args:
        print('''Creates graphics per experiment from outputs''')

    allValues = utils.collectAllTotalsValues(args)
    printTable(allValues)
    normalize(allValues)

    F = 1

    baselineValues = []
    for m in utils.METRICS:
        fig = plt.figure(figsize=(4 * F, 4 * F))
        subplot = getSubplot(fig, m)

        plotMetricBars(m, allValues,
                       fig = fig,
                       subplot = subplot)

        graphicName = 'exp' + '-' + m + '.jpg'
        print('Saving', graphicName)
        fig.savefig(graphicName, bbox_inches='tight')
        plt.close()

def printTable(allValues):
    tableHeaders = [ 'E?', 'Metric' ] + utils.TARGETS
    rows = []
    for e, m in itertools.product(utils.EXPERIMENTS, utils.METRICS):
        metricValues = [ val.value for val
                         in filter(lambda x: x.metric == m and x.experiment == e,
                                   allValues)
        ]
        if not metricValues:
            continue
        row = [ e, m ]
        for value in metricValues:
            row.append('%0.5f' % value)
        while len(row) < len(utils.METRICS) + 2:
            row.append('0')
        rows.append(row)

    print(table(rows, headers=tableHeaders, tablefmt='latex'))

if __name__ == '__main__':
    main(sys.argv[1:])
