#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include "ext4.h"

static int dump_level = 0;
static void dump_ext4_super(char *ext4_img)
{
	int fd;
	struct ext2_super_block  super;
	int size;

	memset((void*)&super, 0, sizeof(struct ext2_super_block));
	fd = open(ext4_img, O_RDONLY);
	if (fd < 0) {
		printf("open %s failed\n", ext4_img);
		return;
	}
	lseek(fd, 1024, SEEK_SET);
	size = read(fd, &super, sizeof(struct ext2_super_block));
	close(fd);

	printf("Inodes count s_inodes_count:%d\n", super.s_inodes_count);
	printf("Blocks count s_blocks_count:%d\n", super.s_blocks_count);
	printf("Reserved blocks count s_r_blocks_count:%x\n", super.s_r_blocks_count);
	printf("Free blocks count s_free_blocks_count:%d\n", super.s_free_blocks_count);
	printf("Free inodes count s_free_inodes_count:%d\n", super.s_free_inodes_count);
	printf("First Data Block s_first_data_block:%d\n", super.s_first_data_block);
	printf("Block size s_log_block_size:%d\n", super.s_log_block_size);
	printf("Allocation cluster size s_log_cluster_size:%d\n", super.s_log_cluster_size);
	printf("Blocks per group s_blocks_per_group:%d\n", super.s_blocks_per_group);
	printf("Fragments per group s_clusters_per_group:%d\n", super.s_clusters_per_group);
	printf("Inodes per group s_inodes_per_group:%d\n", super.s_inodes_per_group);
	printf("Mount count s_mnt_count:%d\n", super.s_mnt_count);
	printf("Maximal mount count s_max_mnt_count:%d\n", super.s_max_mnt_count);
	printf("File system state s_state:%x\n", super.s_state);
	printf("size of inode structure s_inode_size:%d\n", super.s_inode_size);
	printf("block group # of this superblock s_block_group_nr:%d\n", super.s_block_group_nr);
	printf("inode number of journal file s_journal_inum:%x\n", super.s_journal_inum);
	printf("device number of journal file s_journal_dev:%x\n", super.s_journal_dev);
	printf("start of list of inodes to delete s_last_orphan:%x\n", super.s_last_orphan);
	printf("Group desc size: INCOMPAT_64BIT s_desc_size:%d\n", super.s_desc_size);
	printf("First metablock group s_first_meta_bg:%x\n", super.s_first_meta_bg);
	printf("Backup of the journal inode s_jnl_blocks:%ld\n", (unsigned long)super.s_jnl_blocks);
	printf("Blocks count high 32bits s_blocks_count_hi:%x\n", super.s_blocks_count_hi);
	printf("Free blocks count s_free_blocks_hi:%x\n", super.s_free_blocks_hi);
	printf("Inode number of active snapshot s_snapshot_inum:%x\n", super.s_snapshot_inum);
	printf("sequential ID of active snapshot s_snapshot_id:%x\n", super.s_snapshot_id);
	printf("inode number of disk snapshot list s_snapshot_list:%x\n", super.s_snapshot_list);
	if (dump_level > 1) {
		printf("Mount time s_mtime:%x\n", super.s_mtime);
		printf("Write time s_wtime:%x\n", super.s_wtime);
		printf("Magic signature s_magic:%x\n", super.s_magic);
		printf("Behaviour when detecting errors s_errors:%x\n", super.s_errors);
		printf("minor revision level s_minor_rev_level:%x\n", super.s_minor_rev_level);
		printf("time of last check s_lastcheck:%x\n", super.s_lastcheck);
		printf("max. time between checks s_checkinterval:%x\n", super.s_checkinterval);
		printf("OS s_creator_os:%x\n", super.s_creator_os);
		printf("Revision level s_rev_level:%x\n", super.s_rev_level);
		printf("Default uid for reserved blocks s_def_resuid:%x\n", super.s_def_resuid);
		printf("Default gid for reserved blocks s_def_resgid:%x\n", super.s_def_resgid);
		printf("First non-reserved inode s_first_ino:%d\n", super.s_first_ino);
		printf("compatible feature set s_feature_compat:%x\n", super.s_feature_compat);
		printf("incompatible feature set s_feature_incompat:%x\n", super.s_feature_incompat);
		printf("readonly-compatible feature set s_feature_ro_compat:%x\n", super.s_feature_ro_compat);
		printf("128-bit uuid for volume s_uuid:%lx\n", (unsigned long)super.s_uuid);
		printf("volume name no NUL s_volume_name:%lxx\n", (unsigned long)super.s_volume_name);
		printf("directory last mounted on NULL s_last_mounted:%lx\n", (unsigned long)super.s_last_mounted);
		printf("For compression s_algorithm_usage_bitmap:%lx\n", (unsigned long)super.s_algorithm_usage_bitmap);
		printf("Nr of blocks to try to preallocate s_prealloc_blocks:%d\n", super.s_prealloc_blocks);
		printf("Nr to preallocate for dirs s_prealloc_dir_blocks:%d\n", super.s_prealloc_dir_blocks);
		printf("Per group table for online growth s_reserved_gdt_blocks:%x\n", super.s_reserved_gdt_blocks);
		printf("uuid of journal superblock s_journal_uuid:%lx\n", (unsigned long)super.s_journal_uuid);
		printf("HTREE hash seed s_hash_seed:%lx\n", (unsigned long)super.s_hash_seed);
		printf("Default hash version to use s_def_hash_version:%x\n", super.s_def_hash_version);
		printf("Default type of journal backup s_jnl_backup_type:%x\n", super.s_jnl_backup_type);
		printf("s_wtime_hi:%x\n", super.s_wtime_hi);
		printf("s_mtime_hi:%x\n", super.s_mtime_hi);
		printf("s_mkfs_time_hi:%x\n", super.s_mkfs_time_hi);
		printf("s_lastcheck_hi:%x\n", super.s_lastcheck_hi);
		printf("s_first_error_time_hi:%x\n", super.s_first_error_time_hi);
		printf("s_last_error_time_hi:%x\n", super.s_last_error_time_hi);
		printf("Filename charset encoding s_encoding:%x\n", super.s_encoding);
		printf("Filename charset encoding flags s_encoding_flags:%x\n", super.s_encoding_flags);
		printf("Padding to the end of the block s_reserved:%lx\n", (unsigned long)super.s_reserved);
		printf("crc32c(superblock) s_checksum:%x\n", super.s_checksum);
		printf("overhead blocks/clusters in fs s_overhead_clusters:%x\n", super.s_overhead_clusters);
		printf("If sparse_super2 enabled s_backup_bgs:%lx\n", (unsigned long)super.s_backup_bgs);
		printf("Encryption algorithms in use  s_encrypt_algos:%lx\n", (unsigned long)super.s_encrypt_algos);
		printf("Salt used for string2key algorithm s_encrypt_pw_salt:%lx\n", (unsigned long)super.s_encrypt_pw_salt);
		printf("Location of the lost+found inode s_lpf_ino:%x\n", super.s_lpf_ino);
		printf("inode for tracking project quota s_prj_quota_inum:%x\n", super.s_prj_quota_inum);
		printf("crc32c(orig_uuid) if csum_seed set s_checksum_seed:%x\n", super.s_checksum_seed);
		printf("default EXT2_MOUNT_ s_default_mount_opts:%x\n", super.s_default_mount_opts);
		printf("When the filesystem was created s_mkfs_time:%x\n", super.s_mkfs_time);
		printf("All inodes have at least # bytes s_min_extra_isize:%x\n", super.s_min_extra_isize);
		printf("New inodes should reserve # bytes s_want_extra_isize:%x\n", super.s_want_extra_isize);
		printf("Miscellaneous flags s_flags:%x\n", super.s_flags);
		printf("RAID stride in blocks s_raid_stride:%x\n", super.s_raid_stride);
		printf("# seconds to wait in MMP checking s_mmp_update_interval:%x\n", super.s_mmp_update_interval);
		printf("Block for multi-mount protection s_mmp_block:%lx\n", (unsigned long)super.s_mmp_block);
		printf("blocks on all data disks (N s_raid_stripe_width:%x\n", super.s_raid_stripe_width);
		printf("FLEX_BG group size s_log_groups_per_flex:%x\n", super.s_log_groups_per_flex);
		printf("metadata checksum algorithm s_checksum_type:%x\n", super.s_checksum_type);
		printf("versioning level for encryption s_encryption_level:%x\n", super.s_encryption_level);
		printf("Padding to next 32bits s_reserved_pad:%x\n", super.s_reserved_pad);
		printf("nr of lifetime kilobytes written s_kbytes_written:%ld\n", (unsigned long)super.s_kbytes_written);
		printf("line number where error happened s_last_error_line:%x\n", super.s_last_error_line);
		printf("block involved of last error s_last_error_block:%lx\n", (unsigned long)super.s_last_error_block);
		printf("function where error hit no NUL? s_last_error_func:%lx\n", (unsigned long)super.s_last_error_func);
		printf("default mount options no NUL? s_mount_opts:%lx\n", (unsigned long)super.s_mount_opts);
		printf("inode number of user quota file s_usr_quota_inum:%x\n", super.s_usr_quota_inum);
		printf("inode number of group quota file s_grp_quota_inum:%x\n", super.s_grp_quota_inum);
		printf("Reserved blocks count high 32 bits s_r_blocks_count_hi:%x\n", super.s_r_blocks_count_hi);
		printf("number of fs errors s_error_count:%x\n", super.s_error_count);
		printf("first time an error happened s_first_error_time:%x\n", super.s_first_error_time);
		printf("inode involved in first error s_first_error_ino:%x\n", super.s_first_error_ino);
		printf("block involved in first error s_first_error_block:%lx\n", (unsigned long)super.s_first_error_block);
		printf("function where error hit no NUL? s_first_error_func:%lx\n", (unsigned long)super.s_first_error_func);
		printf("line number where error happened s_first_error_line:%x\n", super.s_first_error_line);
		printf("most recent time of an error s_last_error_time:%x\n", super.s_last_error_time);
		printf("inode involved in last error s_last_error_ino:%x\n", super.s_last_error_ino);
		printf("active snapshot reserved blocks s_snapshot_r_blocks_count:%lx\n", (unsigned long)super.s_snapshot_r_blocks_count);
	}
}

int main(int argc, char *argv[])
{
	dump_ext4_super("/Users/zoucao-ipc/Downloads/ext4.data");
	return 0;
}
