form Cepstral Distance
     comment File Paths
     sentence sound_path_1 .
     sentence sound_path_2 .
endform

sounds[1] = Read from file... 'sound_path_1$'
sounds[2] = Read from file... 'sound_path_2$'

@create_cepstrums

@diff_matrices

procedure create_cepstrums
    for i to 2
        sound = sounds[i]
        selectObject: sound

        spectrum = To Spectrum... false
        cepstrum = To PowerCepstrum
        matrix = To Matrix

        matrices[i] = matrix

        selectObject: cepstrum
        Remove
        selectObject: spectrum
        Remove
        #selectObject: sound
        #Remove
    endfor
endproc

procedure diff_matrices
    selectObject: matrices[1]
    xMax1 = Get number of columns
    selectObject: matrices[2]
    xMax2 = Get number of columns

    xMax = min(xMax1, xMax2)
    result = 0
    for i to xMax
        selectObject: matrices[1]
        y1 = Get value in cell... 1 i

        selectObject: matrices[2]
        y2 = Get value in cell... 1 i

        result = result + (y1 - y2) * (y1 - y2)
    endfor

    appendInfoLine: result
endproc
