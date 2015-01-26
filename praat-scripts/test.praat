# writeInfoLine: "The texts in all tiers and intervals:"

file_name$ = "Diana_A.1.1.Un2_001"
#soundPath$ = "/home/ivo/SpeechSynthesis/corpus/Diana_A.1.1.Un2/"
#matrix_file$ = "/home/ivo/test.Matrix"

form Args
     text soundPath
     text matrix_file
endform

# locate sound

#sound = Read from file... 'soundPath$''file_name$'.wav
writeInfoLine: "Extracting for ", soundPath$
sound = Read from file... 'soundPath$'

#appendInfoLine: "New sound id: ", sound
selectObject: sound

# generate MFCC
# To MFCC... : <coef count> <window length> <time step>
mfcc = To MFCC... 12 0.015 0.005 100 100 0.0
selectObject: mfcc

matrix = To Matrix
selectObject: matrix
columns = Get number of columns
rows = Get number of rows

appendInfoLine: "Size: ", rows, "x", columns

selectObject: matrix
writeInfoLine: "Saving to ", matrix_file$
Save as text file... 'matrix_file$'