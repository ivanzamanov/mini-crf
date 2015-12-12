form Feature Extraction
  comment I/O file paths
  sentence soundPath /home/ivo/SpeechSynthesis/corpus-test/Diana_A.1.1.Un2/Diana_A.1.1.Un2_0013.wav
  sentence textGridPath /home/ivo/SpeechSynthesis/corpus-test/Diana_A.1.1.Un2/Diana_A_1_1_Un2_0013.TextGrid
  sentence outputFile /tmp/praat-output.txt
  sentence laryngographFile _
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
# Coalesce noise intervals
for i to intervals - 1
  #appendInfoLine: i
  l1$ = Get label of interval... 1 i
  l2$ = Get label of interval... 1 (i+1)
  if (l1$=="$"||l1$=="_") && (l2$=="$"||l2$=="_")
    appendInfoLine: "Remove of ", (i+1), " ", l1$, l2$
    Remove left boundary... 1 (i+1)
    Set interval text: 1, i, l1$
    i -= 1
    intervals = Get number of intervals... 1
  endif
endfor

# Ensure minimum interval length of 0.03
for k to 3
for i from 2 to intervals
  start = Get starting point... 1 i
  end = Get end point... 1 i
  if end - start < 0.03
    newStart = start - (0.03 - end + start)
    if start > 0
      #appendInfoLine: i, " adjust interval, length is ", (end - start)
      l1$ = Get label of interval... 1 (i-1)
      l2$ = Get label of interval... 1 i

      appendInfoLine: "Remove ", start, " insert ", newStart
      Remove left boundary... 1 i
      Insert boundary... 1 newStart
      Set interval text: 1, (i-1), l1$
      Set interval text: 1, i, l2$

      l1$ = Get label of interval... 1 (i-1)
      l2$ = Get label of interval... 1 i
    endif
  endif
endfor
endfor
intervalCount = Get number of intervals... 1

soundObj = Read from file... 'soundPath$'
selectObject: soundObj
Rename... Sound
const_total_time = Get total duration

# Extract pitch tier
#pitch = To Pitch (ac)... timeStep 75 20 1 0 0 0.01 0.35 0.14 600
# Autocorrelation parameters

# Defaults are:
# pitchFloor = 75
# maxCandidates = 15 # 15
# silenceThreshold = 0.03 # 0.03
# voicingThreshold = 0.45 # 0.45
# octaveCost = 0.02 # 0.01
# octaveJumpCost = 0.5 # 0.35
# voicedUnvoicedCost = 0.14 # 0.14
# pitchCeiling = 600 # 600

pitchFloor = 75
maxCandidates = 15
silenceThreshold = 0.03
voicingThreshold = 0.45
octaveCost = 0.02
octaveJumpCost = 0.5
voicedUnvoicedCost = 0.14
pitchCeiling = 600

if laryngographFile$ == "_"
   pitch = To Pitch (ac)... timeStep pitchFloor maxCandidates 1 silenceThreshold voicingThreshold octaveCost octaveJumpCost voicedUnvoicedCost pitchCeiling
else
  laryngographSignal = Read from file... 'laryngographFile$'
  pitch = To Pitch (ac)... timeStep pitchFloor maxCandidates 1 silenceThreshold voicingThreshold octaveCost octaveJumpCost voicedUnvoicedCost pitchCeiling
