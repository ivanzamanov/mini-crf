from math import *
from opster import command

from functools import lru_cache
import matplotlib.pyplot as plt

def frange(start, end, step):
    start += 0.0
    step += 0.0
    end += 0.0
    return [start + x * step for x in range(0, int((end - start) / step) + 1)]

def myF(t):
    #return 5 + 2 * cos(2 * pi * t - pi / 2) + 3 * cos(4 * pi * t)
    return cos(2 * t)

def FT(values):
    result = [0 for v in values]
    period = float(len(values))
    for f, _ in enumerate(result):
        real = 0
        img = 0
        for t, val in enumerate(values):
            t = float(t)
            real += val * cos(2 * pi * t * float(f) / period)
            img  -= val * sin(2 * pi * t * float(f) / period)

        result[f] = (real / period, img / period)
        #print(result[f])
    return result

def rFT(frequencies):
    result = [0 for f in frequencies]
    period = float(len(frequencies))
    for t, _ in enumerate(result):
        value = 0
        for f, (real, img) in enumerate(frequencies):
            real = real * cos(2 * pi * t * f / period)
            img = img * sin (2 * pi * t * f / period)
            value += real - img
        result[t] = value
    return result

@command()
def main():
    period = pi
    timeRange = (-period / 2, period / 2)
    timeStep = 0.2

    tVals = [myF(t) for t in frange(timeRange[0], timeRange[1], timeStep)]
    tBins = range(0, len(tVals))
    fVals = FT(tVals)

    plot = plt.subplot(411)
    plot.set_title("Fourier Domain")
    plot.plot(tBins, [a for (a,b) in fVals], 'b')
    plot.plot(tBins, [b for (a,b) in fVals], 'r')
    #for v in fVals: print(v)

    plot = plt.subplot(412)
    plot.set_title("Fourier Domain Modded")
    modded = [0 for x in fVals]
    scale = 2
    for i, _ in enumerate(fVals):
        ni = int(i / scale) % len(fVals)
        modded[ni] = fVals[i];
        print(str(i) + ' -> ' + str(ni))

    plot.plot(tBins, [a for (a,b) in modded], 'b')
    plot.plot(tBins, [b for (a,b) in modded], 'r')

    plot = plt.subplot(413)
    plot.set_title("Time Domain Inversed")
    iVals = rFT(modded)
    plot.plot(tBins, iVals, 'g')

    plot = plt.subplot(414)
    plot.set_title("Time Domain Original")
    plot.plot(tBins, tVals, 'b')

    plt.show()

'''def FT(func, T, step=1.0):
    (sT, eT) = T
    @lru_cache()
    def result(freq):
        real = 0
        img = 0
        for t in frange(sT, eT, step):
            val = func(t)
            real = real + val * cos(2 * pi * t * freq) * step
            img = img - val * sin(2 * pi * t * freq) * step
        return (real, img)

    return result'''

'''def rFT(func, period, F, step=1.0):
    (fMin, fMax) = F
    @lru_cache()
    def result(time):
        result = 0
        for f in frange(fMin, fMax, step):
            (real, img) = func(f)
            real = real * cos(2 * pi * time * f)
            img = img * sin (2 * pi * time * f)
            result = result + real - img
        return result / period
    return result'''

'''@command()
def main():
    period = pi
    timeRange = (-period / 2, period / 2)
    timeStep = 0.01

    freqStep = 1 / period
    maxFreqComponents = 20
    freqRange = (- (maxFreqComponents / 2) / period, (maxFreqComponents / 2) / period)

    ft = FT(myF, timeRange, timeStep)
    rft = rFT(ft, period, freqRange, freqStep)

    tdVal = [myF(t) for t in frange(timeRange[0], timeRange[1], timeStep)]
    fdVal = [ft(f) for f in frange(freqRange[0], freqRange[1], freqStep)]

    plot = plt.subplot(311)
    plot.set_title("Fourier Domain")
    x = [x for x in frange(freqRange[0], freqRange[1], freqStep)]
    y = fdVal
    plot.plot(x, [a for (a,b) in y], 'b')
    plot.plot(x, [b for (a,b) in y], 'r')
    for v in fdVal: print(v)

    x = [x for x in frange(timeRange[0], timeRange[1], timeStep)]
    y = [rft(t) for t in x]

    plot = plt.subplot(312)
    plot.set_title("Time Domain Inversed")
    plot.plot(x, y, 'g')

    plot = plt.subplot(313)
    plot.set_title("Time Domain Original")
    plot.plot(x, tdVal, 'b')

    plt.show()'''

main.command()
