sudo umount /mnt/ && sudo rmmod ext4_test && sudo rmmod nvme_test && sudo dmesg -C && sudo insmod /home/zoucao/github/melinux/driver_module/fs/ext4/ext4_test.ko  && sudo insmod /home/zoucao/github/melinux/driver_module/block/nvme/nvme_4.19/nvme/nvme_test.ko && sudo mount -t ext4v2 /dev/nvme0n1 /mnt/
