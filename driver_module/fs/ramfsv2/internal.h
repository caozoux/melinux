/* internal.h: ramfs internal definitions
 *
 * Copyright (C) 2005 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

struct ramfs_mount_opts {
	umode_t mode;
};
struct ramfs_fs_info {
	struct ramfs_mount_opts mount_opts;
};

#define RAMFS_DEFAULT_MODE	0755

extern const struct address_space_operations ramfs_aopsv2;
extern const struct inode_operations ramfs_file_inode_operationsv2;
extern const struct file_operations ramfs_file_operationsv2;
extern const struct super_operations ramfs_ops;
extern struct kmem_cache *ramfsv2_inode_cachep;
int ramfsv2_supper_init(void);
