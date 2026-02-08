#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <getopt.h>
#include <string>

// Constants
#define PAGE_SIZE	4096
#define DEFAULT_FILE_MODE 0644
#define MAX_PATH_LEN	256
#define MAX_TYPE_LEN	64

// Memory operation types
enum MemoryType {
	MEM_TYPE_ANON,
	MEM_TYPE_DIRTY,
	MEM_TYPE_SHEM,
	MEM_TYPE_MMAP,
	MEM_TYPE_THP,
	MEM_TYPE_MLOCK,
	MEM_TYPE_UNKNOWN
};

// Configuration structure
struct MemoryConfig {
	int page_count;
	int do_fork;
	enum MemoryType type;
	char filename[MAX_PATH_LEN];
};

// Global long options for getopt
static struct option long_options[] = {
	{"version", no_argument,	0,	'v'},
	{"help",	no_argument,	0,	'h'},
	{"fork",	no_argument,	0,	'k'},
	{"type",	required_argument,	0,	't'},
	{"file",	required_argument,	0,	'f'},
	{"size",	required_argument,	0,	's'},
	{0, 0, 0, 0}
};

// Function declarations
static void print_help(void);
static enum MemoryType parse_memory_type(const char *type_str);
static int parse_arguments(int argc, char *argv[], struct MemoryConfig *config);
static int test_anon_memory(unsigned long page_count);
static int test_dirty_memory(const char *filename, unsigned long page_count);
static int test_thp_memory(unsigned long page_count);
static int test_mmap_memory(const char *filename, unsigned long page_count);
static void run_fork_test(void);

//============================================================================
// Helper Functions
//============================================================================

static void print_help(void)
{
	printf("Memory test tool\n\n");
	printf("Options:\n");
	printf("  -t, --type TYPE    Memory type: anon, dirty, shem, mmap, thp, mlock\n");
	printf("  -f, --file FILE    Specify file name (for dirty/mmap types)\n");
	printf("  -s, --size NUM     Page count\n");
	printf("  -k, --fork         Fork process after memory allocation\n");
	printf("  -v, --version      Show version\n");
	printf("  -h, --help         Show this help message\n");
}

static enum MemoryType parse_memory_type(const char *type_str)
{
	if (!type_str)
		return MEM_TYPE_UNKNOWN;

	if (strcmp(type_str, "anon") == 0)
		return MEM_TYPE_ANON;
	if (strcmp(type_str, "dirty") == 0)
		return MEM_TYPE_DIRTY;
	if (strcmp(type_str, "shem") == 0)
		return MEM_TYPE_SHEM;
	if (strcmp(type_str, "mmap") == 0)
		return MEM_TYPE_MMAP;
	if (strcmp(type_str, "thp") == 0)
		return MEM_TYPE_THP;
	if (strcmp(type_str, "mlock") == 0)
		return MEM_TYPE_MLOCK;

	return MEM_TYPE_UNKNOWN;
}

static void run_fork_test(void)
{
	pid_t pid = fork();

	if (pid < 0) {
		perror("fork failed");
		return;
	}

	if (pid > 0) {
		// Parent process
		printf("Parent process (PID: %d) running...\n", getpid());
	} else {
		// Child process
		printf("Child process (PID: %d) running...\n", getpid());
	}

	// Both processes sleep indefinitely
	while (1) {
		sleep(1);
	}
}

//============================================================================
// Memory Test Functions
//============================================================================

static int test_anon_memory(unsigned long page_count)
{
	printf("Testing anonymous memory allocation: %lu pages (%lu MB)\n",
	       page_count, (page_count * PAGE_SIZE) / (1024 * 1024));

	for (unsigned long i = 0; i < page_count; i++) {
		void *buf = malloc(PAGE_SIZE);
		if (!buf) {
			fprintf(stderr, "Failed to allocate page %lu\n", i);
			return -1;
		}
		memset(buf, 0, PAGE_SIZE);
		// Note: buffers are intentionally not freed to consume memory
	}

	printf("Anonymous memory allocation completed\n");
	return 0;
}

static int test_dirty_memory(const char *filename, unsigned long page_count)
{
	int fd;
	char pagebuf[PAGE_SIZE] = {0};

	if (!filename || strlen(filename) == 0) {
		fprintf(stderr, "Error: filename required for dirty memory test\n");
		return -1;
	}

	printf("Testing dirty page generation: %lu pages to file '%s'\n",
	       page_count, filename);

	fd = open(filename, O_CREAT | O_RDWR, DEFAULT_FILE_MODE);
	if (fd < 0) {
		perror("Failed to open file");
		return -1;
	}

	for (unsigned long i = 0; i < page_count; i++) {
		if (write(fd, pagebuf, PAGE_SIZE) != PAGE_SIZE) {
			perror("Failed to write page");
			close(fd);
			return -1;
		}
	}

	close(fd);
	printf("Dirty page generation completed\n");
	return 0;
}

