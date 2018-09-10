import numpy as np
import matplotlib.pyplot as plt
from math import factorial

w = [8, 16, 24, 32]
se1 = [33, 28, 25, 24]
se2 = [35, 30, 27, 26]
se3 = [38, 33, 29, 28]

fig, ax = plt.subplots(figsize=(5, 2.5))

plt.grid()
plt.plot(se1,)
plt.plot(se2)
plt.plot(se3)

ax.set_xlabel('Workers Count')
ax.set_ylabel('Total Time (s)')

plt.xticks([0, 1, 2, 3], w)
ax.set_xlim(0, 3)
#plt.yticks([0, 50000, 100000, 150000], ("0", "5K", "100K", "150K"))
ax.set_ylim(0, 40)

plt.tight_layout()
plt.show()
