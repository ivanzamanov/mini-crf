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

for i to segments
    line$ = Get string... i
    fileName$ = extractWord$(line$, "File=")
    startTime = extractNumber(line$, "Start=")
    endTime = extractNumber(line$, "End=")
    desiredPitch = extractNumber(line$, "Pitch=")
    desiredDuration = extractNumber(line$, "Duration=")

    fileNames$[i] = fileName$
    startTimes[i] = startTime
    endTimes[i] = endTime
    pitches[i] = desiredPitch
    durations[i] = desiredDuration
endfor

duration = 0
boundaries[0] = 0
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
    dur = Get total duration
    duration += dur
    selectObject: sound
    Remove
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
for i to segments
    selectObject: parts[i]
    Remove
endfor

selectObject: concat
manipulation = To Manipulation... 0.005 75 600
selectObject: manipulation
pitchTier = Extract pitch tier

for i from 1 to segments-1
    selectObject: pitchTier
    mid1 = (boundaries[i-1] + boundaries[i]) / 2
    mid2 = (boundaries[i] + boundaries[i+1]) / 2
    #appendInfoLine: "Mids ", mid1, " and ", mid2

    p1 = Get low index from time... mid1
    p2 = Get high index from time... mid2
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

    if p1 != p2
        for point from p1 to p2
            time = Get time from index... point
            newPitch = startPitch + (endPitch - startPitch) * (time - time1) / (time2 - time1)
            Remove point... point
            Add point... time newPitch
        endfor
    endif
endfor

# Duration
selectObject: manipulation
durationTier = Create DurationTier... Duration 0 duration
for i from 1 to segments
    startTime = startTimes[i]
    endTime = endTimes[i]
    oldDuration = (endTime - startTime)
    newDuration = durations[i]
    scale = newDuration / oldDuration

    selectObject: durationTier
    point1 = boundaries[i-1] + 0.0001
    point2 = boundaries[i] - 0.0001
    Add point... point1 scale
    Add point... point2 scale
endfor

selectObject: durationTier
plusObject: manipulation
Replace duration tier

selectObject: pitchTier
plusObject: manipulation
#Replace pitch tier

selectObject: manipulation
result = Get resynthesis (overlap-add)

selectObject: concat
Remove
selectObject: manipulation
Remove
selectObject: pitchTier
Remove
selectObject: strings
Remove

selectObject: result
Rename... concat-result
Save as WAV file... 'outputPath$'
