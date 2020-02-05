#ifndef __RAMFSV2_INODE_H__
#define __RAMFSV2_INODE_H__
struct ramfsv2_inode_info {
	unsigned long flag;
	struct inode vfs_inode;
};
#endif

