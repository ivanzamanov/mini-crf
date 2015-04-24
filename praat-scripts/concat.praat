form Concatenation
     comment Input file
     sentence fileName D:\cygwin\home\ivo\SpeechSynthesis\concat-test.txt
     comment Output file
     sentence outputPath /home/ivo/concat-output.wav
endform

strings = Read Strings from raw text file... 'fileName$'
selectObject: strings

segments = Get number of strings

for i to segments
    line$ = Get string... i
    fileName$ = extractWord$(line$, "File=")
    startTime = extractNumber(line$, "Start=")
    endTime = extractNumber(line$, "End=")

    fileNames$[i] = fileName$
    startTimes[i] = startTime
    endTimes[i] = endTime
endfor

duration = 0
boundaries[0] = 0
for i to segments
    fileName$ = fileNames$[i]
    sound = Read from file... 'fileName$'
    selectObject: sound

    duration = duration + (endTimes[i] - startTimes[i])
    part = Extract part... startTimes[i] endTimes[i] rectangular 1.0 0
    parts[i] = part
    boundaries[i] = duration

    selectObject: sound
    Remove
    selectObject: part
endfor

for i to segments
    part = parts[i]
    plusObject: part
endfor

concat = Concatenate
for i to segments
    selectObject: parts[i]
    Remove
endfor

selectObject: concat
manipulation = To Manipulation... 0.005 75 600
pitchTier = Extract pitch tier
pitchCopy = Copy...

for i from 1 to segments-1
    mid1 = (boundaries[i-1] + boundaries[i]) / 2
    mid2 = (boundaries[i] + boundaries[i+1]) / 2
    #appendInfoLine: "Mids ", mid1, " and ", mid2

    p1 = Get nearest index from time... mid1
    p2 = Get nearest index from time... mid2
    #appendInfoLine: "Indices ", p1, " and ", p2

    time1 = Get time from index... p1
    time2 = Get time from index... p2

    if (time1 >= boundaries[i-1] && time1 <= boundaries[i] && time2 >= boundaries[i] && time2 <= boundaries[i+1])
        #appendInfoLine: "Interpolating between ", time1, " and ", time2
        startPitch = Get value at index... p1
        endPitch = Get value at index... p2

        Remove points between... time1 time2
        # Actual interpolation needed
        count = p2 - p1
        #appendInfoLine: "Interpolation points ", count
        anchor = (endPitch + ((endPitch + startPitch) / 2)) / 2
        #appendInfoLine: "Interpolate: ", startPitch, " | ", anchor, " | ", endPitch, " | ", count

        for p from 0 to count
            t = p / count

            v1 = (1 - t) * (1 - t)
            v2 = (1 - t) * t
            v3 = t * t

            #pitch = v1*startPitch + v2*anchor + v3*endPitch
            pitch = startPitch + (p/count)*(endPitch - startPitch)
            #appendInfoLine: v1, " | ", v2, " | ", v3, " | ", pitch, " | ", t
            
            timePoint = (time1 + (time2 - time1) * t)
            Add point... timePoint pitch
            #appendInfoLine: "Add point ", timePoint, " ", pitch, " ", t
        endfor
    endif
endfor

selectObject: pitchCopy
plusObject: manipulation
Replace pitch tier

selectObject: manipulation
result = Get resynthesis (overlap-add)

selectObject: manipulation
Remove
selectObject: pitchTier
Remove
selectObject: pitchCopy
Remove
selectObject: strings
Remove

#Save as WAV file... 'outputPath$'
