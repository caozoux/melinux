
#define KBUILD_MODNAME "foo"
#include <linux/bpf.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/path.h>
#include <linux/types.h>
#include <asm/ptrace.h>
#include "bpf_helpers.h"
#define SEC(NAME) __attribute__((section(NAME), used))

struct task_write {
	char name[64];
	char filename[128];
	char block[32];
	unsigned long write_size;
};

struct bpf_map_def SEC("maps") my_map = {
	.type = BPF_MAP_TYPE_HASH,
	.key_size = sizeof(unsigned long),
	.value_size = sizeof(struct task_write),
	.max_entries = 10000,
};

SEC("kprobe/vfs_write")
int bpf_prog1(struct pt_regs *ctx)
{
	unsigned long cur;
	unsigned int pid;
	struct task_write *writer, new_writer;
	//struct file *filed = (struct fiile *)ctx->di;
	//struct inode *inode;
	//struct dentry *dentry;
	int size = ctx->dx;

	//char fmt[] = "zz len %d\n";
	//bpf_trace_printk(fmt, sizeof(fmt), pid);

	//inode = filed->f_inode;
	//dentry = file->path.dentry;
	cur= bpf_get_current_pid_tgid();
	__builtin_memset(&pid, 0, sizeof(unsigned int));
	pid = cur;

	writer = bpf_map_lookup_elem(&my_map, &pid);
	if (!writer) {
		//__builtin_memset(&new_writer, 0, sizeof(struct task_write));
		//__builtin_memset(&new_writer, 0, 8);
		//bpf_get_current_comm(new_writer.name,64);
		//bpf_probe_read(new_writer.block, 32, inode->i_sb->s_id);
		new_writer.write_size += size;
		//bpf_map_update_elem(&my_map, &pid, &new_writer, BPF_ANY);
	} else {
		 writer->write_size += size;
	}


	return 0;
}

char _license[] SEC("license") = "GPL";
unsigned int _version SEC("version") = LINUX_VERSION_CODE;
