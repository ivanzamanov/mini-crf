import matplotlib.pyplot as plt
import numpy as np
import itertools
import math

fig = plt.figure(figsize=(10, 2))
ax = plt.subplot(1, 1, 1)
plt.grid()
plt.rc('text', usetex=True)
plt.ylim([0, 1.1])
plt.xlabel(r'Frequency (Hz)')
#plt.xscale('log')

BARK_BAND_EDGES = [0, 100, 200, 300, 400, 510, 630, 770, 920, 1080, 1270, 1480, 1720, 2000, 2320, 2700, 3150, 3700, 4400, 5300, 6400, 7700, 9500, 12000, 15500]
BARK_CENTERS = [50, 150, 250, 350, 450, 570, 700, 840, 1000, 1170, 1370, 1600, 1850, 2150, 2500, 2900, 3400, 4000, 4800, 5800, 7000, 8500, 10500, 13500]

plt.xticks(BARK_BAND_EDGES)
plt.setp(ax.get_xticklabels(), rotation=35, horizontalalignment='right')

def invert(period):
    return 1 / period
def welch(n, N):
    N1 = (N - 1) / 2
    return 1 - ( (n - N1) / N1 ) ** 2

def plotAroundFilter(bot, top):
    botF = bot
    topF = top
    N = topF - botF
    print(botF, topF)
    for x in np.arange(botF, topF + 1, (topF - botF) / 100):
        y = math.sin(
            (x - botF) / N * math.pi
        )
        yield x, y

def plotFilters():
    offset = len(BARK_BAND_EDGES) - 10 - 1
    size = 10
    for bot, top in zip(BARK_BAND_EDGES[offset:offset + size], BARK_BAND_EDGES[offset + 1:offset + size + 1]):
        yield plotAroundFilter(bot, top)

def plotFilter(vals):
    vals = [(x, y) for x, y in vals]
    plt.plot([ x for x,y in vals], [y for x,y in vals], color='black')

filters = [points for points in plotFilters()]
#plotFilter( filters[0])
for vals in filters: plotFilter(vals)
plt.show()
#plt.savefig('auditory.png', bbox_inches='tight')
