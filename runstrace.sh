make clean
make
strace ./bin/app files/* | ./bin/view > out.txt