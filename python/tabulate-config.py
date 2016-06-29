import argparse
import matplotlib.pyplot as plt
import itertools

from utils import *

def findMatching(files):
    result = []
    for f1, f2 in itertools.product(files, files):
        if guessTarget(f1) == guessTarget(f2):
            if not (f2, f1) in result and not f1 == f2:
                result.append((f1, f2))

    return result

def main(files):
    for inputFile1, inputFile2 in findMatching(files):
        hv1 = collectConfigWeights(inputFile1)
        hv2 = collectConfigWeights(inputFile2)

        fig = plt.figure(figsize=(6, 2))
        ax = plt.subplot(1, 1, 1)
        plt.grid()
        plt.rc('text', usetex=True)
        plt.ylim([0, 2])
        plt.xlabel(r'Feature')
        plt.ylabel(r'Weight')

        title = guessTarget(inputFile1)
        plt.title(title)

        plt.xticks(range(1, len(hv1) + 1), [ h for h, v in hv1 ], rotation=35)

        plt.plot([ v for h, v in hv1 ], 'k-')
        plt.plot([ v for h, v in hv2 ], 'k--')

        plt.setp(ax.get_xticklabels(), rotation=-35, horizontalalignment='right')

        metric = guessTarget(inputFile1)
        output = '-'.join(['config-optimized', metric]) + '.jpg'

        print(inputFile1, inputFile2, '->', output)
        plt.savefig(output, bbox_inches='tight')

        plt.close()


if __name__ == '__main__':
    ap = argparse.ArgumentParser(
        description='''Generates graphs of feature weights from matching configs.''')
    ap.add_argument('files', metavar='files', nargs='+', help='Files to process')
    args = ap.parse_args()
    main(files = args.files)