endif

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
phonCount = 0
for i to intervalCount

  phonCount += 1

  selectObject: textGridObj
  intervalLabel$ = Get label of interval... 1 i
  appendInfoLine: "Label ", i, " = ", intervalLabel$
  # Left and right boundaries of the current interval in the TextGrid
  intervalLeft = Get start point... 1 i
  intervalRight = Get end point... 1 i

  selectObject: pointProcess
  startIndex = Get nearest index... intervalLeft
  endIndex = Get nearest index... intervalRight

  #appendInfoLine: "[ ", intervalLeft, " ", intervalRight, " ]"
  if startIndex != 0
    startPulsePoint = Get time from index... startIndex
  else
    startPulsePoint = startIndex
  endif
  endPulsePoint = Get time from index... endIndex

  semiPhonStart[phonCount] = startPulsePoint
  semiPhonEnd[phonCount] = endPulsePoint

  if semiphons && !(intervalLabel$ == "_" || intervalLabel$ == "$")
    appendInfoLine: "Split ", intervalLabel$
    # Label of left semiphon
    labels$[phonCount] = "-" + intervalLabel$
    # right semiphon
    labels$[phonCount + 1] = intervalLabel$ + "-"

    #Find point of maximum energy between startPoint and endPoint
    #This is where the phoneme should be split
    selectObject: soundObjOriginal
    startPoint = startPulsePoint
    endPoint = endPulsePoint

    @findMaxEnergyPoint

    if maxEnergyPoint != 0
      semiPhonEnd[phonCount] = maxEnergyPoint
      semiPhonStart[phonCount + 1] = maxEnergyPoint
      semiPhonEnd[phonCount + 1] = endPulsePoint
      phonCount += 1
    else if intervalLabel$ != "_" && intervalLabel$ != "$"
      labels$[phonCount] = intervalLabel$ + "-"
      #exitScript: "Error at ", i
    else
      exitScript: "Error at ", i
    endif
  else
    labels$[phonCount] = intervalLabel$
  endif

  #appendInfo: "[ ", intervalLeft, " ", intervalRight, " ] -> "
  #appendInfo: "[ ",semiPhonStart[intervalIndex]," ",semiPhonEnd[intervalIndex]," ] "
  #appendInfoLine: "[ ",semiPhonStart[intervalIndex + 1]," ",semiPhonEnd[intervalIndex + 1]," ]"
endfor

#appendInfoLine: "intervals = ", intervalCount

# By now, I have the boundaries as defined by the text grid and maximum energy
# Now, round to a glottal pulse
semiPhonCount = phonCount
appendInfoLine: "Will create ", phonCount, " pieces"

# Create a segmentation textgrid
selectObject: textGridSegmented
for i to semiPhonCount
  #appendInfoLine: i, "-------"
  my_label$ = labels$[i]
  insertedIntervals = Get number of intervals... 2
  Set interval text: 2, insertedIntervals, my_label$
  appendInfoLine: "End ", i, " ", semiPhonEnd[i], " ", my_label$
  if semiPhonEnd[i] != const_total_time
    Insert boundary: 2, semiPhonEnd[i]
  endif
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
  selectObject: mfccObj
  #appendInfoLine: semiPhonStart[i], " ", semiPhonEnd[i]
  startFrames[i] = Get frame number from time... semiPhonStart[i]
  endFrames[i] = Get frame number from time... semiPhonEnd[i]

  startFrames[i] = floor(startFrames[i])
  endFrames[i] = floor(endFrames[i])

  selectObject: soundObj
  semiPhonEnergy[i] = Get energy... semiPhonStart[i] semiPhonEnd[i]
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
appendFileLine: outputFile$, "intervals=", semiPhonCount

@unforcePointProcess
# Output all semiphons
checkDuration = 0
semiPhonStart[1] = 0
for i to semiPhonCount
  startPoint_ = semiPhonStart[i]
  endPoint_ = semiPhonEnd[i]
  label_$ = labels$[i]
  duration = endPoint_ - startPoint_
  startFrame_ = startFrames[i]
  endFrame_ = endFrames[i]
  energy = semiPhonEnergy[i]

  checkDuration += duration
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
appendInfoLine: "Total duration of extracted ", checkDuration, " from ", semiPhonCount, " parts"

# Extract pulses to be used to determine cut points
# Ok, sooo I need to know which parts are voiced/unvoiced, hence I can't use the
# 'forced' version above
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
  appendFileLine: outputFile$, "energy=", energy

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
