# prereq: libuv
wget https://dist.libuv.org/dist/v1.23.0/libuv-v1.23.0.tar.gz
tar -xvzf libuv-v1.23.0.tar.gz
cd libuv-v1.23.0/
sh autogen.sh
./configure --prefix=/usr --disable-static
make
sudo make install
sudo cp /usr/lib/libuv.* /usr/lib/x86_64-linux-gnu/
cd ..

# cassandra
git clone https://github.com/datastax/cpp-driver.git
cd cpp-driver
sudo apt install cmake
cmake .
make all
sudo make install
cd ..
sudo cp /usr/local/lib/x86_64-linux-gnu/libcassandra.so* /usr/lib/x86_64-linux-gnu/
