#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/file.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/iomap.h>
#include <linux/kallsyms.h>
#include <linux/miscdevice.h>

#define REQ_REMAP (1UL<<24)

#define SIMP_BLKDEV_DISKNAME "simp_blkdev"          //块设备名
#define SIMP_BLKDEV_DEVICEMAJOR COMPAQ_SMART2_MAJOR //主设备号
#define SIMP_BLKDEV_BYTES (50*1024*1024)            // 块设备大小为50MB
#define SECTOR_SIZE_SHIFT 9

#define LOOKUP_SYMS(name) do {							\
		orig_##name = (void *)kallsyms_lookup_name(#name);		\
		if (!orig_##name) {						\
			pr_err("kallsyms_lookup_name: %s\n", #name);		\
			return -EINVAL;						\
		}								\
	} while (0)

#define MISC_NAME "inode_blkdev"

typedef struct {
	unsigned long lg_offset;
	unsigned long br_start;
	unsigned long br_block;
} block_map_data;

typedef struct {
	char *disk_name;
	int disk_name_len;

	char *file_path;
	int file_path_len;

	block_map_data *map_data;
	int map_cnt;

	struct file *filp;
	struct gendisk *blkdev_disk;
	struct request_queue *blkdev_queue;
	struct request_queue *tag_blkdev_queue;

	unsigned long disk_size;

} blk_disk_data;

typedef loff_t (*iomap_actor_t)(struct inode *inode, loff_t pos, loff_t len,
		void *data, struct iomap *iomap);

static struct gendisk *simp_blkdev_disk;// gendisk结构表示一个简单的磁盘设备
static struct block_device_operations simp_blkdev_fops = { //块设备操作，gendisk的一个属性
    .owner = THIS_MODULE,
};
static struct request_queue *simp_blkdev_queue;//指向块设备请求队列的指针
unsigned char simp_blkdev_data[SIMP_BLKDEV_BYTES];// 虚拟磁盘块设备的存储空间
static struct request_queue *tag_queue;//指向块设备请求队列的指针

static struct bio_set split_bs;
static struct inode *disk_inode;

static blk_qc_t (*orig_blk_mq_submit_bio)(struct request_queue *q, struct bio *bio);
loff_t
(*orig_iomap_apply)(struct inode *inode, loff_t pos, loff_t length, unsigned flags,
		struct iomap_ops *ops, void *data, iomap_actor_t actor);
static int blk_inode_iomap(blk_disk_data *disk_data, struct inode * inode);

struct iomap_ops *orig_xfs_iomap_ops;

block_map_data *test_block_map;

static loff_t
iomap_bmap_actor(struct inode *inode, loff_t pos, loff_t length, void *data,
						struct iomap *iomap)
{
	return 0;
}

//static int blk_inode_iomap(struct inode * inode, loff_t pos, loff_t length, unsigned long *blkio_map)
static int blk_inode_iomap(blk_disk_data *disk_data, struct inode * inode)
{
	struct iomap iomap = { 0 };
	loff_t written = 0, ret;
	loff_t total_size = inode->i_size;
	loff_t length = total_size;
	loff_t offset = 0;
	int cnt = 0;


	while(offset < total_size) {
		ret = orig_xfs_iomap_ops->iomap_begin(inode, offset, length, 0, &iomap);
		if (ret)
			return ret;

		if (WARN_ON(iomap.offset > offset))
			return -EIO;

		if (WARN_ON(iomap.length == 0))
			return -EIO;

		/*
		 * Cut down the length to the one actually provided by the filesystem,
		 * as it might not be able to give us the whole size that we requested.
		 */
		if (iomap.offset + iomap.length < offset + length)
			length = iomap.offset + iomap.length - offset;

		/*
		 * Now the data has been copied, commit the range we've copied.  This
		 * should not fail unless the filesystem has had a fatal error.
		 */
		if (orig_xfs_iomap_ops->iomap_end) {
			ret = orig_xfs_iomap_ops->iomap_end(inode, offset, length,
						 written > 0 ? written : 0,
						 0, &iomap);
		}

		disk_data->map_data[cnt].lg_offset = offset>>9;
		disk_data->map_data[cnt].br_start = iomap.addr>>9;
		disk_data->map_data[cnt].br_block = iomap.length>>9;
		printk("lg_offset:%lx br_start:%lx br_block:%lx \n",
				disk_data->map_data[cnt].lg_offset,
				disk_data->map_data[cnt].br_start,
				disk_data->map_data[cnt].br_block
				);
		cnt++;

		offset += iomap.length;
		length = total_size - offset ;
	}

	return cnt;
}

