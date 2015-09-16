from math import *
from opster import command

from functools import lru_cache
import matplotlib.pyplot as plt

def frange(start, end, step):
    return [start + x * step for x in range(0, int((end - start) / step))]

def myF(x):
    #return sin ( 2 * pi * x)
    return cos(x * 2 * pi)
    #return sin(x * 2 * pi)

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
        return result / (2 * pi)

    return result

@command()
def main():
    freqStep = 0.01
    freqRange = (-5, 5)
    timeRange = (-1/2, 1/2)
    timeStep = 0.01

    ft = FT(myF, timeRange, timeStep)
    rft = rFT(ft, freqRange, freqStep)

    plot = plt.subplot(211)
    x = [x for x in frange(freqRange[0], freqRange[1], freqStep)]
    y = [ft(t) for t in x]

    plot.plot(x, [a for (a,b) in y], 'b')
    plot.plot(x, [b for (a,b) in y], 'r')

    print("Computed FT")

    x = [x for x in frange(timeRange[0], timeRange[1], timeStep)]
    y = [rft(t) for t in x]

    plot = plt.subplot(212)
    plot.plot(x, y, 'g')
    
    plt.show()

main.command()
