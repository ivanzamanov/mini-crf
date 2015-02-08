form Feature Extraction
     comment I/O file paths
     sentence soundPath /home/ivo/SpeechSynthesis/corpus/Diana_A.1.1.Un2/Diana_A.1.1.Un2_001.wav
     sentence textGridPath /home/ivo/SpeechSynthesis/corpus/Diana_A.1.1.Un2/Diana_A_1_1_Un2_001.TextGrid
     sentence matrix_file /home/ivo/praat-tmp/mfcc.Matrix
     sentence pitch_matrix /home/ivo/praat-tmp/pitch.Matrix
     comment Generic
     real timeStep 0.005
     comment MFCC extraction parameters
     natural mfccCount 12
     real mfccWindowLength 0.015
     comment Pitch extraction parameters
endform

writeInfoLine: "Parameters:"
appendInfoLine: "Sound file: ", soundPath$
appendInfoLine: "Matrix file: ", matrix_file$
appendInfoLine: "Pitch Matrix file: ", pitch_matrix$
appendInfoLine: "Time step: ", timeStep
appendInfoLine: "MFCC count: ", mfccCount
appendInfoLine: "MFCC window length: ", mfccWindowLength
appendInfoLine: ""

textGridObj = Read from file... 'textGridPath$'
Rename... TextGrid

intervalCount = Get number of intervals... 1
for i to intervalCount
	intervalLabel$ = Get label of interval... 1 i
	startPoint = Get start point... 1 i
	endPoint = Get end point... 1 i
	appendInfoLine: "Label = ", intervalLabel$, " Start = ", startPoint, " End = ", endPoint
endfor

soundObj = Read from file... 'soundPath$'
selectObject: soundObj
Rename... Sound

# generate MFCC
# To MFCC... : <coef count> <window length> <time step>
mfccObj = To MFCC... mfccCount mfccWindowLength timeStep 100 100 0.0
selectObject: mfccObj

mfccMatrixObj = To Matrix
#removeObject: mfccObj
selectObject: mfccMatrixObj
columns = Get number of columns
rows = Get number of rows

#Trim MFCC matrix rows...
mfccMatrixObjTrimmed = Create simple Matrix... "MFCC Trimmed" mfccCount columns x*y
for i to mfccCount
    for j to columns
        selectObject: mfccMatrixObj
        value = Get value in cell... i j

        selectObject: mfccMatrixObjTrimmed
        Set value... i j value
    endfor
endfor

removeObject: mfccMatrixObj
Rename... MFCC
mfccMatrixObj = mfccMatrixObjTrimmed
rows = mfccCount

appendInfoLine: "Size: ", rows, "x", columns

selectObject: mfccMatrixObj
appendInfoLine: "Saving to ", matrix_file$
Save as text file... 'matrix_file$'

selectObject: soundObj
pitch = To Pitch... timeStep 75 600
selectObject: pitch
Rename... Pitch
i = 0
pitch_matrix = Create simple Matrix... Pitch_Matrix 1 columns x*y
for i to columns
    selectObject: pitch
    pitch_value = Get value at time: i*timeStep, "Hertz", "Linear"
    if pitch_value = undefined
       pitch_value = 0
    endif
    selectObject: pitch_matrix
    Set value... 1 i pitch_value
endfor

Save as text file... 'pitch_matrix$'