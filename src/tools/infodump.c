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
	
	struct wlmio_node_info info;
	int ret = wlmio_get_node_info_sync(node_id, &info);
	if(ret < 0)
	{
		fprintf(stderr, "Could not get node info\n");
		return EXIT_FAILURE;
	}

	printf("Info for Node %d:\n", node_id);
	printf("Node Name = %s\n", info.name);
	printf("UAVCAN Protocol Version = %d.%d\n", info.protocol_version.major, info.protocol_version.minor);
	printf("Hardware Version = %d.%d\n", info.hardware_version.major, info.hardware_version.minor);
	printf("Software Version = %d.%d\n", info.software_version.major, info.software_version.minor);
	printf("VCS ID = 0x%llX\n", info.software_vcs_revision_id);
	printf("Unique ID = 0x");
	for(uint32_t i = 0; i < 16; i += 1)
	{ printf("%02X", info.unique_id[i]); }
	printf("\n");
	if(info.flags & WLMIO_NODE_INFO_CRC)
	{ printf("Software CRC = 0x%llX\n", info.software_image_crc); }
	else
	{ printf("Software CRC = Not present\n"); }

	return EXIT_SUCCESS;
}
