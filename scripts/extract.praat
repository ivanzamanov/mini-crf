form Feature Extraction
   comment I/O file paths
   sentence soundPath /home/ivo/SpeechSynthesis/corpus-small/Diana_A.1.2.Un1_001.wav
   sentence textGridPath /home/ivo/SpeechSynthesis/corpus-small/Diana_A_1_2_Un1_001.TextGrid
   sentence outputFile /home/ivo/praat-output.txt
   comment Generic
   real timeStep 0.005
   comment MFCC extraction parameters
   natural mfccCount 12
   comment Pitch extraction parameters
endform

writeInfoLine: "Parameters:"
appendInfoLine: "Sound file: ", soundPath$
appendInfoLine: "TextGrid file: ", textGridPath$
appendInfoLine: "Output file: ", outputFile$
appendInfoLine: "Time step: ", timeStep
appendInfoLine: "MFCC count: ", mfccCount
appendInfoLine: ""

deleteFile: outputFile$
appendFileLine: outputFile$, "[Config]"
appendFileLine: outputFile$, "timeStep=", timeStep
appendFileLine: outputFile$, "mfcc=", mfccCount

textGridObj = Read from file... 'textGridPath$'
Rename... TextGrid

intervalCount = Get number of intervals... 1
appendFileLine: outputFile$, "intervals=", intervalCount

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
  startPoint = lastTime

  selectObject: soundObj
  #appendInfoLine: startPoint, " ", endPoint
  #Extract part for overlap... startPoint endPoint 0.1

  selectObject: mfccObj
  startFrame = Get frame number from time... startPoint
  startFrame = max(1, startFrame)
  endFrame = Get frame number from time... endPoint

  startFrame = floor(startFrame)
  endFrame = ceiling(endFrame)

  startFrame = max(startFrame, lastFrame)
  endFrame = max(endFrame, startFrame + 1)

  # Explicitly - first feature frame is the last frame...
  startFrame = lastFrame
  lastFrame = endFrame

  lastTime = endPoint
endproc

procedure getPulseBoundaries
  selectObject: pointProcess
  startIndex = Get nearest index... startPoint
  endIndex = Get nearest index... endPoint

  startPulsePoint = Get time from index... startIndex
  endPulsePoint = Get time from index... endIndex

  selectObject: pitchTier
  pitchAtStart = Get value at time... startPoint
  pitchAtEnd = Get value at time... endPoint

  if abs(startPoint - startPulsePoint) <= 1/pitchAtStart
    startPoint = startPulsePoint
  endif

  #appendInfoLine: endPoint, " ", endPulsePoint, " ", 1/pitchAtEnd, " ", endIndex
  if abs(endPoint - endPulsePoint) <= 1/pitchAtEnd
    endPoint = endPulsePoint
  endif

  #appendInfoLine: startPoint, " ", endPoint
  false = 0
  if false != 0
    selectObject: textGridSegmented
    Insert point... 1 startPoint
    intervalIndex = Get number of points... 1
    Set point text... 1 intervalIndex "s"
    Insert point... 1 endPoint
    intervalIndex = Get number of points... 1
    Set point text... 1 intervalIndex "e"
  endif

endproc

for i to intervalCount
  selectObject: textGridObj
  intervalLabel$ = Get label of interval... 1 i

  appendFileLine: outputFile$, "[Entry]"
  appendFileLine: outputFile$, "label=", intervalLabel$

  startPoint = Get start point... 1 i
  endPoint = Get end point... 1 i

  @getPulseBoundaries

  duration = endPoint - startPoint

  @getFrameBoundaries

  appendFileLine: outputFile$, "start=", startPoint
  appendFileLine: outputFile$, "end=", endPoint
  #appendFileLine: outputFile$, "frames=", (endFrame - startFrame + 1)
  appendFileLine: outputFile$, "frames=", 2
  appendFileLine: outputFile$, "duration=", duration

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
endfor

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
