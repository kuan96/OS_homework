#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include "../include/function.h"
#include "../include/command.h"

void init()
{
	ready_head = (Task *)malloc(sizeof(Task));
	waiting_head = (Task *)malloc(sizeof(Task));
	running = (Task *)malloc(sizeof(Task));
	ready_head->next = waiting_head->next = running->next = NULL;

	task_arr = (Task **)malloc(32 * sizeof(Task *));
	total_task = 0;

	terminate_count = 0;

	for (int i = 0; i < 8; ++i)
	{
		resourse[i] = 0;
	}

	rr_flag = 0;
	pp_flag = 0;

	stop_flag = 0;
}

void sigroutine(int sig)
{
	if (sig == SIGVTALRM)
	{
		// add turnaround time
		for (int i = 0; i < total_task; ++i)
		{
			if (strcmp(task_arr[i]->state, "terminate"))
			{
				task_arr[i]->turnaround++;
			}
		}

		// 判斷誰需要離開waiting
		Task *pre = waiting_head;
		Task *cur = pre->next;
		while (cur)
		{
			pre = cur;
			cur = cur->next;

			pre->waited++;

			if (pre->need_wait == -1) // task waiting for resources
			{
				int flag = 0;
				int n = pre->resourse_need_curIndex;
				for (int i = 0; i < n; ++i)
				{
					if (resourse[pre->resourse_need[i]])
					{
						flag = 1;
						break;
					}
				}

				if (!flag) // resource has been release so go back to ready
				{
					pre->need_wait = 0; // don't need to wait resource
					strncpy(pre->state, "ready", 16);
					pre = delete (&waiting_head, &pre);
					insert(&ready_head, &pre);
				}
			}
			else // task is sleeping
			{
				pre->per_waited++;
				if (pre->per_waited >= pre->need_wait)
				{
					pre->per_waited = pre->need_wait = 0;
					// delete pre from waiting and add it to ready
					strncpy(pre->state, "ready", 16);
					pre = delete (&waiting_head, &pre);
					insert(&ready_head, &pre);
				}
			}
		}

		if (running->next)
		{
			running->next->run++;
			// run 3-ms and back to ready
			if (rr_flag && (running->next->run % 3 == 0))
			{
				// go to ready
				Task *tmp = running->next;
				running->next = NULL;
				strncpy(tmp->state, "ready", 16);
				insert(&ready_head, &(tmp));
				// switch to mainprocess to find next ready task
				swapcontext(&tmp->uctx, &main_process);
			}

			// preempted by new task
			if (pp_flag && ready_head->next && (ready_head->next->priority < running->next->priority))
			{
				Task *tmp = running->next;
				running->next = NULL;
				strncpy(tmp->state, "ready", 16);
				insert(&ready_head, &(tmp));
				// switch to mainprocess to find next ready task
				swapcontext(&tmp->uctx, &main_process);
			}
		}
	}
}

void stop()
{
	stop_flag = 1;
	setcontext(&main_process);
}

void insert(Task **head, Task **node)
{
	Task *pre = *head;
	Task *cur = pre->next;

	if (!cur)
	{
		pre->next = *node;
		return;
	}

	if (pp_flag)
	{
		while (cur && (*node)->priority >= cur->priority)
		{
			pre = cur;
			cur = cur->next;
		}
	}
	else
	{
		while (cur)
		{
			pre = cur;
			cur = cur->next;
		}
	}

	pre->next = *node;
	(*node)->next = cur;
}

Task *delete(Task **head, Task **node)
{
	Task *pre = *head;
	Task *cur = pre->next;

	while (cur && cur != (*node))
	{
		pre = cur;
		cur = cur->next;
	}

	if (!cur)
	{
		return cur;
	}

	pre->next = cur->next;
	cur->next = NULL;
	return cur;
}

void free_element()
{
	free(ready_head);
	free(waiting_head);

	for (int i = 0; i < 32; ++i)
	{
		free(task_arr[i]);
	}

	free(task_arr);
}

