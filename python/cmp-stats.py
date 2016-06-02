from opster import command
import csv

@command()
def main(inputFile):
    with open(inputFile) as f:
        reader = csv.reader(f, delimiter = '\t')
        headers = reader.__next__()
        print(headers)
        for row in map(lambda x: map(float, x), reader):
            print([ x for x in row][1:])

main.command()
