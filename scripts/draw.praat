form Draw
  comment I/O file paths
  sentence soundPath /home/ivo/SpeechSynthesis/corpus-test/Diana_A.1.1.Un2/Diana_A.1.1.Un2_0013.wav
  sentence outputPath /tmp/drawing.png
endform

start = 0
end = 0

if variableExists("startString$")
  start = number(startString$)
endif
if variableExists ("endString$")
  end = number(endString$)
endif

Erase all
Draw inner box
sound = Read from file... 'soundPath$'

#writeInfoLine: start, " ", end
Draw... start end 0 0 1 Curve

#Save as EPS file... outputPath$

appendInfoLine: "Drawing ", outputPath$
Save as 300-dpi PNG file... 'outputPath$'