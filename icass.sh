#############################
# Cassandra
#sudo apt-get install libuv-dev
git clone https://github.com/libuv/libuv.git
cd libuv
mkdir -p out/cmake ; cd out/cmake ; cmake -DBUILD_TESTING=ON ../..
sudo make all
sudo make install
cd ..

git pull https://github.com/datastax/cpp-driver.git
cd cpp-driver
cmake .
make all
sudo make install
cd ..
