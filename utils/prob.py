def p(n,w,k):
    r = 1.0
    for i in range(0, w):
        r = r * (k - i) / (n - i)
        #print(i, r)
    return r * 100

if __name__ == "__main__":
    n = 100
    for k in range(1, n+1):
        r1 = p(100, 1, k)
        r2 = p(100, 2, k)
        r3 = p(100, 3, k)
        r4 = p(100, 4, k)
        r5 = p(100, 5, k)
        print("%f,%f,%f,%f,%f" % (r1,r2,r3,r4,r5))
