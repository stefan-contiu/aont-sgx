pkill aont_srv
./aont_srv worker w0 &
./aont_srv worker w1 &
./aont_srv worker w2 &
# give the workers time to subscribe before launching the re-key
sleep 3
./aont_srv master w0 w1 w2
# make sure no left-overs
pkill aont_srv
