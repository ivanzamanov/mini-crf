form Feature Extraction
   comment I/O file paths
   sentence soundPath /home/ivo/SpeechSynthesis/corpus-test/Diana_A.1.1.Un2/Diana_A.1.1.Un2_0023.wav
   sentence textGridPath /home/ivo/SpeechSynthesis/corpus-test/Diana_A.1.1.Un2/Diana_A_1_1_Un2_0023.TextGrid
   sentence outputFile /home/ivo/praat-output.txt
   comment Generic
   real timeStep 0.005
   comment MFCC extraction parameters
   natural mfccCount 12
   comment Pitch extraction parameters
endform

#writeInfoLine: "Parameters:"
#appendInfoLine: "Sound file: ", soundPath$
#appendInfoLine: "TextGrid file: ", textGridPath$
#appendInfoLine: "Output file: ", outputFile$
#appendInfoLine: "Time step: ", timeStep
#appendInfoLine: "MFCC count: ", mfccCount
#appendInfoLine: ""

deleteFile: outputFile$
appendFileLine: outputFile$, "[Config]"
appendFileLine: outputFile$, "timeStep=", timeStep
appendFileLine: outputFile$, "mfcc=", mfccCount

textGridObj = Read from file... 'textGridPath$'
Rename... TextGrid

intervalCount = Get number of intervals... 1
phonemesCount = intervalCount * 2
appendFileLine: outputFile$, "intervals=", phonemesCount

soundObj = Read from file... 'soundPath$'
selectObject: soundObj
Rename... Sound

# Extract pitch tier
pitch = To Pitch... timeStep 75 600
selectObject: pitch
Rename... PitchTier
pitchTier = Down to PitchTier

# Extract pulses to be used to determine cut points
selectObject: soundObj
pitchTmp = To Pitch (ac)... timeStep 75 20 1 0 0 0.01 0.35 0.14 600
pointProcess = To PointProcess
selectObject: pitchTmp
Remove

# generate MFCC
soundObjOriginal = soundObj
selectObject: soundObj
soundObj = Filter (pre-emphasis)... 50
totalDuration = Get total duration
mfccObj = To MFCC... mfccCount (4*timeStep) timeStep 100 100 0.0
selectObject: mfccObj
maxMFCCFrameCount = Get number of frames

textGridSegmented = Create TextGrid... 0 totalDuration phonemes phonemes

lastTime = 0
lastFrame = 1
procedure getFrameBoundaries
  startPoint_ = lastTime

  selectObject: soundObj
  #appendInfoLine: startPoint, " ", endPoint_
  #Extract part for overlap... startPoint endPoint_ 0.1

  selectObject: mfccObj
  startFrame = Get frame number from time... startPoint_
  startFrame = max(1, startFrame)
  endFrame = Get frame number from time... endPoint_

  startFrame = floor(startFrame)
  endFrame = ceiling(endFrame)

  startFrame = max(startFrame, lastFrame)
  endFrame = max(endFrame, startFrame + 1)

  # Explicitly - first feature frame is the last frame...
  startFrame = lastFrame
  lastFrame = endFrame

  lastTime = endPoint_
endproc

procedure findMaxEnergyPoint
  analysisFrame_ = 0.005
  count_ = (endPoint - startPoint) / analysisFrame_
  max_ = 0
  maxEnergyPoint = 0
  for i_ to count_ - 2
    p1_ = startPoint + (i_ * analysisFrame_)
    p2_ = p1_ + analysisFrame_
    energy_ = Get energy... p1_ p2_
    if energy_ > max_
      maxEnergyPoint = (p1_ + p2_) / 2
      max_ = energy_
    endif
  endfor
endproc

procedure getPulseBoundaries
  selectObject: pointProcess
  pointsCount_ = Get number of points
  startIndex = Get low index... startPoint_
  endIndex = Get high index... endPoint_

  #endIndex = max(endIndex, pointsCount_)
  startIndex = max(startIndex, 1)

  startPulsePoint = Get time from index... startIndex
  endPulsePoint = Get time from index... endIndex

  selectObject: pitchTier
  pitchAtStart = Get value at time... startPoint_
  pitchAtEnd = Get value at time... endPoint_

  if abs(startPoint_ - startPulsePoint) <= 1/pitchAtStart
    startPoint_ = startPulsePoint
  endif

  appendInfoLine: endPoint_, " ", endPulsePoint, " ", 1/pitchAtEnd, " ", endIndex, " ", startIndex
  if abs(endPoint_ - endPulsePoint) <= 1/pitchAtEnd
    endPoint_ = endPulsePoint
  endif

  #appendInfoLine: startPoint, " ", endPoint
  false = 0
  if false != 0
    selectObject: textGridSegmented
    Insert point... 1 startPoint_
    intervalIndex = Get number of points... 1
    Set point text... 1 intervalIndex "s"
    Insert point... 1 endPoint_
    intervalIndex = Get number of points... 1
    Set point text... 1 intervalIndex "e"
  endif

endproc

procedure outputEntry
  appendFileLine: outputFile$, "[Entry]"
  appendFileLine: outputFile$, "label=", label_$
  appendFileLine: outputFile$, "start=", startPoint_
  appendFileLine: outputFile$, "end=", endPoint_
  #appendFileLine: outputFile$, "frames=", (endFrame - startFrame + 1)
  appendFileLine: outputFile$, "frames=", 2
  appendFileLine: outputFile$, "duration=", duration

  if endPoint_ <= startPoint_
   exitScript: "Fuck...", startPoint_, " ", endPoint_
  endif

  featureFrames[1] = startFrame
  featureFrames[2] = endFrame
  for k to 2
    frame = featureFrames[k]
    selectObject: pitch
    value = Get value in frame: frame, "Hertz"
    appendFileLine: outputFile$, "pitch=", value
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

for i to intervalCount
  selectObject: textGridObj
  intervalLabel$ = Get label of interval... 1 i

  startPoint = Get start point... 1 i
  endPoint = Get end point... 1 i

  # Find point of maximum energy between startPoint and endPoint
  selectObject: soundObj
  @findMaxEnergyPoint

  entryPoints[1] = startPoint
  entryPoints[2] = maxEnergyPoint
  entryPoints[3] = endPoint
  labels$[1] = "-" + intervalLabel$
  labels$[2] = intervalLabel$ + "-"

  for entryPointId to 2
    label_$ = labels$[entryPointId]
    startPoint_ = entryPoints[entryPointId]
    endPoint_ = entryPoints[entryPointId + 1]

    @getPulseBoundaries

    duration = endPoint_ - startPoint_

    # Split in semi-phonemes here
    @getFrameBoundaries
    @outputEntry
  endfor
endfor

# And clean up a bit

selectObject: pitchTier
#Remove
selectObject: mfccObj
Remove
selectObject: pointProcess
#Remove
selectObject: soundObj
Remove
selectObject: pitch
Remove

appendInfoLine: "Done"
