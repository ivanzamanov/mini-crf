import numpy as np, sys
import matplotlib.pyplot as plt
from math import *

F = 2
fig = plt.figure(figsize=(5 * F, 1 * F))
p = fig.add_subplot(1, 1, 1)

p.set_title(r"MFCC Filterbanks")

p.axes.set_ylim([0, 1])
p.axes.set_xlabel(r'Frequency (Hz)')
p.axes.set_ylabel(r'')

def traingular(bot, cent, top, f):
    if f < bot or f > top:
        return 0

    if f <= cent:
        return (f - bot) / (cent - bot)

    return (top - f) / (top - cent)

def mel(f):
    return 2595.0 * log10(1 + f / 700.0)

def rmel(m):
    return 700.0 * (10.0 ** (m / 2595.0) - 1)

def toFilters(mels):
    for bot, mid, top in zip(mels, mels[1:], mels[2:]):
        yield (bot, mid, top)

botF = 0
topF = 8000
p.set_xlim(botF, topF)

botM = mel(botF)
topM = mel(topF)

mels = np.arange(botM, topM + 1, (topM - botM) / 24)

hertz = list(map(rmel, mels))

plt.plot(hertz, [ i % 2 for i, x in enumerate(hertz) ], 'k-')
hertz = hertz[1:len(hertz) - 1]
plt.plot(hertz, [ i % 2 for i, x in enumerate(hertz) ], 'k--')

if len(sys.argv) > 1:
    fig.savefig(sys.argv[1], bbox_inches='tight')
else:
    plt.show()
