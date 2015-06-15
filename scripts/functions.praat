
# Input: startPoint, endPoint, timeStep
# Object: Sound
# Output: maxEnergyPoint
procedure findMaxEnergyPoint
  analysisFrame_ = timeStep
  count_ = (endPoint - startPoint - timeStep * 2) / analysisFrame_
  max_ = 0
  maxEnergyPoint = 0
  for i_ to count_
    p1_ = startPoint + (i_ * analysisFrame_)
    p2_ = p1_ + analysisFrame_
    energy_ = Get energy... p1_ p2_
    #appendInfoLine: p1_, " ", p2_
    # Undefined check for edge case at end of file
    if (energy_ != undefined) && energy_ > max_
    #if energy_ > max_
      maxEnergyPoint = (p1_ + p2_) / 2
      max_ = energy_
    endif
  endfor

  # Extreme edge cases
  if maxEnergyPoint == startPoint || maxEnergyPoint >= endPoint || maxEnergyPoint == 0
    maxEnergyPoint = (endPoint - startPoint) / 2
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
  startIndex = max(startIndex, 1)
  endIndex = max(endIndex, 2)

  startIndex = min(maxIndex_ - 1, startIndex)
  endIndex = min(maxIndex_, endIndex)

  # It's possible that a max energy point is
  # between pulse end and start point
  endIndex = max(endIndex, startIndex + 1)
  minStartIndex = endIndex

  #appendInfoLine: i, ": ", startIndex, " ", endIndex
  startPulsePoint = Get time from index... startIndex
  endPulsePoint = Get time from index... endIndex

  #selectObject: soundObj
  #startPulsePoint = Get nearest zero crossing... 1 startPulsePoint
  #endPulsePoint = Get nearest zero crossing... 1 endPulsePoint
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
