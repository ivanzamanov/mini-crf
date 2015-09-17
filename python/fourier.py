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
    return sin(t)

def FT(func, T, step=1.0):
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

    return result

def rFT(func, F, step=1.0):
    (fMin, fMax) = F
    @lru_cache()
    def result(time):
        result = 0
        for f in frange(fMin, fMax, step):
            (real, img) = func(f)
            real = real * cos(2 * pi * time * f)
            img = img * sin (2 * pi * time * f)
            result = result + real - img
        return 1 / (2 * pi) * result
    return result

@command()
def main():
    period = 2 * pi
    timeRange = (-period / 2, period / 2)
    timeStep = 0.01

    freqStep = 1 / period
    maxFreqComponents = 4
    freqRange = (- (maxFreqComponents / 2) / period, (maxFreqComponents / 2) / period)

    ft = FT(myF, timeRange, timeStep)
    rft = rFT(ft, freqRange, freqStep)

    tdVal = [myF(t) for t in frange(timeRange[0], timeRange[1], timeStep)]
    fdVal = [ft(f) for f in frange(freqRange[0], freqRange[1], freqStep)]

    plot = plt.subplot(311)
    plot.set_title("Fourier Domain")
    x = [x for x in frange(freqRange[0], freqRange[1], freqStep)]
    y = fdVal
    plot.plot(x, [a for (a,b) in y], 'b')
    plot.plot(x, [b for (a,b) in y], 'r')

    x = [x for x in frange(timeRange[0], timeRange[1], timeStep)]
    y = [rft(t) for t in x]

    plot = plt.subplot(312)
    plot.set_title("Time Domain Inversed")
    plot.plot(x, y, 'g')

    plot = plt.subplot(313)
    plot.set_title("Time Domain Original")
    plot.plot(x, tdVal, 'b')

    plt.show()

main.command()
