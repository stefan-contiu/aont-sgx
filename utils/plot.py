import numpy as np
import matplotlib.pyplot as plt

N = 6
aont_write = (0.902013,
0.267091,
0.229204,
0.295415,
0.250181,
0.160769
)
aes_write = (0.728728,
0.149146,
0.125699,
0.123932,
0.117207,
0.078388
)
aont_read = (0.989852,
0.424975,
0.426671,
0.375194,
0.401011,
0.358725
)
aes_read = (0.805273,
0.295662,
0.206541,
0.247136,
0.280948,
0.238761
)

#menStd = (20*cm, 30*cm, 32*cm, 10*cm, 20*cm)

fig, ax = plt.subplots(figsize=(3.5, 3))

ind = np.arange(N)    # the x locations for the groups
width = 0.3         # the width of the bars
p1 = ax.bar(ind, aont_read, width, color='r', bottom=0)

#womenStd = (30*cm, 25*cm, 20*cm, 31*cm, 22*cm)
p2 = ax.bar(ind + width, aes_read, width,
            color='y', bottom=0,  hatch="/")

#ax.set_title('Scores by group and gender')
ax.set_xticks(ind + width)
ax.set_xticklabels(('4KB', '256KB', '512KB', '1MB', '2MB', '4MB'))
plt.xticks(rotation=90)

ax.legend((p1[0], p2[0]), ('SafeBox', 'Encrypt'), ncol=2)
#ax.yaxis.set_units(inch)
ax.autoscale_view()

plt.grid()
#ax.set_ylim([0,900])
ax.set_xlabel('Block Size')
ax.set_ylabel('Latency (s)')

plt.tight_layout()
plt.show()
