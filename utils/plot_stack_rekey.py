import numpy as np
import matplotlib.pyplot as plt


N = 6

misc_r = (0.001312,
0.002856,
0.00478,
0.008714,
0.013916,
0.027986
)
storage_r = (0.000177,
0.006246,
0.011619,
0.024276,
0.047934,
0.094697
)

misc = misc_r
storage = storage_r

misc_label = 'Misc.' #'Misc (SHA, PK)'
title = 'Breakdown Safe-Box Upload' # 'Breakdown Safe-Box Upload'

fig, ax = plt.subplots(figsize=(3.5, 3))


ind = np.arange(N)    # the x locations for the groups
width = 0.35*2       # the width of the bars: can also be len(x) sequence

p1 = plt.bar(ind, misc, width, color='r')
p2 = plt.bar(ind, storage, width, color='y',
             bottom=misc)
#p3 = plt.bar(ind, aes, width, color='b',
#            bottom=[i+j for i,j in zip(misc, storage)])

plt.ylabel('Latency (s)')
plt.xlabel('Block Size')
#plt.title(title)

plt.xticks(ind, ('4 KB', '256 KB', '512 KB', '1 MB', '2 MB', '4 MB'))
plt.xticks(rotation=90)
ax.set_xticks(ind + width/2)

#plt.yticks(np.arange(0, 81, 10))
plt.legend((p1[0], p2[0]), ("SGX", 'I/O'), ncol=2)
plt.grid()
plt.tight_layout()

axes = plt.gca()
#axes.set_ylim([0,1])

plt.show()