static int hook_block_tag(blk_disk_data *disk_data)
{
	struct file  *filp = NULL;
	struct inode        *f_inode;
	struct super_block  *i_sb;
	struct block_device *s_bdev;
 	struct gendisk * bd_disk;
	struct request_queue *  bd_queue;
	int cnt;

	//filp = filp_open("/mnt/data",O_RDONLY | O_LARGEFILE, 0);
	//filp = filp_open(disk_data->file_path, O_RDONLY | O_LARGEFILE, 0);
	filp = filp_open("/mnt/data", O_RDONLY | O_LARGEFILE, 0);

	if (IS_ERR(filp)) {
		printk("err open file failed\n");
		return -EINVAL;
	}

	f_inode = filp->f_inode;
	i_sb = f_inode->i_sb;
	s_bdev = i_sb->s_bdev;
	bd_disk = s_bdev->bd_disk;
	bd_queue = s_bdev->bd_queue;

	disk_inode = f_inode;

	cnt = blk_inode_iomap(disk_data, f_inode);
	if (!cnt) {
		goto error;
	}

	tag_queue = bd_disk->queue;
	disk_data->tag_blkdev_queue = tag_queue;

	disk_data->disk_size = f_inode->i_size;
	disk_data->filp = filp;
	bd_disk->queue->queuedata = disk_data;

    set_capacity(disk_data->blkdev_disk, f_inode->i_size>>9);
	return 0;
error:
	fput(filp);
	return -EINVAL;
}

static blk_qc_t simp_blkdev_do_request(struct request_queue *q, struct bio *bio)
{
	int i;
	struct bio *split_bio;

	if (bio->bi_opf &  REQ_REMAP)
		return orig_blk_mq_submit_bio(tag_queue, bio);

	for (i=0; i < 5; i++) {
		if (bio->bi_iter.bi_sector >= test_block_map[i].lg_offset
				&& bio->bi_iter.bi_sector <  (test_block_map[i].lg_offset + test_block_map[i].br_block))
		{
#if 0
			unsigned long last_sect = bio->bi_iter.bi_sector + bio->bi_iter.bi_size/512;
			if (last_sect > (test_block_map[i].lg_offset + test_block_map[i].br_block)) {

				unsigned long fornt_sec =  bio->bi_iter.bi_size/512 -  (last_sect - (test_block_map[i].lg_offset + test_block_map[i].br_block));
				split_bio = bio_split(bio, fornt_sec, GFP_NOIO, &split_bs);
				bio_chain(split_bio, bio);
				split_bio->bi_iter.bi_sector =  (split_bio->bi_iter.bi_sector - test_block_map[i].lg_offset) + test_block_map[i].br_start ;

				orig_blk_mq_submit_bio(tag_queue, split_bio);

				continue;
			}
#endif

			bio->bi_iter.bi_sector =  (bio->bi_iter.bi_sector - test_block_map[i].lg_offset) + test_block_map[i].br_start ;
			bio->bi_opf |= REQ_REMAP;
			return orig_blk_mq_submit_bio(tag_queue, bio);
		}
	}

	printk("zz %s error sec:%lx \n",__func__, (unsigned long)bio->bi_iter.bi_sector);
	bio_io_error(bio);
	return BLK_QC_T_NONE;
}

static int misc_template_open(struct inode *inode, struct file * file)
{
	return 0;
}

static ssize_t misc_template_read(struct file *file, char __user * buf, size_t size, loff_t *ppos)
{
	return 0;
}

static ssize_t misc_template_write(struct file *file, const char __user * buf, size_t size, loff_t *ppos)
{
	return size;
}

static int misc_template_release (struct inode *inode, struct file *file)
{
	return 0;
}

static long misc_template_unlocked_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
#if 0

	int ret = 0;

	if (copy_to_user((char __user *) arg, &data, sizeof(struct ping_data))) {
		ret = -EFAULT;
		goto OUT;
	}

OUT:
	return ret;
#else
	return 0;
