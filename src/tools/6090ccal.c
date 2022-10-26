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


void calibrate(const uint8_t node_id, const uint8_t ch)
{
	assert(ch > 0 && ch <= 6);

  int32_t ret;
  char reg[51];
  struct wlmio_register_access reg_access;

  snprintf(reg, sizeof(reg), "ch%u.type", ch);

  reg_access.type = WLMIO_REGISTER_VALUE_UINT8;
  reg_access.length = 1;
  memcpy(reg_access.value, &(uint8_t[]){4U}, 1);

  ret = wlmio_register_access_sync(node_id, reg, &reg_access, NULL);
  if(ret < 0)
  {
    printf("Failed to write %s register\n", reg);
    exit(EXIT_FAILURE);
  }

  printf("Apply 20 mV to the input of CH%u and press any key", ch);
  getc(stdin);

  snprintf(reg, sizeof(reg), "ch%u.counts", ch);
  ret = wlmio_register_access_sync(node_id, reg, NULL, &reg_access);
  if(ret < 0)
  {
    printf("Failed to read %s register\n", reg);
    exit(EXIT_FAILURE);
  }

  uint16_t low;
  memcpy(&low, reg_access.value, 2);
  low -= 32768;

  printf("Apply 100 mV to the input of CH%u and press any key", ch);
  getc(stdin);
 
  //snprintf(reg, sizeof(reg), "ch%u.counts", ch);
  ret = wlmio_register_access_sync(node_id, reg, NULL, &reg_access);
  if(ret < 0)
  {
    printf("Failed to read %s register\n", reg);
    exit(EXIT_FAILURE);
  }
 
  uint16_t high;
  memcpy(&high, reg_access.value, 2);
  high -= 32768;

  //printf("Offset of %i counts\n", offset - 32768);

  snprintf(reg, sizeof(reg), "ch%u.cal", ch);
  reg_access.length = 2;
  memcpy(reg_access.value, &low, 2);
  memcpy(reg_access.value + 2, &high, 2);
  ret = wlmio_register_access_sync(node_id, reg, &reg_access, NULL);
  if(ret < 0)
  {
    printf("Failed to write %s register\n", reg);
    exit(EXIT_FAILURE);
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
	
	for(uint8_t i = 1; i <= 6; i += 1)
	{ calibrate(node_id, i); }

	// store settings to eeprom
	int32_t ret = wlmio_execute_command_sync(node_id, 65530, NULL, 0);
	printf("Stored calibration values in EEPROM ret: %d\n", ret);

	return 0;
}
