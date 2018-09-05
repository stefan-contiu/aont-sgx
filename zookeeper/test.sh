# kill any leftovers
#pkill aont_srv
pkill admin.out
pkill master.out

# CLEAR zookeeper DB
/home/nuc17/stefan/zk/zookeeper-3.4.12/bin/zkCli.sh rmr /workers
/home/nuc17/stefan/zk/zookeeper-3.4.12/bin/zkCli.sh rmr /status
/home/nuc17/stefan/zk/zookeeper-3.4.12/bin/zkCli.sh rmr /assign
/home/nuc17/stefan/zk/zookeeper-3.4.12/bin/zkCli.sh rmr /tasks

# start master
java -cp .:/home/nuc17/stefan/zk/zookeeper-3.4.12/dist-maven/zookeeper-3.4.12.jar:/home/nuc17/stefan/zk/zookeeper-3.4.12/build/lib/slf4j-api-1.7.25.jar:/home/nuc17/stefan/zk/zookeeper-3.4.12/build/lib/slf4j-log4j12-1.7.25.jar:/home/nuc17/stefan/zk/zookeeper-3.4.12/build/lib/log4j-1.2.17.jar:/home/nuc17/stefan/zk/zookeeper-book-example/target/ZooKeeper-Book-0.0.1-SNAPSHOT.jar org.apache.zookeeper.book.Master localhost:2181
#sleep 5s

# start a couple of workers
#/home/nuc17/stefan/aont-sgx/server/aont_srv 127.0.0.1:2181 &
#seel 3s

# start the re-encrypt by admin
#./admin.out 127.0.0.1:2181

