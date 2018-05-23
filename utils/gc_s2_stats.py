filepath = '/media/stefan/Windows/all_sentinel_2_images/index.csv'

def stream_avg(prev_avg, x, n):
    return (prev_avg * n + x) / (n + 1)

all_values = []

with open(filepath) as fp:
    # skip header line
    line = fp.readline()
    print(line)
    cnt = 1
    valid = 0
    size_min = 10000
    size_max = 0
    avg = 0
    total = 0

    while line:
        if (cnt % 500000 == 0):
            print("Lines processed :", cnt)
        line = fp.readline()
        if not line:
            break
        items = [x.strip() for x in line.split(',')]
        #try:
            #print(items[5])
        if (items[5]):
            size_mb = int(items[5]) / (1024 * 1024)
            valid += 1
            size_min = min(size_min, size_mb)
            size_max = max(size_max, size_mb)
            avg = stream_avg(avg, size_mb, valid)
            total += size_mb
            all_values.append(size_mb)

        cnt += 1

    print("Lines processed :", cnt)

    print("\n---stats  :")
    print("Non-zero entries   :", valid)
    print("min (MB)           :", size_min)
    print("max (MB)           :", size_max)
    print("avg (MB)           :", avg)
    print("total (TB)         :", total / (1024 * 1024))

print("Sorting values ...")
all_values.sort()

print("Compute Histogram ...")
buckets_count = 100
buckets = []
x = []
for i in range(0, buckets_count):
    x.append((size_max / buckets_count) * i)
    buckets.append(0)
    print(x[i])
# add extra last bucket
x.append(size_max)
buckets.append(0)

for i in range(0, len(all_values)):
    # find bucket for all_values[i]
    j = 0
    while (x[j] < all_values[i] and j < buckets_count):
        j += 1
    buckets[j] += 1
#    bucket_index = i / buckets_count
#    buckets[bucket_index] += 1

with open("sizes.csv", "w") as o:
    for i in range(0, len(buckets)):
        o.write(str(x[i]) + "," + str(buckets[i]) + "\n")
