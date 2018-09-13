git clone https://github.com/intel/intel-sgx-ssl.git
cd intel-sgx-ssl/openssl_source
wget http://www.openssl.org/source/openssl-1.1.0e.tar.gz
echo Please add manually to file: SGX_SDK = /opt/intel/sgxsdk
cd ../Linux
pico sgx/buildenv.mk

#make all
#sudo make install


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


############################
# ZooKeeper

#git clone https://github.com/stefan-contiu/aont-sgx.git
#cd aont-sgx/server
#make SGX=1
