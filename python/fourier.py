from math import *
from opster import command

import matplotlib.pyplot as plt

def frange(start, end, step):
    return [start + x * step for x in range(0, int((end - start) / step))]

def myF(x):
    #return sin ( 2 * pi * x)
    return cos(2 * pi * x) + 0.3
    #return sin(x * 2 * pi)

def FF(func, freq, T, step=1.0):
    (sT, eT) = T
    real = 0
    img = 0
    for t in frange(sT, eT, step):
        val = func(t)
        real = real + val * cos(2 * pi * t * freq) * step
        img = img - val * sin(2 * pi * t * freq) * step

    return (real, img)
    #return func() * sqrt(real ** 2 + img ** 2)

def rFF(func, time, f, fStep, T, tStep):
    (fMin, fMax) = f
    result = 0
    for f in frange(fMin, fMax, fStep):
        (real, img) = FF(func, f, T, tStep)
        result = result + real * cos(2 * pi * time * f) + img * sin (2 * pi * time * f)

    return result

@command()
def main():
    plot = plt.subplot(111)

    step = 0.001
    x = [x for x in frange(-pi / 2, pi / 2, step)]
    y = [FF(myF, t, (-1/2, 1/2), step) for t in x]

    plot.plot(x, [a / 10 for (a,b) in y], 'b')
    plot.plot(x, [b for (a,b) in y], 'r')

    x = [x for x in frange(-1 / 2, 1 / 2, step)]
    y = [rFF(myF, xVal, (-1, 1), 0.1, (-1/2, 1/2), step) for xVal in x]

    plot.plot(x, y, 'g')
    
    plt.show()

main.command()