#endif
}

struct file_operations misc_temp_ops = {
	.open = misc_template_open,
	.read = misc_template_read,
	.write = misc_template_write,
	.unlocked_ioctl =misc_template_unlocked_ioctl,
	.release = misc_template_release
};

struct miscdevice  misc_dev = {
	.name = MISC_NAME,
	.fops = &misc_temp_ops,
};

static int sym_init(void)
{
	LOOKUP_SYMS(blk_mq_submit_bio);
	LOOKUP_SYMS(iomap_apply);
	LOOKUP_SYMS(xfs_iomap_ops);
	return 0;
}

static void destroy_blk_disk(blk_disk_data *disk_data)
{

    del_gendisk(disk_data->blkdev_disk);// 释放磁盘块设备
    put_disk(disk_data->blkdev_disk);   // 释放申请的设备资源
    blk_cleanup_queue(disk_data->blkdev_queue);// 清除请求队列
	kfree(disk_data->disk_name);
	kfree(disk_data->file_path);
	kfree(disk_data->map_data);
	fput(disk_data->filp);
	kfree(disk_data);
}

static blk_disk_data *create_blk_disk(char *file_path)
{
	blk_disk_data *disk_data;
	struct gendisk *blkdev_disk;
	struct request_queue *blkdev_queue;

	disk_data = kmalloc(sizeof(blk_disk_data), GFP_KERNEL);
	if (!disk_data)
		return NULL;

    blkdev_disk = alloc_disk(1);
    if (!blkdev_disk) {
        goto err_alloc_disk;
    }

    strcpy(blkdev_disk->disk_name, SIMP_BLKDEV_DISKNAME);
    blkdev_disk->major = SIMP_BLKDEV_DEVICEMAJOR;
    blkdev_disk->first_minor = 0;
    blkdev_disk->fops = &simp_blkdev_fops;

	blkdev_queue = blk_alloc_queue_node(GFP_KERNEL, NUMA_NO_NODE);

	blk_queue_make_request(blkdev_queue, simp_blkdev_do_request);
	blk_queue_physical_block_size(blkdev_queue, PAGE_SIZE);
	blk_queue_logical_block_size(blkdev_queue, PAGE_SIZE);
	blkdev_disk->queue = blkdev_queue;

	printk("zz %s %d \n", __func__, __LINE__);
    set_capacity(blkdev_disk, SIMP_BLKDEV_BYTES>>9);

	printk("sip queue size %lx\n", blkdev_queue->limits.logical_block_size);

    //3.入口处添加磁盘块设备
    add_disk(blkdev_disk);

	//strncpy(disk_data->file_path,  file_path, 512);

	disk_data->map_data = kmalloc(PAGE_SIZE, GFP_KERNEL);
	memset(disk_data->map_data, 0, PAGE_SIZE);

    disk_data->blkdev_queue = blkdev_queue;
    disk_data->blkdev_disk = blkdev_disk;

	printk("zz %s %d \n", __func__, __LINE__);
	if (hook_block_tag(disk_data))
		goto free_disk;

	printk("zz %s %d \n", __func__, __LINE__);
	return disk_data;

err_alloc_disk:
	put_disk(blkdev_disk);
	kfree(disk_data);
	return NULL;

free_disk:
    del_gendisk(blkdev_disk);// 释放磁盘块设备
    put_disk(blkdev_disk);   // 释放申请的设备资源
    blk_cleanup_queue(blkdev_queue);// 清除请求队列
	kfree(disk_data);
	return NULL;
}

static blk_disk_data *test_data;
static int __init simp_blkdev_init(void)
{
    int ret;

	if (sym_init())
		return -EINVAL;

	test_data = create_blk_disk("/mnt/data");
	if (!test_data)
		return -EINVAL;

	if (misc_register(&misc_dev)) {
		pr_err(" misc register err\n");
		return -EINVAL;
	}


	//bioset_init(&split_bs, BIO_POOL_SIZE, 0, 0);
	//memset(test_block_map, 0, PAGE_SIZE);

    return 0;

err_alloc_disk:
	return ret;

}

static void __exit simp_blkdev_exit(void)
{
	destroy_blk_disk(test_data);
	misc_deregister(&misc_dev);
}

module_init(simp_blkdev_init);
module_exit(simp_blkdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
