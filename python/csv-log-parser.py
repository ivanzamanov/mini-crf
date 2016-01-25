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
    maxFeatures = len(headers) - 1

    print "Plotting"
    #plt.plot(targetValues)

    nextFeatureIndex = 0
    nextX = 0
    currentPlot = None
    for line in lines:
        if line.startswith('#'):
            if currentPlot:
                currentPlot.plot(x, y)
                #break
            x = []
            y = []
            feature = line[1:]
            currentPlot = plt.subplot(maxFeatures, 1, nextFeatureIndex + 1)
            currentPlot.set_title( feature )
            nextX = 0
            nextFeatureIndex = (nextFeatureIndex + 1) % maxFeatures
        else:
            value = float(getValues(line)[-1])
            #print value, nextX
            nextX += 1
            x.append(nextX)
            y.append(value)
    plt.show();

main.command()
