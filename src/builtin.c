#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include "../include/builtin.h"
#include "../include/command.h"
#include "../include/function.h"

fun function_arr[] = {&task1, &task2, &task3, &task4, &task5, &task6, &task7, &task8, &task9};

int help(char **args)
{
	int i;
	printf("--------------------------------------------------\n");
	printf("My Little Shell!!\n");
	printf("The following are built in:\n");
	for (i = 0; i < num_builtins(); i++)
	{
		printf("%d: %s\n", i, builtin_str[i]);
	}
	printf("%d: replay\n", i);
	printf("--------------------------------------------------\n");
	return 1;
}

int cd(char **args)
{
	if (args[1] == NULL)
	{
		fprintf(stderr, "lsh: expected argument to \"cd\"\n");
	}
	else
	{
		if (chdir(args[1]) != 0)
			perror("lsh");
	}
	return 1;
}

int echo(char **args)
{
	bool newline = true;
	for (int i = 1; args[i]; ++i)
	{
		if (i == 1 && strcmp(args[i], "-n") == 0)
		{
			newline = false;
			continue;
		}
		printf("%s", args[i]);
		if (args[i + 1])
			printf(" ");
	}
	if (newline)
		printf("\n");

	return 1;
}

int exit_shell(char **args)
{
	return 0;
}

int record(char **args)
{
	if (history_count < MAX_RECORD_NUM)
	{
		for (int i = 0; i < history_count; ++i)
			printf("%2d: %s\n", i + 1, history[i]);
	}
	else
	{
		for (int i = history_count % MAX_RECORD_NUM; i < history_count % MAX_RECORD_NUM + MAX_RECORD_NUM; ++i)
			printf("%2d: %s\n", i - history_count % MAX_RECORD_NUM + 1, history[i % MAX_RECORD_NUM]);
	}
	return 1;
}

bool isnum(char *str)
{
	for (int i = 0; i < strlen(str); ++i)
	{
		if (str[i] >= 48 && str[i] <= 57)
			continue;
		else
			return false;
	}
	return true;
}

int mypid(char **args)
{
	char fname[BUF_SIZE];
	char buffer[BUF_SIZE];
	if (strcmp(args[1], "-i") == 0)
	{

		pid_t pid = getpid();
		printf("%d\n", pid);
	}
	else if (strcmp(args[1], "-p") == 0)
	{

		if (args[2] == NULL)
		{
			printf("mypid -p: too few argument\n");
			return 1;
		}

		sprintf(fname, "/proc/%s/stat", args[2]);
		int fd = open(fname, O_RDONLY);
		if (fd == -1)
		{
			printf("mypid -p: process id not exist\n");
			return 1;
		}

		read(fd, buffer, BUF_SIZE);
		strtok(buffer, " ");
		strtok(NULL, " ");
		strtok(NULL, " ");
		char *s_ppid = strtok(NULL, " ");
		int ppid = strtol(s_ppid, NULL, 10);
		printf("%d\n", ppid);

		close(fd);
	}
	else if (strcmp(args[1], "-c") == 0)
	{

		if (args[2] == NULL)
		{
			printf("mypid -c: too few argument\n");
			return 1;
		}

		DIR *dirp;
		if ((dirp = opendir("/proc/")) == NULL)
		{
			printf("open directory error!\n");
			return 1;
		}

		struct dirent *direntp;
		while ((direntp = readdir(dirp)) != NULL)
		{
			if (!isnum(direntp->d_name))
			{
				continue;
			}
			else
			{
				sprintf(fname, "/proc/%s/stat", direntp->d_name);
				int fd = open(fname, O_RDONLY);
				if (fd == -1)
				{
					printf("mypid -p: process id not exist\n");
					return 1;
				}

				read(fd, buffer, BUF_SIZE);
				strtok(buffer, " ");
				strtok(NULL, " ");
				strtok(NULL, " ");
				char *s_ppid = strtok(NULL, " ");
				if (strcmp(s_ppid, args[2]) == 0)
					printf("%s\n", direntp->d_name);

				close(fd);
			}
		}

		closedir(dirp);
	}
	else
	{
		printf("wrong type! Please type again!\n");
	}

	return 1;
}

