cd root 
ls 
make clean all
valgrind --track-fds=yes --read-var-info=yes ./bin/app files/am files/hi | ./bin/view
valgrind --track-fds=yes --read-var-info=yes ./bin/app files/am | ./bin/view
valgrind --track-fds=yes --read-var-info=yes ./bin/app files/am | ./bin/view
valgrind --track-fds=yes --read-var-info=yes ./bin/app2 files/am | ./bin/view
valgrind --track-fds=yes --read-var-info=yes ./bin/app files/am | ./bin/view
valgrind --track-fds=yes --read-var-info=yes ./bin/app files/am | ./bin/view
make clean all
valgrind --track-fds=yes --read-var-info=yes ./bin/app files/am | ./bin/view
valgrind --track-fds=yes --read-var-info=yes ./bin/app files/* | ./bin/view
make clean all
valgrind --track-fds=yes --read-var-info=yes ./bin/app files/* | ./bin/view
cd root
./bin/view /shm
make clean all 
./bin/view /shm
./bin/view /shm
./bin/view /shm
