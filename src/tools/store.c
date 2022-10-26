#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sched.h>
#include <time.h>
#include <unistd.h>

#include <wlmio.h>


int main(int argc, char** argv)
{
	if(argc < 2)
	{
		fprintf(stderr, "Expected argument\n");
		return EXIT_FAILURE;
	}

	char* endptr;
	errno = 0;
	uint8_t node_id = strtol(argv[1], &endptr, 0);
	if(errno == ERANGE || errno == EINVAL || argv[1] == endptr)
	{
		fprintf(stderr, "Invalid node id\n");
		return EXIT_FAILURE;
	}

	if(node_id > 127)
	{
		fprintf(stderr, "Node ID must between 0 and 127 inclusive\n");
		return EXIT_FAILURE;
	}

	// request real-time priority
	struct sched_param sparam;
	sparam.sched_priority = sched_get_priority_min(SCHED_FIFO);
	sched_setscheduler(0, SCHED_FIFO, &sparam);

	// initialize libwlmio
	if(wlmio_init() < 0)
	{
		perror("Failed to initialize libwlmio: ");
		return 1;
	}

	// store settings to eeprom
	int32_t ret = wlmio_execute_command_sync(node_id, 65530, NULL, 0);
	printf("Stored calibration values in EEPROM ret: %d\n", ret);

	return 0;
}
