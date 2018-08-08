all:
	g++ -std=c++17 client.cpp serialization.cpp storage.cpp crypto.cpp redis.cpp rest.cpp cass.cpp -g main.cpp  \
	-lssl -lcrypto -lm -lpthread -lcurl -lcassandra \
	-o cli-aont.o -lstdc++fs 

.PHONY: clean
clean:
	@rm -f *.o
