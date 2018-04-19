import numpy as np
import matplotlib.pyplot as plt

N = 6
#write = (862,327,225,130,106)
#fps = (215.6566745741,
#81.9739322895,
#56.4748404586,
#32.7396542693,
#26.6382525306,
#20.4353657092
#)

fps = (671.5916722633,
109.8659635245,
60.9793280078,
30.312215823,
16.168148747,
8.1510885779
)


#menStd = (20*cm, 30*cm, 32*cm, 10*cm, 20*cm)

fig, ax = plt.subplots(figsize=(3.5, 3))

#fig, ax = plt.subplots()

ind = np.arange(N)    # the x locations for the groups
width = 0.6         # the width of the bars
p1 = ax.bar(ind, fps, width, color='r', bottom=0)



#ax.set_title('Scores by group and gender')
ax.set_xticks(ind + width/2)
plt.xticks(ind, ('4 KB', '256 KB', '512 KB', '1 MB', '2 MB', '4 MB'))
#ax.set_xticklabels(('4 KB', '256 KB', '512 KB', '768 KB', '1 MB'))
plt.xticks(rotation=90)
ax.set_xticks(ind + width/2)

#ax.legend(p1[0], 'SGX-AONT Re-Key')
#ax.yaxis.set_units(inch)
ax.autoscale_view()

plt.grid()
#ax.set_ylim([0,300])
ax.set_xlabel('Block Size')
ax.set_ylabel('Re-Keys / Second')


plt.tight_layout()
plt.show()
