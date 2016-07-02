import csv, numpy as np, itertools
import sys, math
import scipy.stats as stats
from tabulate import tabulate as table

defaultTableFormat = 'psql'

def guessCorpusSubset(name):
    if 'eval' in name:
        return 'eval'
    return 'test'

def guessFeatureSet(name):
    if 'baseline' in name:
        return 'baseline'
    if 'e4' in name:
        return 'e4'
    if '-pb-' in name or 'e3' in name:
        return 'e3'
    return 'e2'

def guessExperiment(name):
    return guessFeatureSet(name) + '-' + guessCorpusSubset(name)

featureMap = {
    'trans-ctx': 'f_2',
    'trans-mfcc': 'f_3',
    'trans-pitch': 'f_1',
    'state-pitch': 'g_1',
    'state-duration': 'g_2',
    'state-energy': 'g_3',
    'trans-baseline': 'f_0'
}

featureSortMap = {
    'trans-ctx': 1,
    'trans-mfcc': 2,
    'trans-pitch': 3,
    'state-pitch': 4,
    'state-duration': 5,
    'state-energy': 6,
    'trans-baseline': 7
}

experimentSortMap = {
    'baseline-test': 1,
    'baseline-eval': 2,
    'e2-test': 3,
    'e2-eval': 4,
    'e3-test': 5,
    'e3-eval': 6,
    'e4-test': 7,
    'e4-eval': 8,
}

targetsSortMap = {
    'baseline': 1,
    'LogSpectrum': 2,
    'LogSpectrumCritical': 3,
    'MFCC': 4,
    'WSS': 5,
    'SegSNR': 6
}

TARGETS = sorted([ 'LogSpectrumCritical', 'LogSpectrum', 'MFCC', 'WSS', 'SegSNR', 'baseline' ],
                 key = lambda x: targetsSortMap[x])
METRICS = sorted([ 'LogSpectrumCritical', 'LogSpectrum', 'MFCC', 'WSS', 'SegSNR' ])
EXPERIMENTS = sorted([ '-'.join(x) for x in itertools.product([ 'e2', 'e3', 'e4', 'baseline' ], ['test', 'eval'])], key = lambda x: experimentSortMap[guessExperiment(x)] )

def featureToSymbol(name):
    if name in featureMap:
        return featureMap[name]
    return name

def shortenMetric(metric):
    return {
        'LogSpectrumCritical': 'LSC',
        'LogSpectrum': 'LS',
        'MFCC': 'MFCC',
        'SegSNR': 'SegSNR',
        'WSS': 'WSS',
        'baseline': 'bl'
    }[metric]

def guessTarget(fileName):
    # Important to be the last, since LSC is after LS...
    r = ''
    for m in TARGETS:
        if m in fileName:
            r = m
    return r

def collectConfigWeights(inputFile):
    hv = []
    with open(inputFile) as f:
        for line in f.readlines():
            line = line.strip('\n')
            if line.startswith('trans-') or line.startswith('state-'):
                idx = line.index('=')
                h = line[:idx]
                v = float(line[idx+1:])

                hv.append( (h, v) )

    if not [ x for x in filter(lambda x: x[0] == 'trans-baseline', hv) ]:
        hv.append(('trans-baseline', 0))

    hv = sorted(hv, key=lambda x: featureSortMap[x[0]])
    hv = [ (h, '%0.4f' % v) for h, v in hv ]
    return hv

class ExpValue():
    def __init__(self,
                 experiment,
                 target,
                 metric,
                 value):
        self.experiment = experiment
        self.target = target
        self.metric = metric
        self.value = value

    def __repr__(self):
        return str(('e=' + str(self.experiment),
                    't=' + str(self.target),
                    'm=' + str(self.metric),
                    'v=' + str(self.value)))

def collectAllTotalsValues(files):
    result = []
    for f in files:
        values = collectTotalsValues(f)
        target = guessTarget(f)
        e = guessExperiment(f)

        for metric, value in values.items():
            ev = ExpValue(experiment = e,
                          target = target,
                          metric = metric,
                          value = np.mean(value))
            result.append(ev)

    return result

def collectTotalsValues(inputFile):
    ''' Collect values from a csv in a dict, skipping the row column '''
    with open(inputFile) as f:
        reader = csv.reader(f, delimiter = '\t')
        headers = reader.__next__()[1:]
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

    return valsDict

def fmtFloat(fl):
    ''' Format a float to string with 3 decimal places'''
    return '%0.3f' % fl

def printStats(items, tableFormat=''):
    ''' Expects output from collectTotalsValues, i.e.
    a dict with key - metric, and value - list of values'''
    rows = ([
        [ h, fmtFloat(np.sum(vals)), fmtFloat(np.mean(vals)), fmtFloat(np.median(vals)), fmtFloat(stats.tstd(vals)) ] for h, vals in items
    ])
    print(table(rows,
                headers = [ 'sum', 'mean', 'median', 'std dev' ],
                tablefmt = getTableFormat(tableFormat)))

def printCorrelation(items, tableFormat=''):
    ''' Expects output from collectTotalsValues, i.e.
    a dict with key - metric, and value - list of values'''
    rows = ([[h] + [
        fmtFloat(stats.pearsonr(v, v2)[0]) for a, v2 in items
    ] for h, v in items])

    print(table(rows,
                headers = [ key for key, val in items],
                tablefmt = getTableFormat(tableFormat)))

def getTableFormat(fmt):
    if fmt:
        return fmt
    return defaultTableFormat

def setDefaultTableFormat(fmt):
    global defaultTableFormat
    defaultTableFormat = fmt
