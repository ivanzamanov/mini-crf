form Cepstral Distance
   comment File Paths
   sentence sound_path_1 D:\cygwin\home\ivo\SpeechSynthesis\mini-crf\praat-scripts\concat-output.wav
   sentence sound_path_2 D:\cygwin\home\ivo\SpeechSynthesis\corpus-test\Diana_A.1.1.Un2\Diana_A.1.1.Un2.wav
endform

sounds[1] = Read from file... 'sound_path_1$'
sounds[2] = Read from file... 'sound_path_2$'

@create_cepstra

appendInfoLine: result

procedure create_cepstra
  selectObject: sounds[1]
  duration1 = Get total duration
  selectObject: sounds[2]
  duration2 = Get total duration

  frameWidth = 0.01
  totalDuration = min(duration1, duration2)
  frameCount = totalDuration / frameWidth
  #appendInfoLine: "Frames: ", frameCount
  for index to frameCount
    startTime = (index - 1) * frameWidth
    endTime = index * frameWidth
    for i to 2
      sound = sounds[i]
      selectObject: sound

      @create_single_cepstrum
    endfor

    @diff_cepstra

    for i to 2
      selectObject: cepstra[i]
      Remove
    endfor
  endfor
endproc

procedure create_single_cepstrum
  frame = Extract part for overlap... startTime endTime 0.01
  spectrum = To Spectrum... false
  cepstrum = To PowerCepstrum
  cepstra[i] = To Matrix

  selectObject: spectrum
  Remove
  selectObject: cepstrum
  Remove
  selectObject: frame
  Remove
endproc

procedure diff_cepstra
  selectObject: cepstra[1]
  xMax1 = Get number of columns
  yMax1 = Get number of rows
  selectObject: cepstra[2]
  xMax2 = Get number of columns
  yMax2 = Get number of rows

  xMax = min(xMax1, xMax2)
  yMax = min(yMax1, yMax2)

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
  result = max(result, dist)
  endfor
endproc
