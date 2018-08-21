# CLEAR zookeeper DB
/home/nuc17/stefan/zk/zookeeper-3.4.12/bin/zkCli.sh rmr /workers
/home/nuc17/stefan/zk/zookeeper-3.4.12/bin/zkCli.sh rmr /status
/home/nuc17/stefan/zk/zookeeper-3.4.12/bin/zkCli.sh rmr /assign
/home/nuc17/stefan/zk/zookeeper-3.4.12/bin/zkCli.sh rmr /tasks

# start master
# java -cp .:/home/nuc17/stefan/zk/zookeeper-3.4.12/dist-maven/zookeeper-3.4.12.jar:/home/nuc17/stefan/zk/zookeeper-3.4.12/build/lib/slf4j-api-1.7.25.jar:/home/nuc17/stefan//zookeeper-3.4.12/build/lib/slf4j-log4j12-1.7.25.jar:/home/nuc17/stefan/zk/zookeeper-3.4.12/build/lib/log4j-1.2.17.jar:/home/nuc17/stefan/zk/zookeeper-book-example/target/ZooKeeper-Book-0.0.1-SNAPSHOT.jar org.apache.zookeeper.book.Master localhost:2181

# start a couple of workers

# start the admin


