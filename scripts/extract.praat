form Feature Extraction
  comment I/O file paths
  sentence soundPath /home/ivo/SpeechSynthesis/corpus-test/Diana_A.1.1.Un2/Diana_A.1.1.Un2_0013.wav
  sentence textGridPath /home/ivo/SpeechSynthesis/corpus-test/Diana_A.1.1.Un2/Diana_A_1_1_Un2_0013.TextGrid
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
intervals = Get number of intervals... 1
for i to intervals - 1
  #appendInfoLine: i
  l1$ = Get label of interval... 1 i
  l2$ = Get label of interval... 1 (i+1)
  if l1$ == l2$ && l1$ == "$"
    #appendInfoLine: "Remove of ", (i+1), " ", l1$, l2$
    Remove left boundary... 1 (i+1)
    Set interval text: 1, i, l1$
    i -= 1
    intervals = Get number of intervals... 1
  endif
endfor

intervalCount = Get number of intervals... 1

if semiphons
  phonemesCount = intervalCount * 2
else
  phonemesCount = intervalCount
endif

soundObj = Read from file... 'soundPath$'
selectObject: soundObj
Rename... Sound
const_total_time = Get total duration

# Extract pitch tier
#pitch = To Pitch (ac)... timeStep 75 20 1 0 0 0.01 0.35 0.14 600
pitch = To Pitch (ac)... timeStep 75 20 1 0.03 0.45 0.01 0.35 0.14 600
# Extract pulses to be used to determine cut points
pointProcess = To PointProcess

@forcePointProcess

# generate MFCC
soundObjOriginal = soundObj
selectObject: soundObj
soundObj = Filter (pre-emphasis)... 50
totalDuration = Get total duration
windowLength = 2 * timeStep
mfccObj = To MFCC... mfccCount windowLength timeStep 100 100 0.0

# Available objects:
# pointProcess, soundObj, pitch

textGridSegmented = Create TextGrid... 0 totalDuration "Phonemes EndPoints" ""
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
  #appendInfoLine: "[ ", intervalLeft, " ", intervalRight, " ]"

  selectObject: pointProcess
  startIndex = Get nearest index... intervalLeft
  endIndex = Get nearest index... intervalRight

  if startIndex != 0
    startPulsePoint = Get time from index... startIndex
  else
    startPulsePoint = startIndex
  endif
  endPulsePoint = Get time from index... endIndex

  semiPhonStart[intervalIndex] = startPulsePoint
  semiPhonEnd[intervalIndex] = endPulsePoint

  if semiphons
    #Find point of maximum energy between startPoint and endPoint
    #This is where the phoneme should be split
    selectObject: soundObjOriginal
    startPoint = startPulsePoint
    endPoint = endPulsePoint

    @findMaxEnergyPoint

    semiPhonEnd[intervalIndex] = maxEnergyPoint
    semiPhonStart[intervalIndex + 1] = maxEnergyPoint
    semiPhonEnd[intervalIndex + 1] = endPulsePoint
  endif

  appendInfo: "[ ", intervalLeft, " ", intervalRight, " ] -> "
  appendInfo: "[ ",semiPhonStart[intervalIndex]," ",semiPhonEnd[intervalIndex]," ] "
  appendInfoLine: "[ ",semiPhonStart[intervalIndex + 1]," ",semiPhonEnd[intervalIndex + 1]," ]"
endfor

#appendInfoLine: "intervals = ", intervalCount

# By now, I have the boundaries as defined by the text grid and maximum energy
# Now, round to a glottal pulse
if semiphons
  semiPhonCount = intervalCount * 2
else
  semiPhonCount = intervalCount
endif

# Create a segmentation textgrid
selectObject: textGridSegmented
for i to semiPhonCount
  appendInfoLine: i, "-------"
  my_label$ = labels$[i]
  insertedIntervals = Get number of intervals... 2
  Set interval text: 2, insertedIntervals, my_label$
  appendInfoLine: "End ", i, " ", semiPhonEnd[i]
  Insert boundary: 2, semiPhonEnd[i]
endfor


#appendInfoLine: "semiPhonCount = ", semiPhonCount
minStartIndex = 0
for i to 0
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
  if semiPhonStart[i] > 0
    selectObject: textGridSegmented
    #appendInfoLine: "Insert ", i
    insertedIntervals = Get number of intervals... 1
    Set interval text: 1, insertedIntervals, my_label$
    Insert boundary... 1 semiPhonStart[i]
    my_label$ = labels$[i]
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

@unforcePointProcess
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
#pointProcess = To PointProcess
selectObject: pointProcess
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
    #appendInfoLine: "Pitch at ", pitchPoint, " ", pitchValue
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
