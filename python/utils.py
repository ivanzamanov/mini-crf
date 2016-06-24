import csv, numpy as np
import sys, math
import scipy.stats as stats
from tabulate import tabulate as table

defaultTableFormat = 'psql'

featureMap = {
    'trans-ctx': 'f_2',
    'trans-mfcc': 'f_3',
    'trans-pitch': 'f_1',
    'state-pitch': 'g_1',
    'state-duration': 'g_2',
    'state-energy': 'g_3',
    'trans-baseline': 'f_0'
}
def featureToSymbol(name):
    if name in featureMap:
        return featureMap[name]
    return name

metrics = [ 'LogSpectrumCritical', 'LogSpectrum', 'MFCC', 'WSS', 'SegSNR', 'baseline' ]
def shortenMetric(metric):
    return {
        'LogSpectrumCritical': 'LSC',
        'LogSpectrum': 'LS',
        'MFCC': 'MFCC',
        'SegSNR': 'SegSNR',
        'WSS': 'WSS',
        'baseline': 'bl'
    }[metric]

def guessMetric(fileName):
    for m in metrics:
        if m in fileName:
            return m
    return ''

def guessExperiment(fileName):
    if 'baseline' in fileName:
        return 'baseline'
    if '-pb-' in fileName:
        return 'e3'
    return 'e2'

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

    hv.sort()
    hv = [ (h, '%0.4f' % v) for h, v in hv ]
    return hv


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
