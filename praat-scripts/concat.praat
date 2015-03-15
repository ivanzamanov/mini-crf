form Concatenation
     comment Input file
     sentence fileName /home/ivo/praat-test
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

    fileNames$[i] = fileName$
    startTimes[i] = startTime
    endTimes[i] = endTime
endfor

for i to segments
    fileName$ = fileNames$[i]
    sound = Read from file... 'fileName$'
    selectObject: sound

    part = Extract part... startTimes[i] endTimes[i] rectangular 1.0 0
    parts[i] = part
    selectObject: part
endfor

for i to segments
    part = parts[i]
    plusObject: part
endfor

Concatenate