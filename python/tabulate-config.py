import matplotlib.pyplot as plt
from opster import command
from tabulate import tabulate as table

import utils

def tabulateValues(hv, tableFmt):
    print(
        table([[ v for h, v in hv ]],
              headers = [ utils.featureToSymbol(h) for h, v in hv ], tablefmt=tableFmt)
    )
    
@command()
def main(inputFile1,
         inputFile2,
         tableFmt = ('t', 'psql', ''),
         output= ('o', '', ''),
         title=('', '', '')):

    hv1 = utils.collectConfigWeights(inputFile1)
    hv2 = utils.collectConfigWeights(inputFile2)

    tabulateValues(hv1, tableFmt)
    tabulateValues(hv2, tableFmt)

    fig = plt.figure(figsize=(6, 2))
    ax = plt.subplot(1, 1, 1)
    plt.grid()
    plt.rc('text', usetex=True)
    plt.ylim([0, 2])
    plt.xlabel(r'Feature')
    plt.ylabel(r'Weight')
    if title:
        plt.title(title)

    plt.xticks(range(1, len(hv1) + 1), [ h for h, v in hv1 ], rotation=35)

    plt.plot([ v for h, v in hv1 ], 'k-')
    plt.plot([ v for h, v in hv2 ], 'k--')

    plt.setp(ax.get_xticklabels(), rotation=-35, horizontalalignment='right')
    if output:
        print('Output', output)
        plt.savefig(output, bbox_inches='tight')
    else:
        plt.show()

main.command()
