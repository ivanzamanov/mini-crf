form Feature Extraction
     comment I/O file paths
     sentence soundPath /home/ivo/SpeechSynthesis/corpus/Diana_A.1.1.Un2/Diana_A.1.1.Un2_001.wav
     sentence textGridPath /home/ivo/SpeechSynthesis/corpus/Diana_A.1.1.Un2/Diana_A_1_1_Un2_001.TextGrid
     sentence outputFile output.txt
     comment Generic
     real timeStep 0.005
     comment MFCC extraction parameters
     natural mfccCount 12
     real mfccWindowLength 0.015
     comment Pitch extraction parameters
endform

writeInfoLine: "Parameters:"
appendInfoLine: "Sound file: ", soundPath$
appendInfoLine: "TextGrid file: ", textGridPath$
appendInfoLine: "Output file: ", outputFile$
appendInfoLine: "Time step: ", timeStep
appendInfoLine: "MFCC count: ", mfccCount
appendInfoLine: "MFCC window length: ", mfccWindowLength
appendInfoLine: ""

deleteFile: outputFile$
appendFileLine: outputFile$, "[Config]"
appendFileLine: outputFile$, "timeStep=", timeStep
appendFileLine: outputFile$, "mfccWindow=", mfccWindowLength
appendFileLine: outputFile$, "mfcc=", mfccCount

textGridObj = Read from file... 'textGridPath$'
Rename... TextGrid

intervalCount = Get number of intervals... 1
appendFileLine: outputFile$, "intervals=", intervalCount

soundObj = Read from file... 'soundPath$'
selectObject: soundObj
Rename... Sound

pitch = To Pitch... timeStep 75 600
selectObject: pitch
Rename... Pitch

# generate MFCC
# To MFCC... : <coef count> <window length> <time step>
selectObject: soundObj
mfccObj = To MFCC... mfccCount mfccWindowLength timeStep 100 100 0.0
selectObject: mfccObj
maxMFCCFrameCount = Get number of frames

procedure toPrecision: toPrecision.value
    #appendInfoLine: "toPrecision arg ", toPrecision.value
    procresult = round( toPrecision.value / timeStep) * timeStep
    #appendInfoLine: "toPrecision ", procresult
endproc

for i to intervalCount
    selectObject: textGridObj
    intervalLabel$ = Get label of interval... 1 i

    startPoint = Get start point... 1 i
    @toPrecision: startPoint
    startPoint = procresult

    endPoint = Get end point... 1 i
    @toPrecision: endPoint
    endPoint = procresult

    timePoint = startPoint + (timeStep / 2)
    appendFileLine: outputFile$, "[Entry]"
    appendFileLine: outputFile$, "label=", intervalLabel$
    appendFileLine: outputFile$, "start=", timePoint
    appendFileLine: outputFile$, "end=", endPoint
    pointsCount = 0
    while timePoint < endPoint
        pointsCount += 1
        timePoint = timePoint + timeStep
    endwhile
    selectObject: mfccObj
    timePoint = startPoint + (timeStep / 2)

    #appendInfoLine: timePoint, " at ", i

    appendFileLine: outputFile$, "frames=", pointsCount
    while timePoint < endPoint
        frame = Get frame number from time... timePoint
        frame = floor(frame)

        if frame < 1
           frame = 1
        endif
        if frame > maxMFCCFrameCount
           frame = maxMFCCFrameCount
        endif

        selectObject: pitch
        value = Get value at time: timePoint, "Hertz", "Linear"
        appendFileLine: outputFile$, "pitch=", value
        selectObject: mfccObj
        for j to mfccCount
            value = Get value in frame... frame j
            appendFileLine: outputFile$, j, "=", value
        endfor
        timePoint = timePoint + timeStep
    endwhile
endfor
