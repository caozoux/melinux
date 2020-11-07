#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "mmtool.h"

#define MAX_PROC_SIZE (1024*1024)

struct proc_task **show_proc_list;
int cur_page = 0;
int proc_size = 0;

void proc_task_free(struct proc_task *task)
{
	free(task->buf);
}

void proc_task_init(proc_t *buf, struct proc_task *task)
{
	strcpy(task->ruser, buf->ruser);
	task->tid = buf->tid;
	strcpy(task->cmd, buf->cmd);
	task->vsize= buf->vsize;
	task->rss = buf->rss;
	memset(task->order, 0, sizeof(unsigned long) * 11);
	task->buf = malloc(4096*4096);
}

static void show_signal_task(unsigned long x, int page)
{
	int ch;
	int i,j, offset = 0;
	unsigned long last_addr;
	unsigned long block_page = 1;
	struct proc_task *task = show_proc_list[x - page - 2];
	char buf[1024], *line;

	if ((x - page) < 2)
		return;

	task = show_proc_list[x - page - 2];

	x = 2;
reflash:
    mvprintw(0, 0, "any key is back \n", cur_page);
    LOCPRINT(1, 0, "%-08s  %-08s %-020s %-020s %-012s %-012s\n", "USER", "PID", "COMMON", "VIRT", "RSS", "order0/1/2/3/4/5/7/8/9/10/11");
	LOCPRINT(2, 0, "%-08s  %-08d %-020s %-020d %-012d%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/\n", task->ruser, task->tid, task->cmd, task->vsize, task->rss
		, task->order[0]
		, task->order[1]
		, task->order[2]
		, task->order[3]
		, task->order[4]
		, task->order[5]
		, task->order[6]
		, task->order[7]
		, task->order[8]
		, task->order[9]
		, task->order[10]
		, task->order[11]
		);
	x++;

#if 1
	for (i = 0; i < task->page_index; i += COW_MAX) {
		last_addr = task->buf[i];
		line = buf;

		for (j = 0; j < COW_MAX ; ++j) {
			if (j == 0) {
				offset = sprintf(line, "%lx:-", task->buf[i]);
				line += offset;
			} else {
				if (task->buf[i+j] == (last_addr + 0x1000)) {
					block_page++;
					offset = sprintf(line, "-");
					line = line + offset;
				} else {
					unsigned long index = 0, val = block_page;
					while (val = val/2)
						index++;
					//printf("%d:%d", (int)block_page,index);
					block_page = 1;
					offset = sprintf(line, "*");
					line += offset;
				}
			}
			last_addr = task->buf[j+i];
		}
    	LOCPRINT(x++, 0, "%s\n", buf);
	}
#endif

    refresh();
    ch = getch();
}

void wshow_proc_list(int x, int page)
{
	int i;
	struct proc_task *proc_task;

    LOCPRINT(x++, 0, "%-08s  %-08s %-020s %-020s %-012s %-012s\n", "USER", "PID", "COMMON", "VIRT", "RSS", "order0/1/2/3/4/5/7/8/9/10/11");

	for (i = 0; i < proc_size; ++i) {

			proc_task = show_proc_list[i];

    		LOCPRINT(x - page + i, 0, "%-08s  %-08d %-020s %-020d %-012d%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/\n", proc_task->ruser, proc_task->tid, proc_task->cmd, proc_task->vsize, proc_task->rss
				, proc_task->order[0]
				, proc_task->order[1]
				, proc_task->order[2]
				, proc_task->order[3]
				, proc_task->order[4]
				, proc_task->order[5]
				, proc_task->order[6]
				, proc_task->order[7]
				, proc_task->order[8]
				, proc_task->order[9]
				, proc_task->order[10]
				, proc_task->order[11]
				);
	}
}

int main()
{
    char mesg[]="Just a string";        /* 将要被打印的字符串*/
    int row,col;                        /* 存储行号和列号的变量，用于指定光标位置*/
	int x = 0, y = 2;
	int ch;
	int cur_row = 0;
	
	show_proc_list = malloc(MAX_PROC_SIZE);
	if (!show_proc_list)
		return -1;

	memset(show_proc_list, 0, MAX_PROC_SIZE);

#if 1
    initscr();                          /* 进入curses 模式*/
	cbreak();
    keypad(stdscr, TRUE);
    noecho();
	move(2,0);
    getmaxyx(stdscr,row,col);           /* 取得stdscr（标准输出设备）的行数和列数*/
	proc_size = scan_proc_list(show_proc_list, 1, 0);
reflash:
    mvprintw(0, 0, "r: reflash n:next p:prev x:exit page:%d\n", cur_page);
	wshow_proc_list(1, cur_page);
	move(y, x);
    refresh();
    ch = getch();

	switch (ch) {
		case KEY_DOWN:
			y++;
			goto reflash;
		case KEY_UP:
			if (y)
				y--;
			goto reflash;
		case 0xa:
			//y++;
			clear();
			show_signal_task(y, cur_page);
			goto reflash;
		case KEY_ENTER:
			y++;
			goto reflash;
		case KEY_BACKSPACE:
			if (y)
				y--;
			goto reflash;
		case KEY_DL:
			if (y)
				y--;
			goto reflash;
		case 'r':
			clear();
			scan_proc_list(show_proc_list, 1, 0);
			refresh();
			goto reflash;
		case 'n':
			cur_page++;
			cur_row += row;
			clear();
			//scan_proc_list(show_proc_list, 1, cur_row);
			refresh();
			goto reflash;
			break;
		case 'p':
			if (cur_row > row)
				cur_row -= row;
			else
				cur_row = 0;

			if (cur_page)
				cur_page --;

			clear();
			//scan_proc_list(show_proc_list, 1, cur_row);
			refresh();
			goto reflash;
			break;
		case 'x':
    		endwin();
			break;
		default:
			goto reflash;
	}
    endwin();
#else
	scan_proc_list(show_proc_list, 0, 1, 0);
#endif
	free(show_proc_list);
    return 0;
}

