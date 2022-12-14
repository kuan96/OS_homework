#ifndef COMMAND_H
#define COMMAND_H

#define MAX_RECORD_NUM 16
#define BUF_SIZE 1024
#define DEFAULT_STACK_SZIE (1024 * 128)
#define MAX_UTHREAD_SIZE 1024

#include <stdbool.h>
#include <ucontext.h>

typedef void (*fun)(void);

char algo[8];

ucontext_t main_process;

struct pipes
{
	char **args;
	int length;
	struct pipes *next;
};

struct cmd
{
	struct pipes *head;
	bool background;
	char *in_file, *out_file;
};

typedef struct uthread
{
	ucontext_t uctx;

	char state[16];
	fun function;
	char stack[DEFAULT_STACK_SZIE];
	char name[32];
	int priority;
	int tid;

	struct uthread *next;

	int need_wait;
	int waited;
	int per_waited;
	int run;
	int turnaround;

	int resourse_get[8];
	int resource_get_curIndex;
	int resourse_need[8];
	int resourse_need_curIndex;
} Task;

// struct ready_list *running;

char *history[MAX_RECORD_NUM];
int history_count;

char *read_line();
struct cmd *split_line(char *);
void test_cmd_struct(struct cmd *);

void sigroutine(int);
void stop();
int stop_flag;

void init();
void free_element();
void insert(Task **, Task **);
Task *delete(Task **, Task **);

Task **task_arr;
int total_task;
Task *ready_head;
Task *waiting_head;
Task *running;

int terminate_count;

int resourse[8];

int rr_flag;
int pp_flag;

#endif
