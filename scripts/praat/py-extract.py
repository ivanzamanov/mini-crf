#!/usr/bin/env python
import sys, re, wave, struct
from math import *

# --- Utilities ---
def writeln(line):
    write(line + '\n')
def write(line):
    global outputStream
    outputStream.write(line)

# --- Data ---
pitchValues = []
def handlePitch(line):
    if line == '--undefined--':
        pitchValues.append(0)
    else:
        pitchValues.append(float(line))

pulses = []
def handlePulses(line):
    pulses.append(float(line))

mfcc = []
def handleMfcc(line):
    mfcc.append(line.split(' '))

intervals = []
def handleTextGrid(line):
    vals = line.split(' ')
    start = float(vals[0])
    end = float(vals[1])
    label = vals[2]
    intervals.append((start, end, label))

def coalesceSilenceAndNoise(intervals):
    result = []
    seq = []
    for start, end, label in intervals:
        if not label in ['$', '_']:
            if seq:
                seqStart, seqEnd, seqLabel = seq[0][0], seq[-1][1], '$'
                if verbose and len(seq) > 1:
                    print 'Coalesced', start, 'to', end, 'from', len(seq), 'intervals'
                result.append((seqStart, seqEnd, seqLabel))
                seq = []

            result.append((start, end, label))
        else:
            seq.append((start, end, label))
    if seq:
        start, end, label = seq[0][0], seq[-1][1], '$'
        if verbose and len(seq) > 1:
            print 'Coalesced', start, 'to', end, 'from', len(seq), 'intervals'
        result.append((start, end, label))
    return result

def valueAtTime(time, lst):
    index = min(len(lst) - 1, int(floor(time / TIME_STEP)))
    #print time, index, 'of', len(lst)
    return lst[index]

UNVOICED = 0.02
def getNearest(time, pulses):
    result = -1
    for p, n in zip(pulses, pulses[1:]):
        if p <= time and n > time:
            result = p
            break
    if abs(time - p) < UNVOICED:
        result = p
    else:
        result = time
    return result

def roundToPulse(intervals, pulses):
    for interval in intervals:
        nearestStart = getNearest(interval.start, pulses)
        nearestEnd = getNearest(interval.end, pulses)
        if verbose: print 'Round [', interval.start, interval.end, ']', 'to', '[', nearestStart, nearestEnd, ']', (interval.start - nearestStart, interval.end - nearestEnd)

        assert nearestStart < nearestEnd

        interval.start = nearestStart
        interval.end = nearestEnd
    return intervals

def generatePhonemes(wave):
    global intervals
    intervals = coalesceSilenceAndNoise(intervals)
    intervals = map(lambda i: Interval(i), intervals)
    intervals = roundToPulse(intervals, pulses)

    phonemes = map(lambda i: Phoneme(start=i.start,end=i.end,label=i.label), intervals)
    map(lambda p: p.populate(wave, mfcc, pulses), phonemes)
    return phonemes

class Interval:
    def __init__(self, interval):
        self.start = interval[0]
        self.end = interval[1]
        self.label = interval[2]

class Phoneme:
    ''' A PhonemeInstance '''
    def __init__(self, start=-1, end=-1, label='%'):
        self.label = label
        self.start = start
        self.end = end
        self.frames = 2
        self.duration = end - start
        self.energy = 0
        self.mfcc = []
        self.pitch = []

    def populate(self, wave, mfcc, pulses):
        sampleRate = wave.getframerate()
        wave.setpos(self.start * sampleRate)
        samples = [ struct.unpack_from("<h", wave.readframes(1))[0]
                    for i in range(0, int(self.duration * sampleRate))]

        self.energy = reduce(lambda x, y: x + y ** 2, samples)

        self.mfcc = [ valueAtTime(self.start, mfcc),
                      valueAtTime(self.end, mfcc) ]
        self.pitch = [ valueAtTime(self.start, pitchValues),
                       valueAtTime(self.end, pitchValues) ]

    def output(self):
        assert self.label != '%'
        assert self.end > self.start
        assert self.duration > 0
        assert self.energy > 0
        assert len(self.mfcc) == self.frames
        assert len(self.pitch) == self.frames

        writeln('[Entry]')
        writeln('label=' + self.label)
        writeln('start=' + str(self.start))
        writeln('end=' + str(self.end))
        writeln('frames=' + str(self.frames))
        writeln('duration=' + str(self.duration))
        writeln('energy=' + str(self.energy))
        for i in range(0, self.frames):
            mfcc = self.mfcc[i]
            writeln('pitch=' + str(self.pitch[i]))
            write('mfcc=')
            writeln(' '.join(map(str, mfcc)))

# --- Args ---
TIME_STEP = 0.01
args = sys.argv[1:]
verbose = '-v' in args
if verbose:
    args.remove('-v')

if len(args) < 2:
    raise Exception('Not enough arguments, 2 needed')

if args[0] == '-':
    inputStream = sys.stdin
else:
    inputStream = args[0]
if args[1] == '-':
    outputStream = sys.stdout
else:
    outputStream = file(args[1], 'w')

# --- Work ---
SECTION = re.compile('\[.*\]')
state = '[None]'
wavFile = None
for line in map(lambda line: line.strip('\n\r'), inputStream.readlines()):
    if 'file=' in line:
        wavFile = line[len('file='):]
    elif re.match(SECTION, line):
        state = line
        if verbose: print 'Begin parsing', state
    else:
        if state == '[Pitch]':
            handlePitch(line)
        elif state == '[Pulses]':
            handlePulses(line)
        elif state == '[MFCC]':
            handleMfcc(line)
        elif state == '[TextGrid]':
            handleTextGrid(line)
        else:
            raise Exception('Unknown state', str(state))

inputStream.close()

assert len(pitchValues) == len(mfcc)

if verbose: print 'Pitch Values', len(pitchValues)
if verbose: print 'Mfcc', len(mfcc)
if verbose: print 'Pulses', len(pulses)
if verbose: print 'Intervals', len(intervals)

writeln('[Config]')
writeln('timeStep=0.01')
writeln('mfcc=12')

phonemes = generatePhonemes(wave.open(wavFile))
writeln('intervals=' + str(len(phonemes)))

map(lambda phon: phon.output(), phonemes)

writeln('pulses=' + str(len(pulses)))
map(lambda pulse: writeln('p=' + str(pulse)), pulses)

outputStream.close()
