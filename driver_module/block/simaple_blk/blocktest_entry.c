#include <linux/module.h>
#include <linux/blkdev.h>

#define BLOCK_DISK_SIZE (1024*1024*1024)

#define SIMP_BLKDEV_DISKNAME "simp_blkdev"          //块设备名
#define SIMP_BLKDEV_DEVICEMAJOR COMPAQ_SMART2_MAJOR //主设备号
#define SIMP_BLKDEV_BYTES (1024*1024*1024)            // 块设备大小为50MB
#define MAX_OFFSET  (SIMP_BLKDEV_BYTES>>12)
#define SECTOR_SIZE_SHIFT 9

static struct gendisk *simp_blkdev_disk;// gendisk结构表示一个简单的磁盘设备
static struct block_device_operations simp_blkdev_fops = { //块设备操作，gendisk的一个属性
    .owner = THIS_MODULE,
};
static struct request_queue *simp_blkdev_queue;//指向块设备请求队列的指针
//unsigned char simp_blkdev_data[SIMP_BLKDEV_BYTES];// 虚拟磁盘块设备的存储空间
unsigned long *simp_blkdev_data;// 虚拟磁盘块设备的存储空间


static int count_cnt=0;
/******************************************************
*
*   磁盘块设备数据请求的处理函数
*
******************************************************/
static void simp_blkdev_do_request(struct request_queue *q){
    struct request *req;// 正在处理的请求队列中的请求
    struct bio *req_bio;// 当前请求的bio
    struct bio_vec *bvec;// 当前请求的bio的段(segment)链表
    unsigned long *disk_mem;      // 需要读/写的磁盘区域
    char *buffer;        // 磁盘块设备的请求在内存中的缓冲区
	int offset;
    int i = 0;

    while((req = blk_fetch_request(q)) != NULL){
        // 判断当前req是否合法
        if((blk_rq_pos(req)<<SECTOR_SIZE_SHIFT) + blk_rq_bytes(req) > SIMP_BLKDEV_BYTES){
            printk(KERN_ERR SIMP_BLKDEV_DISKNAME":bad request:block=%llu, count=%u\n",(unsigned long long)blk_rq_pos(req),blk_rq_sectors(req));
            blk_end_request_all(req, -EIO);
            continue;
        }
        //获取需要操作的内存位置
        //disk_mem = simp_blkdev_data + (blk_rq_pos(req) << SECTOR_SIZE_SHIFT);
        offset = (blk_rq_pos(req) << SECTOR_SIZE_SHIFT)>>12;
        printk("request pos:%d \n",(blk_rq_pos(req)));
        req_bio = req->bio;// 获取当前请求的bio

        switch (rq_data_dir(req)) {  //判断请求的类型
        case READ:
            // 遍历req请求的bio链表
            while(req_bio != NULL){
                //　for循环处理bio结构中的bio_vec结构体数组（bio_vec结构体数组代表一个完整的缓冲区）
                for(i=0; i<req_bio->bi_vcnt; i++){
                    bvec = &(req_bio->bi_io_vec[i]);
                    buffer = kmap(bvec->bv_page) + bvec->bv_offset;
					printk("zz %s read req_bio->bi_vcnt:%lx len:%d offset:%d\n",__func__, (unsigned long)req_bio->bi_vcnt, bvec->bv_len, offset);
					if (offset >= MAX_OFFSET) {
						printk("Warning: offet over memory\n ");
						break;
					}
					if (!simp_blkdev_data[offset]) {
						simp_blkdev_data[offset] = kmalloc(4096, GFP_KERNEL);
						if (!simp_blkdev_data[offset]) {
							printk("Warning: read malloc memory failed\n ");
							break;
						}
						disk_mem = simp_blkdev_data[offset];
					}

                    memcpy(buffer, disk_mem, bvec->bv_len);

                    kunmap(bvec->bv_page);
                    //disk_mem += bvec->bv_len;
					offset += bvec->bv_len>>12;
                }
                req_bio = req_bio->bi_next;
            }
            __blk_end_request_all(req, 0);
            break;
        case WRITE:
            while(req_bio != NULL){
				printk("zz %s write req_bio->bi_vcnt:%lx \n",__func__, (unsigned long)req_bio->bi_vcnt);
                for(i=0; i<req_bio->bi_vcnt; i++){
                    bvec = &(req_bio->bi_io_vec[i]);
                    buffer = kmap(bvec->bv_page) + bvec->bv_offset;
#if 0
					disk_mem = simp_blkdev_data[offset];

					if (!disk_mem) {
						printk("Warning: write malloc memory failed\n ");
						break;
					}

                    memcpy(disk_mem, buffer, bvec->bv_len);
#endif
                    kunmap(bvec->bv_page);
                    disk_mem += bvec->bv_len;
                }
                req_bio = req_bio->bi_next;
            }
			//if (count_cnt++ !=35)
            //	__blk_end_request_all(req, 0);
            break;
        default:
            /* No default because rq_data_dir(req) is 1 bit */
            break;
        }
    }
}


/******************************************************
*
*   模块的入口函数
*
******************************************************/
static int __init simp_blkdev_init(void){
    int ret;

	simp_blkdev_data = kzalloc(MAX_OFFSET, GFP_KERNEL);
	if (!simp_blkdev_data) {
		printk("malloc block %lx failed\n", BLOCK_DISK_SIZE);
		return -ENOMEM;
	}

    //1.添加设备之前，先申请设备的资源
    simp_blkdev_disk = alloc_disk(1);
    if(!simp_blkdev_disk){
        ret = -ENOMEM;
        goto err_alloc_disk;
    }

    //2.设置设备的有关属性(设备名，设备号，fops指针,请求队列,512B的扇区数)
    strcpy(simp_blkdev_disk->disk_name,SIMP_BLKDEV_DISKNAME);
    simp_blkdev_disk->major = SIMP_BLKDEV_DEVICEMAJOR;
    simp_blkdev_disk->first_minor = 0;
    simp_blkdev_disk->fops = &simp_blkdev_fops;
    // 将块设备请求处理函数的地址传入blk_init_queue函数，初始化一个请求队列
    simp_blkdev_queue = blk_init_queue(simp_blkdev_do_request, NULL);
    if(!simp_blkdev_queue){
        ret = -ENOMEM;
        goto err_init_queue;
    }
    simp_blkdev_disk->queue = simp_blkdev_queue;
    set_capacity(simp_blkdev_disk, BLOCK_DISK_SIZE>>9);

    //3.入口处添加磁盘块设备
    add_disk(simp_blkdev_disk);
    return 0;

    err_alloc_disk:
		
    err_init_queue:
		kfree(simp_blkdev_data);
        return ret;
}


/******************************************************
*
*   模块的出口函数
*
******************************************************/
static void __exit simp_blkdev_exit(void)
{
    del_gendisk(simp_blkdev_disk);// 释放磁盘块设备
    put_disk(simp_blkdev_disk);   // 释放申请的设备资源
    blk_cleanup_queue(simp_blkdev_queue);// 清除请求队列
	kfree(simp_blkdev_data);
}

module_init(simp_blkdev_init);// 声明模块的入口
module_exit(simp_blkdev_exit);// 声明模块的出口

