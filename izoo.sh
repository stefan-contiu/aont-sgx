# prereq
sudo apt-get install libevent-dev

# zookeeper
wget http://mirrors.ircam.fr/pub/apache/zookeeper/stable/zookeeper-3.4.12.tar.gz
tar -xvzf zookeeper-3.4.12.tar.gz
cd zookeeper-3.4.12
cd src/c
./configure
make
sudo make install
cd ../..
