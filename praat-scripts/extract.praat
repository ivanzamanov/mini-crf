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

pitch = To Pitch... timeStep 75 600
selectObject: pitch
Rename... PitchTier
pitchTier = Down to PitchTier

# generate MFCC
# To MFCC... : <coef count> <window length> <time step>
selectObject: soundObj
soundObj = Filter (pre-emphasis)... 50
mfccObj = To MFCC... mfccCount (4*timeStep) timeStep 100 100 0.0
selectObject: mfccObj
maxMFCCFrameCount = Get number of frames

procedure getFrameBoundaries
    startFrame = lastFrame

    selectObject: pitchTier
    highPitchPoint = Get low index from time... endPoint
    highPitchPoint = max(1, highPitchPoint)
    highPitchPointTime = Get time from index... highPitchPoint

    if abs(highPitchPointTime - endPoint) < timeStep
        endPoint = highPitchPointTime
    endif

    selectObject: mfccObj
    endFrame = Get frame number from time... endPoint
    endFrame = min(endFrame, maxMFCCFrameCount)

    endFrame = round(endFrame)
    endFrame = max(endFrame, 1)

    lastFrame = endFrame

    startPoint = Get time from frame number... startFrame
    endPoint = Get time from frame number... endFrame
endproc

lastFrame = 1
for i to intervalCount
    selectObject: textGridObj
    intervalLabel$ = Get label of interval... 1 i

    appendFileLine: outputFile$, "[Entry]"
    appendFileLine: outputFile$, "label=", intervalLabel$

    startPoint = Get start point... 1 i
    endPoint = Get end point... 1 i

    @getFrameBoundaries

    appendFileLine: outputFile$, "start=", startPoint
    appendFileLine: outputFile$, "end=", endPoint
    appendFileLine: outputFile$, "frames=", (endFrame - startFrame + 1)

    for frame from startFrame to endFrame
        selectObject: pitch
        value = Get value in frame: frame, "Hertz"
        appendFileLine: outputFile$, "pitch=", value
        selectObject: mfccObj
        for j to mfccCount
            value = Get value in frame: frame, j
            appendFileLine: outputFile$, j, "=", value
        endfor
    endfor
endfor
