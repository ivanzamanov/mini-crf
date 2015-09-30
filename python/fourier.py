from math import *
from opster import command

import matplotlib.pyplot as plt

def frange(start, end, step):
    start += 0.0
    step += 0.0
    end += 0.0
    return [start + x * step for x in range(0, int((end - start) / step) + 1)]

def myF(t):
    #return 5 + 2 * cos(2 * pi * t - pi / 2) + 3 * cos(4 * pi * t)
    return cos(2 * t * 2)

def FT(values):
    result = [0 for v in range(0, len(values) // 2 + 1)]
    period = float(len(values))
    for f, _ in enumerate(result):
        real = 0
        img = 0
        for t, val in enumerate(values):
            t = float(t)
            real += val * cos(2 * pi * t * float(f) / period)
            img  -= val * sin(2 * pi * t * float(f) / period)

        result[f] = (real / period, img / period)
    return result

def rFT(frequencies, period, resultLength):
    result = [0 for f in range(0, resultLength)]
    for t, _ in enumerate(result):
        value = 0
        for f, (real, img) in enumerate(frequencies):
            real = real * cos(2 * pi * t * f / period)
            img = img * sin (2 * pi * t * f / period)
            value += real - img
        result[t] = value
    return result

def subplot(i, I):
    plot = plt.subplot(int(str(I) + '1' + str(i)))
    plot.axhline(0, color='black')
    plot.axvline(0, color='black')
    return plot

def formatC(c):
  return "({:.3f}, {:.3f})".format(c[0], c[1])

@command()
def main():
    PLOTS = 4
    period = pi
    timeRange = (-period / 2, period / 2)
    timeStep = 0.1

    tVals = [myF(t) for t in frange(timeRange[0], timeRange[1], timeStep)]
    tBins = range(0, len(tVals))
    fVals = FT(tVals)

    plot = subplot(1, PLOTS)
    plot.set_title("Fourier Domain")
    plot.plot(tBins[:len(fVals)], [a for (a,b) in fVals], 'b')
    plot.plot(tBins[:len(fVals)], [b for (a,b) in fVals], 'r')
    #for v in fVals: print(v)

    plot = subplot(2, PLOTS)
    plot.set_title("Fourier Domain Modded")
    scale = 2
    newLen = len(fVals)
    #newLen = int((len(fVals) - 1) * scale + 1)
    modded = [(0,0) for x in range(0, newLen)]
    modded = [x for x in fVals]

    '''for i, (real, img) in enumerate(fVals):
        if(sqrt(real * real + img * img) > 0.001):
            print("Phase " + str(i) + ": " + str( atan2(img, real) ))
    for x, y in zip(fVals[1:], fVals[1:][::-1]):
        print(str(x) + ' == ' + str(y) + ' = ' + str(x[0] == y[0] and x[1] == -y[1]))'''

    '''for i, _ in enumerate(modded):
        ni = i / scale
        nf = ni
        ni = int(ni)
        if ni > len(fVals) - 1:
          continue
        a = 0
        a = ni - nf
        c = modded[i]
        c = (c[0] + (1 - a) * fVals[ni][0], c[1] + (1 - a) * fVals[ni][1])
        str1 = formatC(c)
        str2 = ''
        if ni < len(fVals) - 1:
          plus = (a * fVals[ni + 1][0], a * fVals[ni + 1][1])
          c = (c[0] + plus[0], c[1] + plus[1])
          str2 = formatC(plus)
        #c = fVals[ni]
        modded[i] = c
        #print(str(i) + ' -> ' + str(ni) + ' , a = ' + str(a))'''

    '''print
    for i, c in enumerate(fVals): print(str(i) + ': ' + formatC(c))
    print
    for i, c in enumerate(modded): print(str(i) + ': ' + formatC(c))'''

    mTBins = range(0, len(modded))
    plot.plot(mTBins, [a for (a,_) in modded], 'b')
    plot.plot(mTBins, [b for (_,b) in modded], 'r')

    plot = subplot(3, PLOTS)
    plot.set_title("Time Domain Inversed")
    iVals = rFT(modded, int(len(tVals) / scale), len(tVals))
    tBins = range(0, len(iVals))
    plot.plot(tBins, iVals, 'g')

    plot = subplot(4, PLOTS)
    plot.set_title("Time Domain Original")
    tBins = range(0, len(tVals))
    plot.plot(tBins, tVals, 'b')
    
    plt.show()

main.command()