int add(char **args)
{
	char *task_name = args[1];
	char *func = args[2];
	// char *prior = args[3];

	// create a uthread
	Task *newtask = (Task *)malloc(sizeof(Task));
	newtask->tid = total_task + 1;
	newtask->next = NULL;
	newtask->priority = 0;
	newtask->waited = 0;
	newtask->need_wait = 0;
	newtask->per_waited = 0;
	newtask->run = 0;
	newtask->turnaround = 0;
	newtask->resource_get_curIndex = 0;
	newtask->resourse_need_curIndex = 0;
	strncpy(newtask->name, task_name, 32);
	strncpy(newtask->state, "ready", 16);
	newtask->priority = atoi(args[3]);

	int index = 1;
	char task[] = "task0";
	while (index <= 9)
	{
		task[4] = '0' + index;
		if (!strcmp(func, task))
		{
			newtask->function = *function_arr[index - 1];
			break;
		}

		index++;
	}
	if (!strcmp(func, "test_exit"))
		newtask->function = test_exit;
	if (!strcmp(func, "test_sleep"))
		newtask->function = test_sleep;
	if (!strcmp(func, "test_resource1"))
		newtask->function = test_resource1;
	if (!strcmp(func, "test_resource2"))
		newtask->function = test_resource2;

	getcontext(&newtask->uctx);
	newtask->uctx.uc_stack.ss_sp = newtask->stack;
	newtask->uctx.uc_stack.ss_size = DEFAULT_STACK_SZIE;
	newtask->uctx.uc_stack.ss_flags = 0;
	newtask->uctx.uc_link = NULL;
	makecontext(&newtask->uctx, newtask->function, 0);

	printf("Task %s is ready\n", task_name);

	task_arr[total_task++] = newtask;
	insert(&ready_head, &newtask);

	return 1;
}

int del(char **args)
{
	char *task_name = args[1];
	for (int i = 0; i < total_task; ++i)
	{
		if (!strcmp(task_name, task_arr[i]->name))
		{
			terminate_count++;
			strncpy(task_arr[i]->state, "terminate", 16);
			// delete from ready and waiting
			delete (&ready_head, &(task_arr[i]));
			delete (&waiting_head, &(task_arr[i]));
			// delete from running
			if (running->next && !strcmp(task_name, running->next->name))
				running->next = NULL;
			break;
		}
	}

	return 1;
}

int ps(char **args)
{
	puts("\n*************************************************************************************************");
	printf("* TID  |  Name  |     State     |  Running  |  Waiting  | Turnaround |  Resources  |  Priority *\n");
	puts("-------------------------------------------------------------------------------------------------");
	for (int i = 0; i < total_task; ++i)
	{
		printf("*  %d   |   %s   |   %s   |     %d     |     %d     |",
			   task_arr[i]->tid, task_arr[i]->name, task_arr[i]->state, task_arr[i]->run, task_arr[i]->waited);

		// print turnaround time
		if (!strcmp(task_arr[i]->state, "terminate"))
		{
			printf("      %d      |", task_arr[i]->turnaround);
		}
		else
		{
			printf("   none   |");
		}

		// print resource
		if (task_arr[i]->resource_get_curIndex > 0)
		{
			int n = task_arr[i]->resource_get_curIndex;
			for (int j = 0; j < n; j++)
				printf("%d ", task_arr[i]->resourse_get[j]);
		}
		else
			printf("    none    ");

		printf("|");
		printf("     %d     *\n", task_arr[i]->priority);
	}

	puts("*************************************************************************************************");

	return 1;
}

int start(char **args)
{
	stop_flag = 0;

	printf("start simulation\n");
	struct itimerval v;
	signal(SIGVTALRM, sigroutine);
	v.it_value.tv_sec = 0;
	v.it_value.tv_usec = 10000;
	v.it_interval.tv_sec = 0;
	v.it_interval.tv_usec = 10000;
	setitimer(ITIMER_VIRTUAL, &v, NULL);

	getcontext(&main_process);
	// printf("Hii\n");
	if (stop_flag)
		return 1;

	if (terminate_count == total_task)
	{
		printf("simulation over.\n");
		return 1;
	}

	if (running->next)
	{
		printf("task %s is running.\n", running->next->name);
		setcontext(&running->next->uctx);
	}

	Task *cur = ready_head->next;
	if (cur)
	{
		printf("task %s is running.\n", cur->name);
		cur = delete (&ready_head, &cur);
		running->next = cur;
		setcontext(&cur->uctx);
	}
	else
	{
		printf("cpu idle\n");
		while (!ready_head->next)
			;
	}

	setcontext(&main_process);

	return 1;
}

const char *builtin_str[] = {
	"help",
	"cd",
	"echo",
	"exit",
	"record",
	"mypid",
	"add",
	"del",
	"ps",
	"start"};

const int (*builtin_func[])(char **) = {
	&help,
	&cd,
	&echo,
	&exit_shell,
	&record,
	&mypid,
	&add,
	&del,
	&ps,
	&start};

int num_builtins()
{
	return sizeof(builtin_str) / sizeof(char *);
}
