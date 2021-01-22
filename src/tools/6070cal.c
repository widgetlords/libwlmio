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


void calibrate_10v(const uint8_t node_id, const uint8_t ch)
{
	assert(ch > 0 && ch <= 4);
	
	unsigned int low, high;
	
	uint8_t param = 1;
	int32_t ret = wlmio_execute_command_sync(node_id, 0, &param, 1);
	printf("ret: %d\n", ret);
	printf("Measure the output of CH%u and enter it in mV: ", ch);
	ret = scanf("%u", &low);
	if(ret <= 0)
	{
		printf("Failed to parse value\n");
		exit(1);
	}
	
	param = 2;
	ret = wlmio_execute_command_sync(node_id, 0, &param, 1);
	printf("ret: %d\n", ret);
	printf("Measure the output of CH%u and enter it in mV: ", ch);
	ret = scanf("%u", &high);
	if(ret <= 0)
	{
		printf("Failed to parse value\n");
		exit(1);
	}
	
	uint16_t values[2];
	values[0] = low;
	values[1] = high;

	struct wlmio_register_access reg_access;
	reg_access.type = WLMIO_REGISTER_VALUE_UINT16;
	reg_access.length = 2;
	memcpy(reg_access.value, values, 4);
	
	char reg[51];
	snprintf(reg, sizeof(reg), "ch%u.calibration", ch);
	
	ret = wlmio_register_access_sync(node_id, reg, &reg_access, NULL);
	if(ret < 0)
	{
		printf("Failed to write %s register\n", reg);
		exit(1);
	}
}

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
	
	// 10 volt
	for(uint8_t i = 1; i < 5; i += 1)
	{
		calibrate_10v(node_id, i);
	}

	uint8_t param = 0;
	int32_t ret = wlmio_execute_command_sync(node_id, 0, &param, 1);
	
	// store settings to eeprom
	ret = wlmio_execute_command_sync(node_id, 65530, NULL, 0);
	printf("Stored calibration values in EEPROM ret: %d\n", ret);

	return 0;
}
