#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/poll.h>
#include <sys/time.h>
#include <sys/unistd.h>

#include <wlmio.h>

void print_usage_and_exit(char* const argv[])
{
	fprintf(stderr, "Usage: %s id [name] [type] [value] ...\n\n", argv[0]);
	fprintf(stderr, "  id: Node ID, must be between 0 and 127 inclusive.\n");
	fprintf(stderr, "  name: Register name, must be 50 ASCII characters or less.\n");
	fprintf(stderr, "  type: Register type, must be between 1 and 14 inclusive.\n");
	fprintf(stderr, "  value: Register value.\n");
	exit(EXIT_FAILURE);
}

enum
{
	MODE_DUMP = 0U,
	MODE_READ = 1U,
	MODE_WRITE = 2U
};

uint8_t mode;
uint8_t node_id;
char name[51];
struct wlmio_register_access reg_write;


void parse_value(const int argc, char* const argv[])
{
	if(reg_write.type == WLMIO_REGISTER_VALUE_UINT8)
	{
		uint32_t i = 0;
		for(; i < argc; i += 1)
		{
			char* endptr;
			errno = 0;
			uint8_t value = strtol(argv[i], &endptr, 0);
			if(errno == ERANGE || errno == EINVAL || argv[i] == endptr)
			{
				fprintf(stderr, "Invalid value\n");
				print_usage_and_exit(argv);
			}
			reg_write.value[i] = value;
		}

		reg_write.length = i;
	}
	else if(reg_write.type == WLMIO_REGISTER_VALUE_UINT16)
	{
		uint32_t i = 0;
		for(; i < argc; i += 1)
		{
			char* endptr;
			errno = 0;
			uint16_t value = strtol(argv[i], &endptr, 0);
			if(errno == ERANGE || errno == EINVAL || argv[i] == endptr)
			{
				fprintf(stderr, "Invalid value\n");
				print_usage_and_exit(argv);
			}
			memcpy(reg_write.value + 2 * i, &value, 2);
		}

		reg_write.length = i;
	}
}


void parse_arguments(const int argc, char* const argv[])
{
	if(argc == 2) { mode = MODE_DUMP; }
	else if(argc == 3) { mode = MODE_READ; }
	else if(argc >= 5) { mode = MODE_WRITE; }
	else
	{
		fprintf(stderr, "Incorrect number of arguments\n");
		print_usage_and_exit(argv);
	}
	
	char* endptr;

	// parse node id
	errno = 0;
	node_id = strtol(argv[1], &endptr, 0);
	if(errno == ERANGE || errno == EINVAL || argv[1] == endptr)
	{
		fprintf(stderr, "Invalid node id\n");
		print_usage_and_exit(argv);
	}
	
	if(node_id > 127)
	{
		fprintf(stderr, "Node ID must be between 0 and 127 inclusive\n");
		exit(EXIT_FAILURE);
	}

	if(mode == MODE_DUMP) { return; }

	// parse name
	memset(name, 0, 51);
	strncpy(name, argv[2], 50);

	if(mode == MODE_READ) { return; }

	// parse type
	errno = 0;
	reg_write.type = strtol(argv[3], &endptr, 0);
	if(errno == ERANGE || errno == EINVAL || argv[3] == endptr || reg_write.type < 1 || reg_write.type > 14)
	{
		fprintf(stderr, "Invalid type\n");
		print_usage_and_exit(argv);
	}

	// parse value
	parse_value(argc - 4, &argv[4]);
}


void print_register(const struct wlmio_register_access* const reg)
{
	assert(reg != NULL);

	// printf("%-50s, %2d, [ ", name, value.type);
	printf("%s, %2d, [ ", name, reg->type);

	if(reg->type == WLMIO_REGISTER_VALUE_UINT32)
	{
		uint32_t uint32[64];
		memcpy(uint32, reg->value, reg->length * 4);
		
		for(uint32_t i = 0; i < reg->length; i += 1)
		{
			printf("%u", uint32[i]);
			if(i < reg->length - 1)
			{ printf(", "); }
		}
	}
	else if(reg->type == WLMIO_REGISTER_VALUE_UINT16)
	{
		uint16_t uint16[128];
		memcpy(uint16, reg->value, reg->length * 2);
		
		for(uint32_t i = 0; i < reg->length; i += 1)
		{
			printf("%u", uint16[i]);
			if(i < reg->length - 1)
			{ printf(", "); }
		}
	}
	else if(reg->type == WLMIO_REGISTER_VALUE_UINT8)
	{
		uint8_t uint8[256];
		memcpy(uint8, reg->value, reg->length);
		
		for(uint32_t i = 0; i < reg->length; i += 1)
		{
			printf("%u", uint8[i]);
			if(i < reg->length - 1)
			{ printf(", "); }
		}
	}
  else if(reg->type == WLMIO_REGISTER_VALUE_FLOAT32)
  {
    float float32[64];
    memcpy(float32, reg->value, reg->length * 4);

    for(uint32_t i = 0; i < reg->length; i += 1)
    {
      printf("%f", float32[i]);
      if(i < reg->length - 1)
      { printf(", "); }
    }
  }

	printf(" ]\n");
}


void dump_registers(void)
{
	uint16_t i = 0;
	while(1)
	{
		int r = wlmio_register_list_sync(node_id, i, name);
		if(r < 0)
		{
			fprintf(stderr, "Error listing registers: %d\n", r);
			break;
		}
		else if(name[0] == 0) { break; }

		struct wlmio_register_access value;
		r = wlmio_register_access_sync(node_id, name, NULL, &value);
		if(r < 0)
		{
			fprintf(stderr, "Error accessing register: %d\n", r);
			break;
		}
		
		print_register(&value);
		
		if(i == UINT16_MAX) { break; }
		i += 1;
	}
}


void read_register(void)
{
	struct wlmio_register_access reg;
	int r = wlmio_register_access_sync(node_id, name, NULL, &reg);
	if(r < 0)
	{
		fprintf(stderr, "Error accessing register: %d\n", r);
		exit(EXIT_FAILURE);
	}

	if(reg.type == WLMIO_REGISTER_VALUE_EMPTY)
	{
		fprintf(stderr, "Register not present on node\n");
		exit(EXIT_FAILURE);
	}
	else
	{ print_register(&reg); }
}


void write_register(void)
{
	struct wlmio_register_access read;
	int r = wlmio_register_access_sync(node_id, name, &reg_write, &read);
	if(r < 0)
	{
		fprintf(stderr, "Error accessing register: %d\n", r);
		exit(EXIT_FAILURE);
	}

	if(read.type == WLMIO_REGISTER_VALUE_EMPTY)
	{
		fprintf(stderr, "Register not present on node\n");
		exit(EXIT_FAILURE);
	}
	else
	{ print_register(&read); }
}


int main(const int argc, char* const argv[])
{	
	parse_arguments(argc, argv);

	// initialize libwlmio
	if(wlmio_init() < 0)
	{
		fprintf(stderr, "Failed to initialize libwlmio\n");
		exit(EXIT_FAILURE);
	}
	
	switch(mode)
	{
		case MODE_DUMP:
			dump_registers();
			break;

		case MODE_READ:
			read_register();
			break;

		case MODE_WRITE:
			write_register();
			break;
	}

	return 0;
}
