from math import *
from itertools import *
import matplotlib.pyplot as plt

PERIOD = 200
x1 = [ sin(2 * pi * 10 * t / PERIOD) for t in range(0, PERIOD) ]

x2 = [ sin(2 * pi * 10 * t / PERIOD) for t in range(0, PERIOD) ]

crossfade = 30

mid = PERIOD

def hann(n, N):
    return 0.5 * (1 - cos(2 * pi * n / (N - 1)) )

def crossfaded(x1, x2, crossfadeTime):
    for t in range(0, crossfadeTime):
        v1 = x1[len(x1) - crossfadeTime + t]
        v2 = x2[t]
        yield v1 * hann(t + crossfadeTime, crossfadeTime * 2) + v2 * hann(t, crossfadeTime * 2)

total = [ v for v in chain( x1[0:PERIOD - crossfade / 2], crossfaded(x1, x2, crossfade), x2[crossfade:PERIOD - crossfade/2])]

print len(total)

plt.plot(total)

plt.vlines(PERIOD, -1, 1, color="r")
plt.vlines(PERIOD - crossfade / 2, -1, 1, color="r")
plt.vlines(PERIOD + crossfade / 2, -1, 1, color="r")

plt.show()
