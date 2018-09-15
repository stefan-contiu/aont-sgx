# prereq: libuv
wget tar -xvzf libuv-v1.23.0.tar.gz
cd libuv-v1.23.0/
sh autogen.sh
./configure --prefix=/usr --disable-static
make
sudo make install
cp /usr/lib/libuv.* /usr/lib/x86_64-linux-gnu/
cd ..

# cassandra
git pull https://github.com/datastax/cpp-driver.git
cd cpp-driver
cmake .
make all
sudo make install
cd ..
