form Feature Extraction
     comment I/O file paths
     sentence soundPath1 /home/ivo/SpeechSynthesis/corpus-small/Diana_A.1.2.Un1_001.wav
     sentence soundPath2 /home/ivo/SpeechSynthesis/corpus-small/Diana_A.1.2.Un1_001.wav
endform

timeStep=0.005
mfccCount=12

soundObj1 = Read from file... 'soundPath1$'
selectObject: soundObj1
Rename... Sound1

# generate MFCC
# To MFCC... : <coef count> <window length> <time step>
selectObject: soundObj1
mfccObj = To MFCC... mfccCount (4*timeStep) timeStep 220 100 0.0
cepstra[1] = To Matrix

soundObj2 = Read from file... 'soundPath2$'
selectObject: soundObj2
Rename... Sound2

# generate MFCC
# To MFCC... : <coef count> <window length> <time step>
selectObject: soundObj2
mfccObj = To MFCC... mfccCount (4*timeStep) timeStep 100 100 0.0
cepstra[2] = To Matrix

@diff_cepstra

writeInfoLine: result

procedure diff_cepstra
    selectObject: cepstra[1]
    xMax1 = Get number of columns
    yMax1 = Get number of rows
    selectObject: cepstra[2]
    xMax2 = Get number of columns
    yMax2 = Get number of rows

    xMax = min(xMax1, xMax2)
    yMax = mfccCount

    result = 0
    for i to xMax
        dist = 0
		for j to yMax
			selectObject: cepstra[1]
			y1 = Get value in cell... j i

			selectObject: cepstra[2]
			y2 = Get value in cell... j i
			dist = dist + (y1 - y2) * (y1 - y2)
        endfor
        result = result + dist
    endfor
endproc
