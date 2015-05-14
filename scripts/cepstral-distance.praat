form Cepstral Distance
     comment File Paths
     sentence sound_path_1 D:\cygwin\home\ivo\SpeechSynthesis\mini-crf\praat-scripts\concat-output.wav
     sentence sound_path_2 D:\cygwin\home\ivo\SpeechSynthesis\corpus-test\Diana_A.1.1.Un2\Diana_A.1.1.Un2.wav
endform

sounds[1] = Read from file... 'sound_path_1$'
sounds[2] = Read from file... 'sound_path_2$'

@create_cepstra

writeInfo: ""
appendInfoLine: result

procedure create_cepstra
    selectObject: sounds[1]
    duration1 = Get total duration
    selectObject: sounds[2]
    duration2 = Get total duration

    frameWidth = 0.005
    totalDuration = min(duration1, duration2)
    frameCount = totalDuration / frameWidth
    appendInfoLine: "Frames: ", frameCount
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
    selectObject: cepstra[2]
    xMax2 = Get number of columns

    xMax = min(xMax1, xMax2)

    result = 0
    for i to xMax
        dist = 0
        selectObject: cepstra[1]
        y1 = Get value in cell... 1 i

        selectObject: cepstra[2]
        y2 = Get value in cell... 1 i
        dist = dist + (y1 - y2) * (y1 - y2)
        result = result + dist
    endfor
endproc