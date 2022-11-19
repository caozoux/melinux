[root@localhost app]# ./kapp-tool inject --rwsem --wrdown --msg "wrdown 1" &
[root@localhost app]# ./kapp-tool inject --rwsem --wrdown --msg "wrdown 2" &
[root@localhost app]# ./kapp-tool inject --rwsem --rddown --msg "rddown 1 cnt 3" &
[root@localhost app]# ./kapp-tool inject --rwsem --rddown --msg "rddown 2 cnt 4" &
[root@localhost app]# ./kapp-tool inject --rwsem --wrdown --msg "wrdown 4 cnt 6" &
[root@localhost app]# ./kapp-tool inject --rwsem --wrdown --msg "rddown 3 cnt 7" &
[root@localhost app]# ./kapp-tool inject --rwsem --wrdown --msg "rddown 3 wrdown 3 cnt 5

