
# Input: startPoint, endPoint, timeStep
# Object: Sound
# Output: maxEnergyPoint
procedure findMaxEnergyPoint
  startSample = Get sample number from time... startPoint
  startSample = floor(startSample + 1)
  endSample = Get sample number from time... endPoint
  endSample = floor(endSample)

  max_ = 0
  maxEnergyPoint = 0
  for i_ from startSample to endSample
    value_ = Get value at sample number... i_
    value_ = abs(value_)
    #appendInfoLine: i_, " = ", value_
    if max_ < value_
       #appendInfoLine: i, ": MAX from ", max_, " to ", value_
       maxEnergyPoint = i_
       max_ = value_
    endif
  endfor
  maxEnergyPoint = Get time from sample number... maxEnergyPoint
  appendInfoLine: i, ": Max en == ", maxEnergyPoint, " value = ", max_

  # Extreme edge cases
  if maxEnergyPoint == startPoint
     appendInfoLine: i, ": MaxEn point == start point"
     maxEnergyPoint = startPoint + 0.001
  endif
  if maxEnergyPoint >= endPoint
     maxEnergyPoint = endPoint - 0.001
     appendInfoLine: i, ": MaxEn point >= end point"
  endif
  if maxEnergyPoint == 0
     appendInfoLine: i, ": MaxEn point == 0"
  endif
  if (maxEnergyPoint - startPoint) < minLength
     appendInfoLine: i, ": MaxEn too close to start"
     maxEnergyPoint = startPoint + 0.001
  endif
  if (endPoint - maxEnergyPoint) < minLength
     appendInfoLine: i, ": MaxEn too close to end"
     maxEnergyPoint = endPoint - 0.001
  endif
endproc

# Input: startPoint_, endPoint_, minStartIndex
# Objects: pointProcess, pitch
# Output: startPulsePoint, endPulsePoint
procedure getPulseBoundaries
  #appendInfoLine: i, ": ", startPoint_, " ", endPoint_
  selectObject: pointProcess
  maxIndex_ = Get number of points
  startIndex = Get high index... startPoint_
  endIndex = Get high index... endPoint_

  # Required for edge cases such as start and end of file
  startIndex = minStartIndex
  endIndex = max(startIndex + 1, endIndex)

  startIndex = min(maxIndex_ - 1, startIndex)
  endIndex = min(maxIndex_, endIndex)

  # It's possible that a max energy point is
  # between pulse end and start point
  endIndex = max(endIndex, startIndex + 1)
  minStartIndex = endIndex

  #appendInfoLine: i, ": ", startIndex, " ", endIndex
  if startIndex != 0
    startPulsePoint = Get time from index... startIndex
  else
    startPulsePoint = startIndex
  endif
  endPulsePoint = Get time from index... endIndex

  #selectObject: soundObj
  #startPulsePoint = Get nearest zero crossing... 1 startPulsePoint
  #endPulsePoint = Get nearest zero crossing... 1 endPulsePoint
endproc

procedure forcePointProcess
  selectObject: pointProcess
  originalPointProcess = Copy... "PointProcessOriginal"
  selectObject: pointProcess
  Voice... 0.01 0.02000000001
endproc

procedure unforcePointProcess
  selectObject: pointProcess
  Remove
  pointProcess = originalPointProcess
endproc

# TODO
procedure getFrameBoundaries
  startPoint_ = lastTime

  selectObject: soundObj

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
