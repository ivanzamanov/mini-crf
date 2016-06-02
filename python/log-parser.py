from math import *
from opster import command

import matplotlib.pyplot as plt



@command()
def main(file):
    f = open(file)
    val = false
    for line in f.readlines():
        if val:
            parseValue(line)
        else:
            parseCoef(line)

def parseValue(line):
    

main.command()
