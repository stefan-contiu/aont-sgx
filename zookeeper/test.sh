

donegenerate_micro_load () {
  /home/nuc17/stefan/aont-sgx/client/cli-aont.o -generate $1
}

reset_zk () {
  # stop master
  pkill -9 -f zookeeper.book.Master
  pkill admin.out

  # CLEAR zookeeper DB
  /home/nuc17/stefan/zk/zookeeper-3.4.12/bin/zkCli.sh rmr /master
  /home/nuc17/stefan/zk/zookeeper-3.4.12/bin/zkCli.sh rmr /status
  /home/nuc17/stefan/zk/zookeeper-3.4.12/bin/zkCli.sh rmr /assign
  /home/nuc17/stefan/zk/zookeeper-3.4.12/bin/zkCli.sh rmr /tasks
  /home/nuc17/stefan/zk/zookeeper-3.4.12/bin/zkCli.sh rmr /workers

  # start master
  java -cp .:/home/nuc17/stefan/zk/zookeeper-3.4.12/dist-maven/zookeeper-3.4.12.jar:/home/nuc17/stefan/zk/zookeeper-3.4.12/build/lib/slf4j-api-1.7.25.jar:/home/nuc17/stefan/zk/zookeeper-3.4.12/build/lib/slf4j-log4j12-1.7.25.jar:/home/nuc17/stefan/zk/zookeeper-3.4.12/build/lib/log4j-1.2.17.jar:/home/nuc17/stefan/zk/zookeeper-book-example/target/ZooKeeper-Book-0.0.1-SNAPSHOT.jar org.apache.zookeeper.book.Master localhost:2181 &
  sleep 5s
}

function reset_workers {
  pkill aont_srv
  for i in $(seq 1 $1); do
    echo Starting worker...
    cd /home/nuc17/stefan/aont-sgx/server && ./aont_srv 127.0.0.1:2181 & sleep 1s
  done
}

start_admin () {
  # write to output
  cd /home/nuc17/stefan/aont-sgx/zookeeper && ./admin.out 127.0.0.1:2181 > $1
}

#### MAIN SCRIPT
for super in 1 2 3
do
  echo "Testing with super blocks: $super"
  #generate_micro_load $super
  for w in 2 4 8 
  do
    reset_zk
    reset_workers "$w"
    file_name="out"
    file_name="$file_name-$super-"
    file_name="$file_name$w"
    start_admin "results/$file_name"
  done
done

#### PLOT CHART
# todo : ...
