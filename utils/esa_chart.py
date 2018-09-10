import numpy as np
import matplotlib.pyplot as plt
from math import factorial

with open('sizes.csv') as f:
    lines = f.readlines()

x = []
y = []
for l in lines:
    items = l.split(',')
    x_val = float(items[0])
    y_val = float(items[1])
    x.append(x_val)
    y.append(y_val)
    #print(x_val, y_val)

fig, ax = plt.subplots(figsize=(5, 2.5))

plt.grid()
plt.plot(x, y)

ax.set_xlabel('File Size')
ax.set_ylabel('Files Count')

plt.xticks([0, 512, 1024, 1536], ("0", ".5 GB", "1 GB", "1.5 GB"))
ax.set_xlim(0, 1024 + 512)
plt.yticks([0, 50000, 100000, 150000], ("0", "5K", "100K", "150K"))
ax.set_ylim(0, 150000)

plt.tight_layout()
plt.show()
