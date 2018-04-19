import numpy as np
import matplotlib.pyplot as plt


N = 6
misc_w = (0.280143,
0.186571,
0.159968,
0.201557,
0.182317,
0.120936
)
storage_w = (0.62187,
0.08052,
0.069236,
0.093858,
0.067864,
0.039833
)
aes_w = (0.062472,
0.020291,
0.01719,
0.022216,
0.022299,
0.020452
)

misc_r = (0.188863,
0.14364,
0.155044,
0.126416,
0.151164,
0.114147
)
storage_r = (0.783054,
0.268787,
0.263342,
0.241141,
0.242798,
0.237552
)
aes_r = (0.017935,
0.012548,
0.008285,
0.007637,
0.007049,
0.007026
)

#
# =============================================================================
# =============================================================================
#

a_misc_w = (0.08754,
0.026924,
0.026193,
0.020162,
0.020276,
0.018275
)
a_storage_w = (0.577398,
0.100541,
0.079192,
0.084815,
0.076458,
0.04508
)
a_aes_w = (0.06379,
0.021681,
0.020314,
0.018955,
0.020473,
0.015033
)

a_misc_r = (0.064257,
0.015353,
0.008579,
0.009311,
0.014242,
0.012661
)
a_storage_r = (0.687311,
0.262005,
0.185549,
0.223935,
0.251493,
0.213034
)
a_aes_r = (0.053705,
0.018304,
0.012413,
0.01389,
0.015213,
0.013066
)

misc = a_misc_r
storage = a_storage_r
aes = a_aes_r
misc_label = 'Misc.' #'Misc (SHA, PK)'
title = 'Breakdown Safe-Box Upload' # 'Breakdown Safe-Box Upload'

fig, ax = plt.subplots(figsize=(3.5, 3))


ind = np.arange(N)    # the x locations for the groups
width = 0.35*2       # the width of the bars: can also be len(x) sequence

p1 = plt.bar(ind, misc, width, color='r')
p2 = plt.bar(ind, storage, width, color='y',
             bottom=misc)
p3 = plt.bar(ind, aes, width, color='b',
            bottom=[i+j for i,j in zip(misc, storage)])

plt.ylabel('Latency (s)')
plt.xlabel('Block Size')
#plt.title(title)

plt.xticks(ind, ('4 KB', '256 KB', '512 KB', '1 MB', '2 MB', '4 MB'))
plt.xticks(rotation=90)
ax.set_xticks(ind + width/2)

#plt.yticks(np.arange(0, 81, 10))
plt.legend((p1[0], p2[0], p3[0]), (misc_label, 'I/O', 'AES'), ncol=3)
plt.grid()
plt.tight_layout()

axes = plt.gca()
axes.set_ylim([0,1])

plt.show()
