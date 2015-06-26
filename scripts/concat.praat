form Concatenation
   comment Input file
   sentence fileName /home/ivo/SpeechSynthesis/mini-crf/example-concat.txt
   comment Output file
   sentence outputPath /tmp/concat-output.wav
endform

writeInfo: ""
strings = Read Strings from raw text file... 'fileName$'
selectObject: strings

segments = Get number of strings

crossfadeTime = 0.05

totalOldDuration = 0
for i to segments
  line$ = Get string... i
  fileName$ = extractWord$(line$, "File=")
  startTime = extractNumber(line$, "Start=")
  endTime = extractNumber(line$, "End=")
  desiredPitch = extractNumber(line$, "Pitch=")
  desiredDuration = extractNumber(line$, "Duration=")
  label$ = extractWord$(line$, "Label=")

  fileNames$[i] = fileName$
  startTimes[i] = startTime
  endTimes[i] = endTime
  pitches[i] = desiredPitch
  durations[i] = desiredDuration
  labels$[i] = label$

  totalOldDuration += endTimes[i] - startTimes[i]
  #appendInfoLine: endTimes[i], " ", startTimes[i], " ", totalOldDuration
endfor

#appendInfoLine: totalOldDuration
duration = 0
textGrid = Create TextGrid... 0 totalOldDuration PlainConcatenation ""
for i to segments
  duration += (endTimes[i] - startTimes[i])
  if i != segments
    Insert boundary... 1 duration
  endif
  label$ = labels$[i]
  Set interval text... 1 i 'label$'
endfor

boundaries[0] = 0
duration = 0
for i to segments
  fileName$ = fileNames$[i]
  sound = Read from file... 'fileName$'
  selectObject: sound

  part = Extract part... startTimes[i] endTimes[i] rectangular 1.0 0
  oldDurations[i] = endTimes[i] - startTimes[i]
  #part = Extract part for overlap... startTimes[i] endTimes[i] crossfadeTime
  resample = Resample... 24000 50
  selectObject: part
  Remove
  part = resample
  selectObject: part
  selectObject: sound
  Remove
  duration += durations[i]
  #appendInfoLine: i, " duration: ", (endTimes[i] - startTimes[i])
  parts[i] = part
  boundaries[i] = duration

  selectObject: part
endfor

appendInfoLine: "Concatenating ", segments, " parts"
concat = parts[1]
for i from 2 to segments
  selectObject: concat
  lastDuration = oldDurations[i-1]
  selectObject: parts[i]
  nextDuration = oldDurations[i]

  plusObject: concat

  cfTime = min(crossfadeTime, lastDuration / 2)
  cfTime = min(cfTime, nextDuration / 2)
  appendInfoLine: "CF Time: ", cfTime
  oldDurations[i-1] -= cfTime/2
  oldDurations[i] -= cfTime/2
  concat = Concatenate with overlap... cfTime
  concatDuration = Get total duration
  appendInfoLine: "After concat: ", concatDuration
endfor
Rename... PlainConcatenation
selectObject: concat
manipulation = To Manipulation... 0.001 75 600

# Pitch modification
blankPitch = Create PitchTier... NewPitchTier 0 concatDuration
for i to segments - 1
  mid = (boundaries[i-1] + boundaries[i]) / 2
  pitchValue = pitches[i]

  Add point... mid pitchValue
endfor

# Duration modification
durationTier = Create DurationTier... Duration 0 concatDuration
totalOldDuration = 0
totalNewDuration = 0

checkDuration = 0
for i to segments
  plusDuration = crossfadeTime / 2
  if i != 1 && i != segments
    plusDuration = crossfadeTime
  endif

  oldDuration = oldDurations[i]
  newDuration = durations[i]
  totalNewDuration += newDuration
  newEndTimes[i] = totalNewDuration
  scale = newDuration / oldDuration
  appendInfoLine: oldDuration, " -> ", newDuration, " by ", scale
  #appendInfoLine: oldDuration, " ", newDuration, " ", scale

  selectObject: durationTier
  point1 = totalOldDuration

  totalOldDuration += oldDuration
  point2 = totalOldDuration - 0.000001

  Add point... point1 scale
  #appendInfoLine: "Add point ", point1, " ", scale
  Add point... point2 scale
  #appendInfoLine: "Add point ", point2, " ", scale
  
  checkDuration += scale * (point2 - point1)
endfor
appendInfoLine: "Old Duration: ", totalOldDuration
appendInfoLine: "New Duration: ", totalNewDuration
appendInfoLine: "Check Duration: ", checkDuration

# ---- Resynthesis assemble ----
# Set pitch manipulation
selectObject: blankPitch
plusObject: manipulation
Replace pitch tier
selectObject: manipulation

# Set duration manipulation
selectObject: durationTier
plusObject: manipulation
Replace duration tier

# And resynthesize
selectObject: manipulation
#appendInfoLine: "Resynthesizing"
result = Get resynthesis (overlap-add)

outputDuration = Get total duration
appendInfoLine: "Duration after dur manip: ", outputDuration

# Optional - assemble a TextGrid to show how the parts fit together
textGrid = Create TextGrid... 0 totalNewDuration Concatenation ""
for i to segments - 1
  endTime = newEndTimes[i]
  Insert boundary... 1 endTime
endfor
for i to segments - 1
  desiredPitch = pitches[i]
  desiredDuration = durations[i]
  label$ = labels$[i]
  text$ = label$ + " : p=" + string$(desiredPitch) + ", d=" + string$(desiredDuration)
  Set interval text... 1 i 'text$'
endfor

for i to segments
  selectObject: parts[i]
  Remove
endfor

selectObject: blankPitch
Remove
selectObject: durationTier
Remove
selectObject: concat
Remove
selectObject: manipulation
Remove
selectObject: strings
Remove

selectObject: result
Rename... Concatenation
Scale peak... 0.99
Save as WAV file... 'outputPath$'