static int test_thp_memory(unsigned long page_count)
{
	void *buf;
	unsigned long size = page_count * PAGE_SIZE;

	printf("Testing THP (Transparent Huge Pages): %lu pages (%lu MB)\n",
	       page_count, size / (1024 * 1024));

	buf = mmap(NULL, size, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (buf == MAP_FAILED) {
		perror("mmap failed");
		return -1;
	}

	// Optional: Use madvise to request huge pages
	if (madvise(buf, size, MADV_HUGEPAGE) != 0) {
		perror("madvise MADV_HUGEPAGE failed (continuing anyway)");
	}

	// Touch memory to allocate pages
	memset(buf, 0, size);

	printf("THP allocation completed\n");
	return 0;
}

static int test_mlock_memory(const char *filename, unsigned long page_count)
{
	int size = page_count * PAGE_SIZE;
	void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	mlock(ptr, size);  // 告诉内核：这些页面必须常驻物理内存

}
static int test_mmap_memory(const char *filename, unsigned long page_count)
{
	int fd;
	char *addr;
	unsigned long size = page_count * PAGE_SIZE;

	if (!filename || strlen(filename) == 0) {
		fprintf(stderr, "Error: filename required for mmap test\n");
		return -1;
	}

	printf("Testing mmap: %lu pages (%lu MB) from file '%s'\n",
	       page_count, size / (1024 * 1024), filename);

	fd = open(filename, O_CREAT | O_RDWR, DEFAULT_FILE_MODE);
	if (fd < 0) {
		perror("Failed to open file");
		return -1;
	}

	// Set file size
	if (ftruncate(fd, size) < 0) {
		perror("Failed to set file size");
		close(fd);
		return -1;
	}

	addr = (char *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED) {
		perror("mmap failed");
		close(fd);
		return -1;
	}

	close(fd); // Can close fd after mmap

	printf("mmap completed: addr=%p, size=%lu bytes\n", addr, size);
	return 0;
}

//============================================================================
// Argument Parsing
//============================================================================

static int parse_arguments(int argc, char *argv[], struct MemoryConfig *config)
{
	int choice;
	char type_str[MAX_TYPE_LEN] = {0};

	// Initialize config with defaults
	memset(config, 0, sizeof(struct MemoryConfig));
	config->type = MEM_TYPE_UNKNOWN;

	while (1) {
		int option_index = 0;
		choice = getopt_long(argc, argv, "vht:f:s:k",
				     long_options, &option_index);

		if (choice == -1)
			break;

		switch (choice) {
		case 'v':
			printf("Memory test tool v1.0\n");
			return 1; // Exit after version

		case 'h':
			print_help();
			return 1; // Exit after help

		case 't':
			strncpy(type_str, optarg, MAX_TYPE_LEN - 1);
			type_str[MAX_TYPE_LEN - 1] = '\0';
			config->type = parse_memory_type(type_str);
			if (config->type == MEM_TYPE_UNKNOWN) {
				fprintf(stderr, "Error: Unknown memory type '%s'\n", optarg);
				return -1;
			}
			break;

		case 'f':
			strncpy(config->filename, optarg, MAX_PATH_LEN - 1);
			config->filename[MAX_PATH_LEN - 1] = '\0';
			break;

		case 's':
			config->page_count = atoi(optarg);
			if (config->page_count <= 0) {
				fprintf(stderr, "Error: Invalid page count '%s'\n", optarg);
				return -1;
			}
			break;

		case 'k':
			config->do_fork = 1;
			break;

		default:
			return -1;
		}
	}

	// Validate required arguments
	if (config->type == MEM_TYPE_UNKNOWN) {
		fprintf(stderr, "Error: Memory type (-t) is required\n");
		return -1;
	}

	if (config->page_count <= 0) {
		fprintf(stderr, "Error: Page count (-s) is required and must be > 0\n");
		return -1;
	}

	// Validate type-specific requirements
	if ((config->type == MEM_TYPE_DIRTY || config->type == MEM_TYPE_MMAP) &&
	    strlen(config->filename) == 0) {
		fprintf(stderr, "Error: Filename (-f) is required for this memory type\n");
		return -1;
	}

	return 0;
}

//============================================================================
// Main Function
//============================================================================

int main(int argc, char *argv[])
{
	struct MemoryConfig config;
	int ret;

	// Parse command line arguments
	ret = parse_arguments(argc, argv, &config);
	if (ret > 0) {
		return 0; // Help or version requested
	}
	if (ret < 0) {
		print_help();
		return -1;
	}

	// Execute the appropriate memory test
	switch (config.type) {
	case MEM_TYPE_ANON:
		ret = test_anon_memory(config.page_count);
		break;

	case MEM_TYPE_DIRTY:
		ret = test_dirty_memory(config.filename, config.page_count);
		break;

	case MEM_TYPE_THP:
		ret = test_thp_memory(config.page_count);
		break;

	case MEM_TYPE_MMAP:
		ret = test_mmap_memory(config.filename, config.page_count);
		break;

	case MEM_TYPE_MLOCK:
		ret = test_mlock_memory(config.filename, config.page_count);
		break;

	case MEM_TYPE_SHEM:
		fprintf(stderr, "Error: Shared memory test not yet implemented\n");
		ret = -1;
		break;

	default:
		fprintf(stderr, "Error: Invalid memory type\n");
		ret = -1;
		break;
	}

	if (ret != 0) {
		fprintf(stderr, "Memory test failed\n");
		return -1;
	}

	// Fork if requested
	if (config.do_fork) {
		run_fork_test();
	}

	// Keep process running
	printf("Test completed. Process (PID: %d) sleeping...\n", getpid());
	while (1) {
		sleep(1);
	}

	return 0;
}
