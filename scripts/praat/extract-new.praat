form Feature Extraction
  comment I/O file paths
  sentence soundPath _
  sentence textGridPath /home/ivo/SpeechSynthesis/corpus-test/Diana_A.1.1.Un2/Diana_A_1_1_Un2_0013.T
  sentence laryngographFile _
endform

timeStep = 0.012

mfccCount = 12
windowLength = 0.04

pitchFloor = 75
maxCandidates = 15
silenceThreshold = 0.03
voicingThreshold = 0.45
octaveCost = 0.02
octaveJumpCost = 0.5
voicedUnvoicedCost = 0.14
pitchCeiling = 600

sound = Read from file... 'soundPath$'
laryngograph = Read from file... 'laryngographFile$'
textGrid = Read from file... 'textGridPath$'

selectObject: laryngograph
larynDuration = Get total duration
pitch = To Pitch (ac)... timeStep pitchFloor maxCandidates 1 silenceThreshold voicingThreshold octaveCost octaveJumpCost voicedUnvoicedCost pitchCeiling
#pitch = To Pitch... timeStep 75 600
pointProcess = To PointProcess

selectObject: sound
soundDuration = Get total duration
mfcc = To MFCC... mfccCount windowLength timeStep 100 100 0.0

appendInfoLine: "file=", soundPath$
# Pitch
appendInfoLine: "[Pitch]"
selectObject: pitch
pitchFrameCount = Get number of frames
for frame from 1 to pitchFrameCount
    value = Get value in frame... frame Hertz
    appendInfoLine: value
endfor

# Pitch Pulses
appendInfoLine: "[Pulses]"
selectObject: pointProcess
pointCount = Get number of points
for i to pointCount
    point = Get time from index... i
    appendInfoLine: point
endfor
appendInfoLine: soundDuration

# MFCC
appendInfoLine: "[MFCC]"
selectObject: mfcc
mfccFrameCount = Get number of frames
for frame from 1 to mfccFrameCount
    value = Get value in frame... frame 1
    for i from 2 to mfccCount
        value = Get value in frame... frame i
        appendInfo: " ", value
    endfor
    appendInfoLine: ""
endfor

# Text Grid
appendInfoLine: "[TextGrid]"
selectObject: textGrid
intervals = Get number of intervals... 1
for interval to intervals
    start = Get starting point... 1 interval
    end = Get end point... 1 interval
    label$ = Get label of interval... 1 interval

    appendInfoLine: start, " ", end, " ", label$
endfor

#appendInfoLine: "Sound duration ", soundDuration
#appendInfoLine: "Laryngograph duration ", larynDuration
#appendInfoLine: "Pitch frames ", pitchFrameCount
#appendInfoLine: "Mfcc frames ", mfccFrameCount
