#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sched.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/unistd.h>

#include <wlmio.h>

void print_usage_and_exit(char* const argv[])
{
	fprintf(stderr, "Usage: %s id\n", argv[0]);
	exit(EXIT_FAILURE);
}

int main(const int argc, char* const argv[])
{
	if(argc < 2)
	{
		fprintf(stderr, "Expected argument\n");
		print_usage_and_exit(argv);
	}
	
	char* endptr;
	errno = 0;
	uint8_t node_id = strtol(argv[1], &endptr, 0);
	if(errno == ERANGE || errno == EINVAL || argv[1] == endptr)
	{
		fprintf(stderr, "Invalid node id\n");
		print_usage_and_exit(argv);
	}
	
	if(node_id > 127)
	{
		fprintf(stderr, "Node ID must between 0 and 127 inclusive\n");
		return EXIT_FAILURE;
	}
	
	// request real-time priority
	struct sched_param param;
	param.sched_priority = sched_get_priority_min(SCHED_FIFO);
	sched_setscheduler(0, SCHED_FIFO, &param);
	
	// initialize libwlmio
	if(wlmio_init() < 0)
	{
		fprintf(stderr, "Failed to initialize libwlmio\n");
		return EXIT_FAILURE;
	}

	int32_t r = wlmio_execute_command_sync(node_id, WLMIO_COMMAND_FACTORY_RESET, NULL, 0);
	if(r < 0 || r != WLMIO_COMMAND_STATUS_SUCCESS)
	{
		fprintf(stderr, "Could not factory reset node %d\n", node_id);
		return EXIT_FAILURE;
	}

	r = wlmio_execute_command_sync(node_id, WLMIO_COMMAND_RESTART, NULL, 0);
	if(r < 0)
	{
		fprintf(stderr, "Could not restart node %d\n", node_id);
		return EXIT_FAILURE;
	}

	printf("Success\n");

	return EXIT_SUCCESS;
}
