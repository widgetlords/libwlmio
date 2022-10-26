#include <stddef.h>
#include <stdint.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <canard.h>
#include <canard_dsdl.h>
#include <gpiod.h>


static void* mem_allocate(CanardInstance* const ins, const size_t amount)
{ return malloc(amount); }

static void mem_free(CanardInstance* const ins, void* const pointer)
{ free(pointer); }

static CanardInstance canard;

static int can_sock = -1;


static int can_socket_init(void)
{
	struct sockaddr_can addr;
	struct ifreq ifr;
	
	int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if(s < 0)
	{ return -1; }
	
	strcpy(ifr.ifr_name, "can0");
	int r = ioctl(s, SIOCGIFINDEX, &ifr);
	if(r < 0)
	{ return -1; }
	
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	
	r = bind(s, (struct sockaddr*)&addr, sizeof(addr));
	if(r < 0)
	{ return -1; }
	
	int canfd = 1;
	r = setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &canfd, sizeof(int));
	if(r < 0)
	{ return -1; }
	
	return s;
}


static int32_t get_node_id(void)
{
	unsigned int offsets[7] = {21, 22, 23, 24, 25, 26, 27};
	int values[7];

	int r = gpiod_ctxless_get_value_multiple("/dev/gpiochip0", offsets, values, 7, false, "fwupdate");
	if(r < 0) { return r; }

	uint8_t node_id = 0;
	node_id |= values[0];
	node_id |= values[1] << 1;
	node_id |= values[2] << 2;
	node_id |= values[3] << 3;
	node_id |= values[4] << 4;
	node_id |= values[5] << 5;
	node_id |= values[6] << 6;

	return node_id;
}


static int uavcan_send(void)
{
	while(1)
	{
		const CanardFrame* const txf = canardTxPeek(&canard);
		if(txf == NULL) { break; }

		canardTxPop(&canard);
			
		struct canfd_frame frame;
		frame.can_id = txf->extended_can_id | 0x80000000UL;
		frame.len = txf->payload_size;
		memcpy(frame.data, txf->payload, txf->payload_size);
			
		write(can_sock, &frame, sizeof(struct canfd_frame));
			
		// free frame after transmission
		canard.memory_free(&canard, (void*)txf);
	}
	
	return 0;
}


static int receive_transfer(CanardTransfer* const tfr)
{
	assert(tfr != NULL);
	
	struct canfd_frame frame;
	int r = read(can_sock, &frame, sizeof(struct canfd_frame));
	
	if(r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
	{ return 0; }
	else if(r < 0)
	{ return -1; }
	
	struct timeval tv;
	ioctl(can_sock, SIOCGSTAMP, &tv);
	
	CanardFrame rxf;
	rxf.timestamp_usec = (tv.tv_sec * 1000000ULL) + tv.tv_usec;
	rxf.extended_can_id = frame.can_id & 0x1FFFFFFFUL;
	rxf.payload_size = frame.len;
	rxf.payload = frame.data;
	
	r = canardRxAccept(&canard, &rxf, 0, tfr);
	if(r <= 0)
	{ return 0; }
	
	return 1;
}


int main(int argc, char** argv)
{
  if(argc != 3)
  {
    fprintf(stderr, "Expected two arguments\n");
    return -1;
  }

  int f = open(argv[2], O_RDWR);
  if(f < 0)
  {
    fprintf(stderr, "Could not open file %s: %s\n", argv[2], strerror(errno));
    return -1;
  }

  // 256K buffer
  uint8_t* const data = malloc(0x40000U);
  if(data == NULL)
  {
    fprintf(stderr, "Could not allocate 256K of memory\n");
    return -1;
  }

  int r = read(f, data, 0x40000U);
  if(r <= 0)
  {
    fprintf(stderr, "Unable to read file %s\n", argv[2]);
    return -1;
  }
  else if(r > 0x38000U)
  {
    fprintf(stderr, "File is too large\n");
    return -1;
  }
  const size_t data_len = r;

  char* endptr;
	errno = 0;
	uint8_t node_id = strtol(argv[1], &endptr, 0);
	if(errno == ERANGE || errno == EINVAL || argv[1] == endptr)
	{
		fprintf(stderr, "Invalid node id\n");
		return -1;
	}
	
	if(node_id > 127)
	{
		fprintf(stderr, "Node ID must between 0 and 127 inclusive\n");
		return -1;
	}

  canard = canardInit(&mem_allocate, &mem_free);
	canard.mtu_bytes = CANARD_MTU_CAN_FD;
	canard.node_id = get_node_id();
	
	can_sock = can_socket_init();
	
	static CanardRxSubscription file_read_subscription;
	canardRxSubscribe(
		&canard,
		CanardTransferKindRequest,
		408,
		261,
		CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
		&file_read_subscription
	);

  {
    uint8_t payload[4] = { 0xFD, 0xFF, 0x01, 0x2F };
    CanardTransfer tfr =
    {
      .timestamp_usec = 0,
      .priority = CanardPriorityNominal,
      .transfer_kind = CanardTransferKindRequest,
      .port_id = 435,
      .remote_node_id = node_id,
      .transfer_id = 0,
      .payload_size = 4,
      .payload = payload
    };
    canardTxPush(&canard, &tfr);
    uavcan_send();
  }

  while(1)
  {
    CanardTransfer tfr;
    if(receive_transfer(&tfr) <= 0)
    { continue; }

    if(tfr.payload_size == 0)
    { continue; }

    size_t payload_offset = 0;
    uint64_t offset = canardDSDLGetU64(tfr.payload, tfr.payload_size, payload_offset << 3, 40);
    payload_offset += 5;

    free((void*)tfr.payload);

    tfr.payload = malloc(260);
    memset((void*)tfr.payload, 0, 260);

    size_t bytes;
    if(offset >= data_len)
    { bytes = 0; }
    else
    {
      bytes = data_len - offset;
      bytes = bytes > 256 ? 256 : bytes;
    }
    printf("%u\n", bytes);
    memcpy((void*)tfr.payload + 2, &bytes, 2);
    memcpy((void*)tfr.payload + 4, data + offset, bytes);

    tfr.transfer_kind = CanardTransferKindResponse;
    tfr.remote_node_id = node_id;
    tfr.payload_size = 4 + bytes;

    canardTxPush(&canard, &tfr);
    free((void*)tfr.payload);

    uavcan_send();
  }

  free(data);
  close(f);
  close(can_sock);

  return 0;
}
