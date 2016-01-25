from math import *
from opster import command

import matplotlib.pyplot as plt

def readLines(path):
    f = open(path)
    for line in f.readlines():
        line = line.rstrip('\r\n')
        yield line
    f.close()

def nextValues(lines): getValues(next(lines))
def getValues(line):  return line.split(' ')

@command()
def main(logFilePath):
    lines = readLines(logFilePath)
    headers = getValues(next(lines))
    allValues = map( lambda x: map(float, x), map(getValues, lines))

    print "Plotting"
    targetValues = [ values[len(values) - 1] for values in allValues ]
    plt.plot(targetValues)
    plt.axhline( min(targetValues), color='r')
    for i, (prev, n) in enumerate(zip(allValues, allValues[1:])):
        if i % 1000 == 0:
            plt.axvline(i, color='r')
    plt.show();

main.command()
