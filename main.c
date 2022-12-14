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
#include "include/shell.h"
#include "include/command.h"

int main(int argc, char *argv[])
{
	init();

	strncpy(algo, argv[1], 8);
	if (!strcmp(algo, "RR"))
		rr_flag = 1;
	if (!strcmp(algo, "PP"))
		pp_flag = 1;

	history_count = 0;
	for (int i = 0; i < MAX_RECORD_NUM; ++i)
		history[i] = (char *)malloc(BUF_SIZE * sizeof(char));

	signal(SIGTSTP, stop);

	shell();

	free_element();

	for (int i = 0; i < MAX_RECORD_NUM; ++i)
		free(history[i]);

	return 0;
}
