import matplotlib.pyplot as plt

plt.rc('text', usetex=True)
#plt.rc('font', family='serif')

plt.ylabel(r'F')
plt.xlabel(r'$\theta + \delta$')

plt.plot(range(0, 100), range(0, 100))

plt.show()
