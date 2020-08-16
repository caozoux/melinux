#make && scp *.ko root@vm:~/ && scp user_space/test root@vm:~/  && ssh root@vm "rmmod misc.ko  && insmod misc.ko && dmesg -C && ./test -k \"generic_make_request\""
make && scp *.ko root@vm:~/ && scp user_space/test root@vm:~/  && ssh root@vm "rmmod misc.ko  && insmod misc.ko"
