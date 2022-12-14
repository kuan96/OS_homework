#include "../include/task.h"
#include "../include/command.h"
#include <ucontext.h>
#include <string.h>
#include <stdio.h>

void task_sleep(int ms)
{
    Task *tmp = running->next;
    running->next = NULL;
    printf("task %s goes to sleep\n", tmp->name);
    strncpy(tmp->state, "waiting", 16);
    tmp->need_wait = ms;
    insert(&waiting_head, &tmp);
    swapcontext(&tmp->uctx, &main_process);
    return;
}

void task_exit()
{
    Task *tmp = running->next;
    running->next = NULL;
    printf("task %s has terminated\n", tmp->name);
    strncpy(tmp->state, "terminate", 16);
    terminate_count++;
    setcontext(&main_process);
}
