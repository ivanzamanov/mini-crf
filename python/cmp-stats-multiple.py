import sys
import numpy as np
import matplotlib.pyplot as plt

import utils

def normalize(allValues):
    maxes = {}
    for e, vals in allValues.items():
        for target, runVals in vals.items():
            for metric, metricVals in runVals.items():
                if not metric in maxes:
                    maxes[metric] = 0
                maxes[metric] = max(maxes[metric], np.mean(metricVals))

    for e, vals in allValues.items():
        for target, runVals in vals.items():
            for metric, metricVals in runVals.items():
                runVals[metric] = runVals[metric] / maxes[metric]

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

N_METRICS = 11
METRICS = [ 'LogSpectrumCritical', 'LogSpectrum', 'MFCC', 'WSS', 'SegSNR' ]
def getSubplot(fig, target):
    p = fig.add_subplot(1, 1, 1)

    p.grid()
    p.axes.set_ylim([0, 1])
    p.axes.set_xlabel(r'')
    p.axes.set_ylabel(r'')

    p.set_xticks(range(0, len(METRICS)))
    p.set_xticklabels([ x for x in map(utils.shortenMetric, METRICS)], rotation=-75)

    p.set_title(target)
    return p

def main(args):
    if not args:
        print('''Creates graphics per experiment from outputs''')
    allValues = {
        'e2': {},
        'e3': {},
        'baseline': {}
    }

    for f in args:
        values = utils.collectTotalsValues(f)
        metric = utils.guessMetric(f)
        e = utils.guessExperiment(f)

        allValues[e][metric] = values

    normalize(allValues)
    plotNumber = 0
    plots = []
    for experiment, expValues in sorted(allValues.items()):
        for target, optValues in sorted(expValues.items()):
            print(experiment, target)
            plotNumber += 1

            F = 1
            fig = plt.figure(figsize=(2 * F, 4 * F))

            subplot = getSubplot(fig, target)
            plots.append((subplot, target))
            offset = 0
            for metric, values in sorted(optValues.items()):
                v = np.mean(values)
                subplot.bar(offset, v, 0.5,
                            hatch=getHatch(metric),
                            color=getColor(metric))
                offset += 1

            fig.savefig(experiment + '-optimized-' + target + '.jpg', bbox_inches='tight')
            plt.close()

if __name__ == '__main__':
    main(sys.argv[1:])
