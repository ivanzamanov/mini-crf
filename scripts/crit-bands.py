
HERTZ = [100, 200, 300, 400, 510, 630, 770, 920, 1080, 1270, 1480, 1720, 2000, 2320, 2700, 3150, 3700, 4400, 5300, 6400, 7700, 9500, 12000, 15500]

for h in HERTZ:
  period = 1.0 / h
  harmonic = h / (24000.0 / 512)

  print "Period: ", period
  print "Harmonic: ", harmonic
