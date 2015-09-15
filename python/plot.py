import matplotlib.pyplot as plt
from math import *


data = []

size = 100
def hann(i, size):
    return 0.5 * (1 - cos(2 * pi * i / size))

def transform(data, size):
    size = size * 2
    for i in range(0, size / 2):
        data.append(hann(i, size))

def transform2(data, size):
    size = size * 2
    for i in range(0, size / 2):
        i += size/2
        data.append(hann(i, size))

plot = plt.subplot(111)
transform(data, size)
transform2(data, size)
for i, v in enumerate(data):
    plot.scatter(i, v, color='blue' if i >= len(data)/2 else 'red')

plt.show()
