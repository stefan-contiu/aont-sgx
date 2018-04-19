import numpy as np
import matplotlib.pyplot as plt
import codecs

x = []
c = 0
file_name = '/home/stefan/Downloads/files_size.txt'
with codecs.open(file_name, "r",encoding='utf-8', errors='ignore') as fp:
#with open('/home/stefan/Downloads/files_size.txt') as fp:
    #lines = fp.readlines()
    for line in fp:
        c = c + 1
        line = line.rstrip()
        skip = ['', 'rattach.docx', 'PRP', '2.pdf', 'VERSION', 'autres', 'May',
                'CT.pdf', 'and', 'Empirical', 'Transform.pdf', 'Higher-Order', 'most']
        if c%100000==0:
            print(c)
        if (line not in skip):
            #print(line)
            v = float(line.rstrip())/(1024*1024)
            if v <= 10:
                x.append(v)
        #if c==1000:
        #    break

print('Lines in total', c)
#x = mu + sigma * np.random.randn(10000)

# the histogram of the data
n, bins, patches = plt.hist(x, 200, log=True)

print(n)
print(bins)

plt.xlabel('File Sizes (MB)')
plt.ylabel('Count')
plt.title('Histogram of File Sizes')
#plt.text(60, .025, r'$\mu=100,\ \sigma=15$')
#plt.axis([40, 160, 0, 0.03])
plt.grid(True)
plt.show()
