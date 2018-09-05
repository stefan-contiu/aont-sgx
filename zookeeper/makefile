worker:
	g++  -I /usr/local/include/zookeeper/ -L /usr/local/lib/ worker.cpp -o worker.out -lzookeeper_st
admin:
	g++  -I /usr/local/include/zookeeper/ -L /usr/local/lib/ cass.cpp admin.cpp -o admin.out -lzookeeper_st -fopenmp -lcassandra
master:
	gcc -I /usr/local/include/zookeeper/ -I./  -L /usr/local/lib/ master.c -o master.out -lzookeeper_st