char *read_line()
{
	char *buffer = (char *)malloc(BUF_SIZE * sizeof(char));
	if (buffer == NULL)
	{
		perror("Unable to allocate buffer");
		exit(1);
	}

	if (fgets(buffer, BUF_SIZE, stdin) != NULL)
	{
		if (buffer[0] == '\n' || buffer[0] == ' ' || buffer[0] == '\t')
		{
			free(buffer);
			buffer = NULL;
		}
		else
		{
			if (strncmp(buffer, "replay", 6) == 0)
			{
				char *token = strtok(buffer, " ");
				token = strtok(NULL, " ");
				int index = strtol(token, NULL, 10);
				if (index > MAX_RECORD_NUM || index > history_count)
				{
					free(buffer);
					buffer = NULL;
				}
				else
				{
					char *temp = (char *)malloc(BUF_SIZE * sizeof(char));
					int head = 0;
					if (history_count > MAX_RECORD_NUM)
					{
						head += history_count % MAX_RECORD_NUM;
					}
					strncpy(temp, history[(head + index - 1) % MAX_RECORD_NUM], BUF_SIZE);
					token = strtok(NULL, " ");
					while (token)
					{
						strcat(temp, " ");
						strcat(temp, token);
						token = strtok(NULL, " ");
					}
					strncpy(buffer, temp, BUF_SIZE);
					free(temp);
					buffer[strcspn(buffer, "\n")] = 0;
					strncpy(history[history_count % MAX_RECORD_NUM], buffer, BUF_SIZE);
					++history_count;
				}
			}
			else
			{
				buffer[strcspn(buffer, "\n")] = 0;
				strncpy(history[history_count % MAX_RECORD_NUM], buffer, BUF_SIZE);
				++history_count;
			}
		}
	}

	return buffer;
}

struct cmd *split_line(char *line)
{
	int args_length = 10;
	struct cmd *new_cmd = (struct cmd *)malloc(sizeof(struct cmd));
	new_cmd->head = (struct pipes *)malloc(sizeof(struct pipes));
	new_cmd->head->args = (char **)malloc(args_length * sizeof(char *));
	for (int i = 0; i < args_length; ++i)
		new_cmd->head->args[i] = NULL;
	new_cmd->head->length = 0;
	new_cmd->head->next = NULL;
	new_cmd->background = false;
	new_cmd->in_file = NULL;
	new_cmd->out_file = NULL;

	struct pipes *temp = new_cmd->head;
	char *token = strtok(line, " ");
	while (token != NULL)
	{
		if (token[0] == '|')
		{
			struct pipes *new_pipe = (struct pipes *)malloc(sizeof(struct pipes));
			new_pipe->args = (char **)malloc(args_length * sizeof(char *));
			for (int i = 0; i < args_length; ++i)
				new_pipe->args[i] = NULL;
			new_pipe->length = 0;
			new_pipe->next = NULL;
			temp->next = new_pipe;
			temp = new_pipe;
		}
		else if (token[0] == '<')
		{
			token = strtok(NULL, " ");
			new_cmd->in_file = token;
		}
		else if (token[0] == '>')
		{
			token = strtok(NULL, " ");
			new_cmd->out_file = token;
		}
		else if (token[0] == '&')
		{
			new_cmd->background = true;
		}
		else
		{
			temp->args[temp->length] = token;
			temp->length++;
		}
		token = strtok(NULL, " ");
	}

	return new_cmd;
}

void test_cmd_struct(struct cmd *cmd)
{
	struct pipes *temp = cmd->head;
	int pipe_count = 0;
	while (temp != NULL)
	{
		printf("pipe %d: ", pipe_count);
		for (int i = 0; i < temp->length; ++i)
		{
			printf("%s ", temp->args[i]);
		}
		printf("\n");
		temp = temp->next;
		++pipe_count;
	}
	printf(" in: %s\n", cmd->in_file ? cmd->in_file : "none");
	printf("out: %s\n", cmd->out_file ? cmd->out_file : "none");
	printf("background: %s\n", cmd->background ? "true" : "false");
}
