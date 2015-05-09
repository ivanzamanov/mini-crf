form Concatenation
     comment Input file
     sentence fileName concat-input.txt
     comment Output file
     sentence outputPath concat-output.wav
endform

writeInfo: ""
strings = Read Strings from raw text file... 'fileName$'
selectObject: strings

segments = Get number of strings

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
endfor

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
    resample = Resample... 24000 50
    selectObject: part
    Remove
    part = resample
    selectObject: part
    selectObject: sound
    Remove
    duration += durations[i]
    parts[i] = part
    boundaries[i] = duration

    selectObject: part
endfor

for i to segments
    part = parts[i]
    plusObject: part
endfor

appendInfoLine: "Concatenating ", segments, " parts"
concat = Concatenate
Rename... PlainConcatenation
for i to segments
    selectObject: parts[i]
    Remove
endfor

selectObject: concat
manipulation = To Manipulation... 0.005 75 600
selectObject: manipulation

pitchTier = Extract pitch tier
blankPitch = Create PitchTier... NewPitchTier 0 duration

for i from 1 to segments-1
    selectObject: pitchTier
    mid1 = (boundaries[i-1] + boundaries[i]) / 2
    mid2 = (boundaries[i] + boundaries[i+1]) / 2
    #appendInfoLine: "Mids ", mid1, " and ", mid2

    p1 = Get nearest index from time... mid1
    p2 = Get nearest index from time... mid2
    if p1 == 0
         p1 = 1
    endif
    totalPoints = Get number of points
    if p2 >= totalPoints
        p2 = 1
    endif
    #appendInfoLine: "Indices ", p1, " and ", p2

    time1 = Get time from index... p1
    time2 = Get time from index... p2

    startPitch = pitches[i]
    endPitch = pitches[i+1]

    selectObject: blankPitch
    Add point... mid1 startPitch
    Add point... mid2 endPitch
endfor

# Duration
durationTier = Create DurationTier... Duration 0 duration
totalOldDuration = 0
totalNewDuration = 0

for i to segments
    startTime = startTimes[i]
    endTime = endTimes[i]
    oldDuration = (endTime - startTime)
    newDuration = durations[i]
    totalNewDuration += newDuration
    newEndTimes[i] = totalNewDuration
    scale = newDuration / oldDuration
    #appendInfoLine: oldDuration, " ", newDuration, " ", scale

    selectObject: durationTier
    point1 = totalOldDuration + 0.00001
    totalOldDuration += oldDuration
    point2 = totalOldDuration - 0.00001
    Add point... point1 scale
    Add point... point2 scale
endfor

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

selectObject: durationTier
plusObject: manipulation
Replace duration tier

selectObject: pitchTier
selectObject: blankPitch
plusObject: manipulation
Replace pitch tier

selectObject: manipulation
result = Get resynthesis (overlap-add)

selectObject: blankPitch
Remove
selectObject: durationTier
Remove
selectObject: concat
#Remove
selectObject: manipulation
Remove
selectObject: pitchTier
Remove
selectObject: strings
Remove

selectObject: result
Rename... Concatenation
#Save as WAV file... 'outputPath$'
