import argparse
import matplotlib.pyplot as plt
import itertools

from utils import *

def findMatching(files):
    files = sorted(files)
    result = []
    for f1, f2 in itertools.product(files, files):
        if guessTarget(f1) == guessTarget(f2):
            if not (f2, f1) in result and not f1 == f2:
                result.append((f1, f2))

    return result

def findAllMatching(files):
    return list(map(lambda f: (guessTarget(f), guessExperiment(f), f), sorted(files)))

def plotWeights(plot, files):
    styles = ['solid', 'dashed', 'dotted', 'dashdot' ]
    colors = ['red', 'green', 'blue', 'red']
    for f, style, c in zip(files, styles, colors):
        weights = collectConfigWeights(f[2])
        plt.xticks(range(1, len(weights) + 1), [ h for h, v in weights ], rotation=35)

        plt.plot([ v for h, v in weights ], color = c,
                 linestyle = style)


def main(files):
    details = findAllMatching(files)
    print(details)

    for m in METRICS:
        files = list(filter(lambda f: f[0] == m, details))

        fig = plt.figure(figsize=(6, 2))
        ax = plt.subplot(1, 1, 1)
        #plt.grid()
        plt.rc('text', usetex=True)
        plt.ylim([0, 2])
        plt.xlabel(r'Feature')
        plt.ylabel(r'Weight')
        plt.setp(ax.get_xticklabels(), rotation=-35, horizontalalignment='right')

        plt.title(m)

        plotWeights(plt, files)

        output = '-'.join(['config-optimized', m]) + '.jpg'
        print(files, '->', output)
        plt.savefig(output, bbox_inches='tight')

        plt.close()


if __name__ == '__main__':
    ap = argparse.ArgumentParser(
        description='''Generates graphs of feature weights from matching configs.''')
    ap.add_argument('files', metavar='files', nargs='+', help='Files to process')
    args = ap.parse_args()
    main(files = args.files)
