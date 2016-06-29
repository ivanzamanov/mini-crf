import sys
import matplotlib.pyplot as plt
import itertools
from tabulate import tabulate as table

import utils

def normalizeExperiment(allValues, e):
    maxVals = {}
    for metric in utils.METRICS:
        # find the maximum
        maxValue = max(
            map(
                lambda x: x.value,
                filter(lambda x: x.metric == metric and x.experiment == e,
                       allValues)
            )
        )

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

def getColor(metric):
    return {
        'LogSpectrumCritical': 'blue',
        'LogSpectrum': 'black',
        'MFCC': 'red',
        'WSS': 'yellow',
        'SegSNR': 'green',
        'baseline': 'cyan'
    }[metric]

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

    p.set_xticks(range(0, len(utils.METRICS)))
    p.set_xticklabels([ x for x in map(utils.shortenMetric, utils.METRICS)], rotation=-75)

    p.set_title(target)
    return p

def main(args):
    if not args:
        print('''Creates graphics per experiment from outputs''')

    allValues = utils.collectAllTotalsValues(args)
    printTable(allValues)
    normalize(allValues)

    F = 1

    baselineValues = []
    for e, m in itertools.product(utils.EXPERIMENTS, utils.METRICS):
        metricValues = [ val.value for val
                         in filter(lambda x: x.metric == m and x.experiment == e,
                                   allValues)
        ]
        if not metricValues:
            continue

        #print(e, m, metricValues)
        fig = plt.figure(figsize=(2 * F, 4 * F))
        subplot = getSubplot(fig, m)

        for offset, value in zip(range(0, len(metricValues)), metricValues):
            subplot.bar(offset, (1 - value), 0.5,
                        color=getColor(m))

        fig.savefig(e + '-' + m + '.jpg', bbox_inches='tight')
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
