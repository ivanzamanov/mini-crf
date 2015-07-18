form Feature Extraction
  comment I/O file paths
  sentence soundPath /home/ivo/SpeechSynthesis/corpus-synth/Diana_E.1.Un1/Diana_E.1.Un1_015.wav
  sentence textGridPath /home/ivo/SpeechSynthesis/corpus-synth/Diana_E.1.Un1/Diana_E_1_Un1_015.TextGrid
  sentence outputFile /tmp/praat-output.txt
endform

writeInfo: ""
#writeInfoLine: "Parameters:"
#appendInfoLine: "Sound file: ", soundPath$
#appendInfoLine: "TextGrid file: ", textGridPath$
#appendInfoLine: "Output file: ", outputFile$
#appendInfoLine: "Time step: ", timeStep
#appendInfoLine: "MFCC count: ", mfccCount
#appendInfoLine: ""

timeStep = 0.005
minLength = 2 * timeStep
mfccCount = 12

semiphons = 1

textGridObj = Read from file... 'textGridPath$'
Rename... TextGrid

intervalCount = Get number of intervals... 1

if semiphons
  phonemesCount = intervalCount * 2
else
  phonemesCount = intervalCount
endif

soundObj = Read from file... 'soundPath$'
selectObject: soundObj
Rename... Sound

# Extract pitch tier
pitch = To Pitch (ac)... timeStep 75 20 1 0 0 0.01 0.35 0.14 600
# Extract pulses to be used to determine cut points
pointProcess = To PointProcess

# generate MFCC
soundObjOriginal = soundObj
selectObject: soundObj
soundObj = Filter (pre-emphasis)... 50
totalDuration = Get total duration
windowLength = 2 * timeStep
mfccObj = To MFCC... mfccCount windowLength timeStep 100 100 0.0

# Available objects:
# pointProcess, soundObj, pitch

textGridSegmented = Create TextGrid... 0 totalDuration phonemes ""
# First off, extract all boundary points...
for i to intervalCount
  if semiphons
    intervalIndex = (i-1) * 2 + 1
  else
    intervalIndex = i
  endif

  selectObject: textGridObj
  intervalLabel$ = Get label of interval... 1 i
  # Label of left semiphon
  labels$[intervalIndex] = "-" + intervalLabel$
  # right semiphon
  labels$[intervalIndex + 1] = intervalLabel$ + "-"

  # Left and right boundaries of the current interval in the TextGrid
  intervalLeft = Get start point... 1 i
  intervalRight = Get end point... 1 i

  # Find point of maximum energy between startPoint and endPoint
  # This is where the phoneme should be split
  selectObject: soundObjOriginal
  startPoint = intervalLeft
  endPoint = intervalRight
  @findMaxEnergyPoint

  if semiphons
    selectObject: textGridObj

    semiPhonStart[intervalIndex] = startPoint
    semiPhonEnd[intervalIndex] = maxEnergyPoint

    semiPhonStart[intervalIndex + 1] = maxEnergyPoint
    semiPhonEnd[intervalIndex + 1] = endPoint
  else
    selectObject: textGridObj
    semiPhonStart[intervalIndex] = startPoint
    semiPhonEnd[intervalIndex] = endPoint
  endif
endfor

appendInfoLine: "intervals = ", intervalCount

# By now, I have the boundaries as defined by the text grid and maximum energy
# Now, round to a glottal pulse
if semiphons
  semiPhonCount = intervalCount * 2
else
  semiPhonCount = intervalCount
endif

appendInfoLine: "semiPhonCount = ", semiPhonCount
minStartIndex = 0
for i to semiPhonCount
  startPoint_ = semiPhonStart[i]
  endPoint_ = semiPhonEnd[i]

  @getPulseBoundaries

  semiPhonStart[i] = startPulsePoint
  semiPhonEnd[i] = endPulsePoint
  
  if i == semiPhonCount
    semiPhonEnd[i] = Get total duration
    semiPhonStart[i] = semiPhonEnd[i - 1]
  endif
  
  if startPulsePoint == endPulsePoint
    appendInfo: "Pulse Skip: ", labels$[i], " ", startPulsePoint, " ", endPulsePoint
    appendInfoLine: " = ", semiPhonStart[i], " ", semiPhonEnd[i]
  endif

  if (i != 1) && startPulsePoint != semiPhonEnd[i - 1]
    appendInfo: "Point Skip: ", i, " ", labels$[i], " ", startPulsePoint, " ", endPulsePoint
    appendInfoLine: " = ", semiPhonStart[i], " ", semiPhonEnd[i - 1]
  endif
