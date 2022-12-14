#include "../include/resource.h"
#include "../include/command.h"
#include <stdio.h>
#include <string.h>

void get_resources(int count, int *resources)
{
    Task *cur = running->next;
    int flag = 0;
    // current task's resourse need
    for (int i = 0; i < count; ++i)
    {
        cur->resourse_need[cur->resourse_need_curIndex] = resources[i];
        cur->resourse_need_curIndex++;
    }

    for (int i = 0; i < count; ++i)
    {
        int n = resources[i];
        if (resourse[n]) // resourse n has been occupied
        {
            flag = 1;
            break;
        }
    }

    if (flag) // resource has been occupied
    {
        printf("task %s is waiting resource.\n", cur->name);
        running->next = NULL;
        // go to waiting
        cur->need_wait = -1; // wait resourse sign
        strncpy(cur->state, "waiting", 16);
        insert(&waiting_head, &cur);
        swapcontext(&cur->uctx, &main_process);
    }

    // occupy resource
    Task *tmp = running->next;
    tmp->resourse_need_curIndex = 0; // don't need any resource currently
    for (int i = 0; i < count; ++i)
    {
        printf("task %s gets resource %d.\n", tmp->name, resources[i]);
        tmp->resourse_get[tmp->resource_get_curIndex] = resources[i];
        tmp->resource_get_curIndex++;
        resourse[resources[i]] = 1;
    }
}

void release_resources(int count, int *resources)
{
    Task *cur = running->next;
    cur->resource_get_curIndex = cur->resourse_need_curIndex = 0;
    for (int i = 0; i < count; ++i)
    {
        printf("task %s release resourse %d.\n", cur->name, resources[i]);
        resourse[resources[i]] = 0;
    }
}
