form Feature Extraction
  sentence soundPath /home/ivo/SpeechSynthesis/corpus-synth/Diana_E.1.Un1/Diana_E.1.Un1_015.wav
endform

writeInfo: ""
sound = Read from file... 'soundPath$'

Resample... 16000 50

Save as WAV file... 'soundPath$'
