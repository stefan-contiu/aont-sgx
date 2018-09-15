git clone https://github.com/intel/intel-sgx-ssl.git
cd intel-sgx-ssl/openssl_source
wget http://www.openssl.org/source/openssl-1.1.0e.tar.gz
echo Please add manually to file: SGX_SDK = /opt/intel/sgxsdk
cd ../Linux
pico sgx/buildenv.mk

echo Please execute make all && sudo make install
#make all
#sudo make install


