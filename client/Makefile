all:
	g++ -std=c++17 client.cpp serialization.cpp storage.cpp crypto.cpp redis.cpp rest.cpp -g main.cpp  \
	  /home/stefan/code/hiredis/libhiredis.a \
	  -L/home/stefan/code/hiredis/ \
	  -I/home/stefan/code/hiredis/ \
	-lgmp -lpbc -lssl -lcrypto -lm -lpthread -lcurl \
	-o cli-aont.o -lstdc++fs

.PHONY: clean
clean:
	@rm -f *.o
