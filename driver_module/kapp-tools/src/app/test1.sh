./kapp-tool inject --rwsem --wrdown --msg "wrdown 1" &
./kapp-tool inject --rwsem --wrdown --msg "wrdown 2" &
./kapp-tool inject --rwsem --rddown --msg "rddown 1 cnt 3" &
./kapp-tool inject --rwsem --rddown --msg "rddown 2 cnt 4" &
./kapp-tool inject --rwsem --wrdown --msg "wrdown 4 cnt 6" &
./kapp-tool inject --rwsem --wrdown --msg "wrup 2" &
./kapp-tool inject --rwsem --wrdown --msg "wrup 1" &
