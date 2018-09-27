workers=("192.168.1.109" "192.168.1.104" "192.168.1.107" "192.168.1.108" "192.168.1.110")

update_git() {
  for i in $(seq 0 $1); do
    echo UPDATE $i on ${workers[$i]}
    ssh nuc@"${workers[$i]}" "cd /home/nuc/stefan/aont-sgx/ && git pull origin master && cd client && make"
    #ssh nuc@"${workers[$i]}" "cd /home/nuc/stefan/aont-sgx/server && make SGX=1"
  done
}

generate_micro_load () {
  files_count=500

  # first call clears the db, then only append
  
  /home/nuc17/stefan/aont-sgx/client/cli-aont.o -macro a17 $files_count 1024 256 $2 &
  sleep 5s

  for i in $(seq 0 $1); do
    echo GENERATING LOAD $i on ${workers[$i]}
    ssh -f nuc@"${workers[$i]}" "cd /home/nuc/stefan/aont-sgx/client/ && ./cli-aont.o -append a$i $files_count 1024 256 $2"
    ssh -f nuc@"${workers[$i]}" "cd /home/nuc/stefan/aont-sgx/client/ && ./cli-aont.o -append b$i $files_count 1024 256 $2"
    ssh -f nuc@"${workers[$i]}" "cd /home/nuc/stefan/aont-sgx/client/ && ./cli-aont.o -append c$i $files_count 1024 256 $2"
    ssh -f nuc@"${workers[$i]}" "cd /home/nuc/stefan/aont-sgx/client/ && ./cli-aont.o -append d$i $files_count 1024 256 $2"
  done
 
  /home/nuc17/stefan/aont-sgx/client/cli-aont.o -append b17 $files_count 1024 256 $2 &
  /home/nuc17/stefan/aont-sgx/client/cli-aont.o -append c17 $files_count 1024 256 $2 &
  /home/nuc17/stefan/aont-sgx/client/cli-aont.o -append d17 $files_count 1024 256 $2
  sleep 5s
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
  java -cp .:/home/nuc17/stefan/zk/zookeeper-3.4.12/dist-maven/zookeeper-3.4.12.jar:/home/nuc17/stefan/zk/zookeeper-3.4.12/build/lib/slf4j-api-1.7.25.jar:/home/nuc17/stefan/zk/zookeeper-3.4.12/build/lib/slf4j-log4j12-1.7.25.jar:/home/nuc17/stefan/zk/zookeeper-3.4.12/build/lib/log4j-1.2.17.jar:/home/nuc17/stefan/zk/zookeeper-book-example/target/ZooKeeper-Book-0.0.1-SNAPSHOT.jar org.apache.zookeeper.book.Master 192.168.1.102:2181 &
  sleep 5s
}


function machine_workers {
  ssh nuc@$1 pkill aont_srv
  for i in $(seq 1 $2); do
    echo Starting worker... $i on $1
    ssh -f nuc@$1 "cd /home/nuc/stefan/aont-sgx/server && ./aont_srv 192.168.1.102:2181"
    sleep 1s
  done
}

function reset_workers {
  for i in $(seq 1 $1); do
    machine_workers "${workers[$i]}" 2
  done
}

start_admin () {
  # write to output
  cd /home/nuc17/stefan/aont-sgx/zookeeper && ./admin.out 192.168.1.102:2181 > $1
}

#### MAIN SCRIPT
#update_git 5
for super in 1 #2 3
do
  echo "Testing with super blocks: $super"
#  generate_micro_load 5 $super
  for w in 1 #2 3 4 5
  do
    echo "Machine(s) with workers : $w"
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