endfor

for i to semiPhonCount
  selectObject: textGridSegmented
  if i != 0
    #appendInfoLine: "Insert ", i
    #Insert boundary... 1 semiPhonStart[i]
  endif
  selectObject: mfccObj
  #appendInfoLine: semiPhonStart[i], " ", semiPhonEnd[i]
  startFrames[i] = Get frame number from time... semiPhonStart[i]
  endFrames[i] = Get frame number from time... semiPhonEnd[i]

  startFrames[i] = floor(startFrames[i])
  endFrames[i] = floor(endFrames[i])
endfor

for i to semiPhonCount
  if i > 1
     startFrames[i] = endFrames[i-1]
  endif

  #appendInfoLine: i, ": ", startFrames[i], " ", endFrames[i]
  if startFrames[i] >= endFrames[i]
     endFrames[i] = startFrames[i] + 1
  endif

  #semiPhonStart[i] = Get time from frame number... startFrames[i]
  #semiPhonEnd[i] = Get time from frame number... endFrames[i]
endfor

for i to semiPhonCount - 1
  if startFrames[i + 1] != endFrames[i]
    appendInfo: "Diff at ", i, " ", i + 1, " "
    appendInfoLine: startFrames[i + 1], " ", endFrames[i]
    appendInfoLine: semiPhonEnd[i], " ", semiPhonStart[i + 1]
    #exitScript: "Fail"
  endif
endfor

deleteFile: outputFile$
appendFileLine: outputFile$, "[Config]"
appendFileLine: outputFile$, "timeStep=", timeStep
appendFileLine: outputFile$, "mfcc=", mfccCount
appendFileLine: outputFile$, "intervals=", phonemesCount

# Output all semiphons
for i to semiPhonCount
  startPoint_ = semiPhonStart[i]
  endPoint_ = semiPhonEnd[i]
  label_$ = labels$[i]
  duration = endPoint_ - startPoint_
  startFrame_ = startFrames[i]
  endFrame_ = endFrames[i]

  #appendInfoLine: startPoint_, " ", endPoint_, " ", label_$, " ", duration, " ", startFrame_, " ", endFrame_

  # These should 1) be very little, just some edge cases
  # and 2) be irrelevant anyway
  if ! startPoint_ >= endPoint_
    @outputEntry
  else
    appendInfoLine: i, ": ", startPoint_, " ", endPoint_
    exitScript: "Fail"
  endif
endfor

# Extract pulses to be used to determine cut points
# Ok, sooo I need to know which parts are voiced/unvoiced, hence I can't use the
# 'forced' version above
selectObject: soundObj
pitch = To Pitch... timeStep 75 600
pointProcess = To PointProcess
pointsCount = Get number of points
appendFileLine: outputFile$, "pulses=", pointsCount
for i to pointsCount
  value = Get time from index... i
  appendFileLine: outputFile$, "p=", value
endfor
value = Get total duration
appendFileLine: outputFile$, "p=", value

# And clean up a bit
selectObject: mfccObj
#Remove
selectObject: pointProcess
#Remove
selectObject: soundObj
Remove
selectObject: pitch
Remove

appendInfoLine: "Done"

# ---------------------
# ----- Functions -----
# ---------------------

# Input: startPoint_, endPoint_, startFrame_, endFrame_, duration, label_$
# Objects: pitch, mfccObj
# Output:
procedure outputEntry
  appendFileLine: outputFile$, "[Entry]"
  appendFileLine: outputFile$, "label=", label_$
  appendFileLine: outputFile$, "start=", startPoint_
  appendFileLine: outputFile$, "end=", endPoint_
  #appendFileLine: outputFile$, "frames=", (endFrame - startFrame + 1)
  appendFileLine: outputFile$, "frames=", 2
  appendFileLine: outputFile$, "duration=", duration

  featureFrames[1] = startFrame_
  featureFrames[2] = endFrame_

  for k to 2
    frame = featureFrames[k]
    selectObject: mfccObj
    if frame < 1
      frame = 1
    endif

    pitchPoint = Get time from frame... frame
    selectObject: pitch
    pitchValue = Get value at time: pitchPoint, "Hertz", "Linear"
    appendFileLine: outputFile$, "pitch=", pitchValue

    selectObject: mfccObj
    appendFile: outputFile$, "mfcc="

    value = Get value in frame: frame, 1
    appendFile: outputFile$, value
    for j from 2 to mfccCount
      value = Get value in frame: frame, j
      appendFile: outputFile$, " ", value
    endfor
    appendFileLine: outputFile$
  endfor
endproc

include functions.praat
