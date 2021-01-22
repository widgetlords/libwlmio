// #define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <sys/unistd.h>

#include <canard.h>
#include <canard_dsdl.h>
#include <gpiod.h>

#include "wlmio.h"

static void* mem_allocate(CanardInstance* const ins, const size_t amount)
{ return malloc(amount); }

static void mem_free(CanardInstance* const ins, void* const pointer)
{ free(pointer); }

static CanardInstance canard;

static int epollfd = -1;

static struct wlmio_status nodes[CANARD_NODE_ID_MAX + 1U];
static int timers[CANARD_NODE_ID_MAX + 1U];

static void (* user_callback)(uint8_t node_id, const struct wlmio_status* old_status, const struct wlmio_status* new_status);


struct fd_entry
{
	int fd;
	void (* handler)(struct fd_entry*);
	struct fd_entry* next;
	struct fd_entry* previous;
};
static struct fd_entry* fd_entry_head = NULL;


static struct fd_entry* fd_entry_find(int fd)
{
	struct fd_entry* c = fd_entry_head;
	while(c)
	{
		if(c->fd == fd)
		{ break; }

		c = c->next;
	}

	return c;
}


static void fd_entry_close(struct fd_entry* const entry)
{
	assert(entry);

	if(entry->previous)
	{ entry->previous->next = entry->next; }
	else
	{ fd_entry_head = entry->next; }

	if(entry->next)
	{ entry->next->previous = entry->previous; }

	close(entry->fd);
	free(entry);
}


static struct fd_entry* fd_entry_add(int fd, void (* const handler)(struct fd_entry*), uint32_t events)
{
	assert(handler);

	struct fd_entry* c = fd_entry_head;
	struct fd_entry* end = NULL;
	while(c)
	{
		if(c->next == NULL)
		{ end = c; }

		c = c->next;
	}

	c = malloc(sizeof(struct fd_entry));
	c->fd = fd;
	c->handler = handler;
	c->previous = end;
	c->next = NULL;

	if(end)
	{ end->next = c; }
	else
	{ fd_entry_head = c; }

	struct epoll_event ev;
  ev.events = events;
  ev.data.ptr = c;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);

	return c;
}


static int can_sock = -1;


static int32_t get_node_id(void)
{
	unsigned int offsets[7] = {21, 22, 23, 24, 25, 26, 27};
	int values[7];

	int r = gpiod_ctxless_get_value_multiple("/dev/gpiochip0", offsets, values, 7, false, "libwlmio");
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


struct task_entry
{
	uint32_t id;
  struct fd_entry* timer;
  void* param;
	void (*callback)(int32_t r, void* uparam);
  void* uparam;
	struct task_entry* next;
};

static struct task_entry* head = NULL;


static void async_remove_entry(struct task_entry* const entry)
{
  assert(entry);
  
  if(!head)
  { return; }

  // close(entry->timer);
	fd_entry_close(entry->timer);

  if(entry == head)
  {
    head = head->next;
    free(entry);
    return;
  }

  struct task_entry* p = head;
  struct task_entry* c = head->next;
  while(c)
  {
    if(c == entry)
    {
      p->next = c->next;
      free(entry);
      return;
    }

    p = c;
    c = c->next;
  }
}


static void async_handler(struct fd_entry* entry)
{
	struct task_entry* c = head;
	while(c)
	{
		if(c->timer == entry)
		{ break; }

		c = c->next;
	}

	c->callback(-ETIMEDOUT, c->uparam);
	async_remove_entry(c);
}


static int32_t async_add(const uint32_t id, const uint64_t timeout, void* const param, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{
  assert(epollfd >= 0);
	assert(callback);

	struct task_entry* const entry = malloc(sizeof(struct task_entry));
  if(!entry)
  { return -ENOMEM; }

  // create and arm timeout timer
  int timer = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if(timer < 0)
  {
    free(entry);
    return errno;
  }

  struct itimerspec it;
  it.it_interval.tv_nsec = 0;
  it.it_interval.tv_sec = 0;
  it.it_value.tv_sec = timeout / 1000000ULL;
  it.it_value.tv_nsec = (timeout - it.it_value.tv_sec * 1000000ULL) * 1000ULL;
  int32_t r = timerfd_settime(timer, 0, &it, NULL);
  if(r < 0)
  {
    free(entry);
    close(timer);
    return errno;
  }

	// register timer with epoll
  // struct epoll_event ev;
  // ev.events = EPOLLIN;
  // ev.data.ptr = entry;
  // r = epoll_ctl(epollfd, EPOLL_CTL_ADD, timer, &ev);
  // if(r < 0)
  // {
  //   free(entry);
  //   close(timer);
  //   return errno;
  // }
	struct fd_entry* fd_entry = fd_entry_add(timer, async_handler, EPOLLIN);

  *entry = (struct task_entry)
  {
    .id = id,
    .timer = fd_entry,
    .param = param,
    .callback = callback,
    .uparam = uparam,
    .next = NULL
  };

  // append entry
  if(!head)
  {
    head = entry;
    return 0;
  }

  struct task_entry* c = head;
  while(c->next)
  { c = c->next; }

  c->next = entry;

  return 0;
}


static void async_tick(void)
{
  struct task_entry* c = head;
  while(c)
  {
    uint64_t e;
    int r = read(c->timer->fd, &e, sizeof(e));
    if(r > 0)
    {
      c->callback(-ETIMEDOUT, c->uparam);
      struct task_entry* t = c;
      c = c->next;
      async_remove_entry(t);
      continue;
    }

    c = c->next;
  }
}


static void async_complete(const uint32_t id, const int32_t r)
{
  struct task_entry* c = head;
  while(c)
  {
    if(c->id == id)
    {
      c->callback(r, c->uparam);
      async_remove_entry(c);
      return;
    }

    c = c->next;
  }
}


static int32_t async_get_param(const uint32_t id, void** const param)
{
  if(param == NULL)
  { return -EINVAL; }

  struct task_entry* c = head;
  while(c)
  {
    if(c->id == id)
    {
      *param = c->param;
      return 0;
    }

    c = c->next;
  }

  return -ENOENT;
}


int32_t wlmio_shutdown(void)
{
	close(can_sock);
	can_sock = -1;

	close(epollfd);
	epollfd = -1;

	return 0;
}


static int uavcan_send(void)
{
	while(1)
	{
		const CanardFrame* const txf = canardTxPeek(&canard);
		if(txf == NULL) { break; }
			
		struct canfd_frame frame;
		frame.can_id = txf->extended_can_id | 0x80000000UL;
		frame.len = txf->payload_size;
		memcpy(frame.data, txf->payload, txf->payload_size);
			
		// write(can_sock, &frame, sizeof(struct canfd_frame));
		int r = send(can_sock, &frame, sizeof(struct canfd_frame), MSG_DONTWAIT);

		// if(r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
		if(r < 0)
		{ break; }

		// free frame after transmission
		canardTxPop(&canard);
		canard.memory_free(&canard, (void*)txf);
	}
	
	return 0;
}


static int receive_transfer(CanardTransfer* const tfr)
{
	assert(tfr != NULL);
	
	struct canfd_frame frame;
	int r = recv(can_sock, &frame, sizeof(struct canfd_frame), MSG_DONTWAIT);
	
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


static uint32_t make_rsp_specifier(const CanardTransfer* const tfr)
{
	return
	(tfr->remote_node_id & 0x7F) |
	((tfr->transfer_id & 0x1F) << 7) |
	((tfr->port_id & 0x1FF) << 12);
}


static void get_node_info_response_handler(const CanardTransfer* const tfr)
{
	const uint32_t id = make_rsp_specifier(tfr);

	struct wlmio_node_info* node_info;
	int32_t r = async_get_param(id, (void**)&node_info);
	if(r < 0)
	{ goto exit; }

	memset(node_info, 0, sizeof(struct wlmio_node_info));

	if(tfr->payload_size > 0)
	{
		size_t payload_offset = 0;

		node_info->protocol_version.major = canardDSDLGetU8(tfr->payload, tfr->payload_size, payload_offset << 3, 8);
		payload_offset += 1;

		node_info->protocol_version.minor = canardDSDLGetU8(tfr->payload, tfr->payload_size, payload_offset << 3, 8);
		payload_offset += 1;

		node_info->hardware_version.major = canardDSDLGetU8(tfr->payload, tfr->payload_size, payload_offset << 3, 8);
		payload_offset += 1;

		node_info->hardware_version.minor = canardDSDLGetU8(tfr->payload, tfr->payload_size, payload_offset << 3, 8);
		payload_offset += 1;

		node_info->software_version.major = canardDSDLGetU8(tfr->payload, tfr->payload_size, payload_offset << 3, 8);
		payload_offset += 1;

		node_info->software_version.minor = canardDSDLGetU8(tfr->payload, tfr->payload_size, payload_offset << 3, 8);
		payload_offset += 1;

		node_info->software_vcs_revision_id = canardDSDLGetU64(tfr->payload, tfr->payload_size, payload_offset << 3, 64);
		payload_offset += 8;

		if(tfr->payload_size > payload_offset)
		{
			size_t bytes = tfr->payload_size - payload_offset;
			bytes = bytes > 16 ? 16 : bytes;
			memcpy(node_info->unique_id, tfr->payload + payload_offset, bytes);
		}
		payload_offset += 16;

		const uint8_t name_len = canardDSDLGetU8(tfr->payload, tfr->payload_size, payload_offset << 3, 8);
		payload_offset += 1;
		if(name_len > 50)
		{
			r = -EPROTO;
			goto exit;
		}

		if(tfr->payload_size > payload_offset)
		{
			size_t bytes = tfr->payload_size - payload_offset;
			bytes = bytes > name_len ? name_len : bytes;
			memcpy(node_info->name, tfr->payload + payload_offset, bytes);
		}
		payload_offset += name_len;

		const uint8_t crc_present = canardDSDLGetU8(tfr->payload, tfr->payload_size, payload_offset << 3, 8);
		payload_offset += 1;
		if(crc_present > 1)
		{
			r = -EPROTO;
			goto exit;
		}
		if(crc_present)
		{
			node_info->flags |= WLMIO_NODE_INFO_CRC;
			node_info->software_image_crc = canardDSDLGetU64(tfr->payload, tfr->payload_size, payload_offset << 3, 64);
			payload_offset += 8;
		}

		const uint8_t coa_len = canardDSDLGetU8(tfr->payload, tfr->payload_size, payload_offset << 3, 8);
		payload_offset += 1;
		if(coa_len > 222)
		{
			r = -EPROTO;
			goto exit;
		}
		if(coa_len)
		{
			node_info->flags |= WLMIO_NODE_INFO_CERTIFICATE_OF_AUTHENTICITY;
			if(tfr->payload_size > payload_offset)
			{
				size_t bytes = tfr->payload_size - payload_offset;
				bytes = bytes > coa_len ? coa_len : bytes;
				memcpy(node_info->certificate_of_authenticity, tfr->payload + payload_offset, bytes);
			}
		}
		payload_offset += coa_len;
	}

	r = 0;

exit:
	async_complete(id, r);
}


static void register_list_response_handler(const CanardTransfer* const tfr)
{
	const uint32_t id = make_rsp_specifier(tfr);

	char* name;
	int32_t r = async_get_param(id, (void**)&name);
	if(r < 0) { return; }

	uint8_t name_len = 0;
	memset(name, 0, 51);
	
	if(tfr->payload_size > 0)
	{
		name_len = canardDSDLGetU8(tfr->payload, tfr->payload_size, 0, 8);
		name_len = name_len > 50 ? 50 : name_len;
	
		if(tfr->payload_size > 1)
		{ memcpy(name, tfr->payload + 1, tfr->payload_size - 1); }
	}

	async_complete(id, 0);
}


static int32_t validate_register_access(const struct wlmio_register_access* const reg)
{
	if(reg == NULL || reg->type > WLMIO_REGISTER_VALUE_FLOAT16)
	{ return -EINVAL; }

	size_t max_length;
	switch(reg->type)
	{
		case WLMIO_REGISTER_VALUE_EMPTY:
		default:
			max_length = UINT16_MAX;
			break;

		case WLMIO_REGISTER_VALUE_STRING:
		case WLMIO_REGISTER_VALUE_UNSTRUCTURED:
		case WLMIO_REGISTER_VALUE_INT8:
		case WLMIO_REGISTER_VALUE_UINT8:
			max_length = 256;
			break;

		case WLMIO_REGISTER_VALUE_BIT:
			max_length = 2048;
			break;

		case WLMIO_REGISTER_VALUE_INT64:
		case WLMIO_REGISTER_VALUE_UINT64:
		case WLMIO_REGISTER_VALUE_FLOAT64:
			max_length = 32;
			break;

		case WLMIO_REGISTER_VALUE_INT32:
		case WLMIO_REGISTER_VALUE_UINT32:
		case WLMIO_REGISTER_VALUE_FLOAT32:
			max_length = 64;
			break;

		case WLMIO_REGISTER_VALUE_INT16:
		case WLMIO_REGISTER_VALUE_UINT16:
		case WLMIO_REGISTER_VALUE_FLOAT16:
			max_length = 128;
			break;
	}

	if(reg->length > max_length)
	{ return -EINVAL; }

	return 0;
}


static const uint8_t register_value_bit_width[15] = {0U, 8U, 8U, 1U, 64U, 32U, 16U, 8U, 64U, 32U, 16U, 8U, 64U, 32U, 16U};
static const uint8_t register_value_length_width[15] = {0U, 2U, 2U, 2U, 1U, 1U, 1U, 2U, 1U, 1U, 1U, 2U, 1U, 1U, 1U};


static void register_access_response_handler(const CanardTransfer* const tfr)
{
	const uint32_t id = make_rsp_specifier(tfr);

	struct wlmio_register_access* uregr;
	int32_t r = async_get_param(id, (void**)&uregr);
	if(r < 0) { return; }

	struct wlmio_register_access regr =
	{
		.type = WLMIO_REGISTER_VALUE_EMPTY,
		.length = 0U,
		.value = { 0 }
	};
	
	// skip timestamp and boolean values
	size_t payload_offset = 8;
	
	regr.type = canardDSDLGetU8(tfr->payload, tfr->payload_size, payload_offset << 3, 8);
	payload_offset += 1;
	
  regr.length = canardDSDLGetU16(tfr->payload, tfr->payload_size, payload_offset << 3, register_value_length_width[regr.type] << 3);
	payload_offset += register_value_length_width[regr.type];

	if(payload_offset < tfr->payload_size)
  { memcpy(regr.value, tfr->payload + payload_offset, tfr->payload_size - payload_offset); }

	if(validate_register_access(&regr) != 0)
	{
		r = -EPROTO;
		goto exit1;
	}
	
	if(regr.type == WLMIO_REGISTER_VALUE_EMPTY)
	{
		r = -ENOENT;
		goto exit1;
	}

	r = 0;

exit1:
	if(uregr)
	{ *uregr = regr; }

	async_complete(id, r);
}


static void execute_command_response_handler(const CanardTransfer* const tfr)
{
	const uint32_t id = make_rsp_specifier(tfr);
	
	uint8_t status = WLMIO_COMMAND_STATUS_SUCCESS;
	if(tfr->payload_size > 0)
	{
		status = canardDSDLGetU8(tfr->payload, tfr->payload_size, 0, 8);
	}
	
	async_complete(id, status);
}


static void heartbeat_timeout_handler(struct fd_entry* const entry)
{
	uint_fast8_t node_id;
	struct wlmio_status* node;
	for(uint_fast8_t i = 0; i < CANARD_NODE_ID_MAX; i += 1)
	{
		if(timers[i] == entry->fd)
		{
			node_id = i;
			node = &nodes[node_id];
			break;
		}
	}

	fd_entry_close(entry);
	timers[node_id] = -1;

	const struct wlmio_status old_status = *node;
	*node = (struct wlmio_status)
	{
		.uptime = 0,
		.health = WLMIO_HEALTH_NOMINAL,
		.mode = WLMIO_MODE_OFFLINE,
		.vendor_status = 0
	};

	if(user_callback)
	{ user_callback(node_id, &old_status, node); }
}


static void heartbeat_handler(const CanardTransfer* const tfr)
{
  assert(tfr);

  const uint8_t node_id = tfr->remote_node_id;

  if(tfr->payload_size == 0 || node_id == CANARD_NODE_ID_UNSET)
  { return; }

  assert(node_id <= CANARD_NODE_ID_MAX);

  struct wlmio_status* const status = &nodes[node_id];
  const struct wlmio_status old_status = *status;

  size_t payload_offset = 0;

  status->uptime = canardDSDLGetU32(tfr->payload, tfr->payload_size, payload_offset, 32);
  payload_offset += 32;

  status->health = canardDSDLGetU8(tfr->payload, tfr->payload_size, payload_offset, 2);
  payload_offset += 8;

  status->mode = canardDSDLGetU8(tfr->payload, tfr->payload_size, payload_offset, 3);
  payload_offset += 8;

  status->vendor_status = canardDSDLGetU8(tfr->payload, tfr->payload_size, payload_offset, 8);
  payload_offset += 8;

  // stop timeout timer if node is going offline
  if(status->mode == WLMIO_MODE_OFFLINE && timers[node_id] >= 0)
  {
		fd_entry_close(fd_entry_find(timers[node_id]));
    timers[node_id] = -1;
  }
  
  // create timeout timer if node is coming online
  if(status->mode != WLMIO_MODE_OFFLINE && timers[node_id] < 0)
  {
    timers[node_id] = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    // register timer with epoll
    // struct epoll_event ev;
    // ev.events = EPOLLIN;
    // ev.data.ptr = status;
    // epoll_ctl(epollfd, EPOLL_CTL_ADD, timers[node_id], &ev);
		fd_entry_add(timers[node_id], heartbeat_timeout_handler, EPOLLIN);
  }

  // reset timeout timer if node is still online
  if(status->mode != WLMIO_MODE_OFFLINE && timers[node_id] >= 0)
  {
    struct itimerspec it;
    it.it_interval.tv_nsec = 0;
    it.it_interval.tv_sec = 0;
    it.it_value.tv_sec = 3U;
    it.it_value.tv_nsec = 0U;
    timerfd_settime(timers[node_id], 0, &it, NULL);
  }

  if(user_callback)
  { user_callback(node_id, &old_status, status); }
}


static void can_socket_handler(struct fd_entry* const entry)
{
	struct canfd_frame frame;
	int32_t r = read(entry->fd, &frame, sizeof(struct canfd_frame));
	if(r < 0)
	{ return; }
	
	struct timeval tv;
	ioctl(entry->fd, SIOCGSTAMP, &tv);
	
	CanardFrame rxf;
	rxf.timestamp_usec = (tv.tv_sec * 1000000ULL) + tv.tv_usec;
	rxf.extended_can_id = frame.can_id & 0x1FFFFFFFUL;
	rxf.payload_size = frame.len;
	rxf.payload = frame.data;
	
	CanardTransfer tfr;
	r = canardRxAccept(&canard, &rxf, 0, &tfr);
	if(r <= 0)
	{ return; }
	
	if(tfr.port_id == 7509 && tfr.transfer_kind == CanardTransferKindMessage)
	{ heartbeat_handler(&tfr); }
	
	else if(tfr.port_id == 430 && tfr.transfer_kind == CanardTransferKindResponse)
	{ get_node_info_response_handler(&tfr); }

	else if(tfr.port_id == 385 && tfr.transfer_kind == CanardTransferKindResponse)
	{ register_list_response_handler(&tfr); }

	else if(tfr.port_id == 384 && tfr.transfer_kind == CanardTransferKindResponse)
	{ register_access_response_handler(&tfr); }

	else if(tfr.port_id == 435 && tfr.transfer_kind == CanardTransferKindResponse)
	{ execute_command_response_handler(&tfr); }
	
	// free transfer payload after processing if there is one
	if(tfr.payload_size > 0 && tfr.payload != NULL) { free((void*)tfr.payload); }
}


static int can_socket_init(void)
{
	struct sockaddr_can addr;
	struct ifreq ifr;
	
	int s = socket(PF_CAN, SOCK_RAW | SOCK_CLOEXEC, CAN_RAW);
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

	// register CAN socket with epoll
  // struct epoll_event ev;
  // ev.events = EPOLLIN;
  // ev.data.fd = s;
  // r = epoll_ctl(epollfd, EPOLL_CTL_ADD, s, &ev);
  // if(r < 0)
  // { return -1; }
	fd_entry_add(s, can_socket_handler, EPOLLIN);
	
	return s;
}


int32_t wlmio_init(void)
{
	canard = canardInit(&mem_allocate, &mem_free);
	canard.mtu_bytes = CANARD_MTU_CAN_FD;
	
	const int32_t node_id = get_node_id();
	if(node_id < 0) { return -1; }
	canard.node_id = node_id;

	epollfd = epoll_create1(0);
	if(epollfd < 0)
	{	return -1; }
	
	can_sock = can_socket_init();
	
	static CanardRxSubscription register_list_subscription;
	canardRxSubscribe(
		&canard,
		CanardTransferKindResponse,
		385,
		51,
		CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
		&register_list_subscription
	);
	
	static CanardRxSubscription register_access_subscription;
	canardRxSubscribe(
		&canard,
		CanardTransferKindResponse,
		384,
		267,
		CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
		&register_access_subscription
	);

	static CanardRxSubscription command_subscription;
  canardRxSubscribe(
    &canard,
    CanardTransferKindResponse,
    435,
    1,
    CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
    &command_subscription
  );

	static CanardRxSubscription node_info_subscription;
  canardRxSubscribe(
    &canard,
    CanardTransferKindResponse,
    430,
    313,
    CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
    &node_info_subscription
  );
  
  static CanardRxSubscription heartbeat_subscription;
  canardRxSubscribe(
    &canard,
    CanardTransferKindMessage,
    7509,
    7,
    CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
    &heartbeat_subscription
  );
  
  // init node status
  for(uint_fast8_t i = 0; i <= CANARD_NODE_ID_MAX; i += 1)
  {
    nodes[i] = (struct wlmio_status)
    {
      .uptime = 0,
      .health = WLMIO_HEALTH_NOMINAL,
      .mode = WLMIO_MODE_OFFLINE,
      .vendor_status = 0
    };

		timers[i] = -1;
  }
	
	return can_sock < 0 ? -1 : 0;
}


int32_t wlmio_tick(void)
{	
	// while(1)
	// {
	// 	CanardTransfer tfr;
	// 	int r = receive_transfer(&tfr);
	// 	if(r <= 0)
	// 	{ break; }
		
	// 	if(tfr.port_id == 7509 && tfr.transfer_kind == CanardTransferKindMessage)
	// 	{ heartbeat_handler(&tfr); }
		
	// 	else if(tfr.port_id == 430 && tfr.transfer_kind == CanardTransferKindResponse)
	// 	{ get_node_info_response_handler(&tfr); }

	// 	else if(tfr.port_id == 385 && tfr.transfer_kind == CanardTransferKindResponse)
	// 	{ register_list_response_handler(&tfr); }

	// 	else if(tfr.port_id == 384 && tfr.transfer_kind == CanardTransferKindResponse)
	// 	{ register_access_response_handler(&tfr); }

	// 	else if(tfr.port_id == 435 && tfr.transfer_kind == CanardTransferKindResponse)
	// 	{ execute_command_response_handler(&tfr); }
		
	// 	// free transfer payload after processing if there is one
	// 	if(tfr.payload_size > 0 && tfr.payload != NULL) { free((void*)tfr.payload); }
	// }

	while(1)
	{
		struct epoll_event ev;
		int32_t r = epoll_wait(epollfd, &ev, 1, 0);
		if(r <= 0)
		{ break; }

		struct fd_entry* entry = ev.data.ptr;
		entry->handler(entry);
	}

	uavcan_send();

	// async_tick();
	
	// handle heartbeat timeouts
	// for(uint_fast8_t i = 0; i <= CANARD_NODE_ID_MAX; i += 1)
  // {
  //   if(timers[i] < 0)
  //   { continue; }

  //   uint64_t e;
  //   int r = read(timers[i], &e, sizeof(e));
  //   if(r == -1 && errno == EAGAIN)
  //   { continue; }

  //   close(timers[i]);
  //   timers[i] = -1;

  //   const struct wlmio_status old_status = nodes[i];
  //   nodes[i] = (struct wlmio_status)
  //   {
  //     .uptime = 0,
  //     .health = WLMIO_HEALTH_NOMINAL,
  //     .mode = WLMIO_MODE_OFFLINE,
  //     .vendor_status = 0
  //   };

  //   if(user_callback)
  //   { user_callback(i, &old_status, &nodes[i]); }
  // }
	
	return 0;
}


// timeout in microseconds
static uint64_t timeout = 2000000ULL;


int32_t wlmio_get_node_info(const uint8_t node_id, struct wlmio_node_info* const node_info, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{
	static uint8_t tfr_ids[128] = { 0 };

	if(node_id > CANARD_NODE_ID_MAX || node_info == NULL || callback == NULL)
	{ return -EINVAL; }

	const CanardTransfer tfr_tx =
	{
		.timestamp_usec = 0,
		.priority = CanardPriorityNominal,
		.transfer_kind = CanardTransferKindRequest,
		.port_id = 430,
		.remote_node_id = node_id,
		.transfer_id = tfr_ids[node_id],
		.payload_size = 0,
		.payload = NULL
	};
	canardTxPush(&canard, &tfr_tx);

	tfr_ids[node_id] = (tfr_ids[node_id] + 1) & 0x1F;

	async_add(make_rsp_specifier(&tfr_tx), timeout, node_info, callback, uparam);
	uavcan_send();

	return 0;
}


void wlmio_set_timeout(const uint64_t us)
{
	timeout = us;
}


void wlmio_wait_for_event(void)
{
	struct epoll_event ev;
	epoll_wait(epollfd, &ev, 1, -1);
	// int n = epoll_wait(epollfd, &ev, 1, -1);
	// if(n <= 0)
	// { return;	}

	// wlmio_tick();
}


int32_t wlmio_register_list(const uint8_t node_id, const uint16_t index, char* const name, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{	
	static uint8_t tfr_ids[128] = { 0 };
	
	if(node_id > CANARD_NODE_ID_MAX || name == NULL) { return -EINVAL; }
	
	const CanardTransfer tfr_tx =
	{
		.timestamp_usec = 0,
		.priority = CanardPriorityNominal,
		.transfer_kind = CanardTransferKindRequest,
		.port_id = 385,
		.remote_node_id = node_id,
		.transfer_id = tfr_ids[node_id],
		.payload_size = 2,
		.payload = &index
	};
	canardTxPush(&canard, &tfr_tx);
	
	tfr_ids[node_id] = (tfr_ids[node_id] + 1) & 0x1F;

	async_add(make_rsp_specifier(&tfr_tx), timeout, name, callback, uparam);
	uavcan_send();
	
	return 0;
}


int32_t wlmio_register_access(const uint8_t node_id, const char* const name, const struct wlmio_register_access* const regw, struct wlmio_register_access* const regr, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{	
	static uint8_t tfr_ids[128] = { 0 };

	int32_t r;
	
	if(
		node_id >= CANARD_NODE_ID_MAX
		|| name == NULL
		|| name[0] == '\0'
		|| (regw == NULL && regr == NULL)
		|| (regw != NULL && validate_register_access(regw) != 0)
	)
	{
		r = -EINVAL;
		goto exit1;
	}
	
	const uint8_t name_len = strnlen(name, 50);
	
	void* const payload = malloc(310);
	size_t payload_offset = 0;
	
	memcpy(payload + payload_offset, &name_len, 1);
	payload_offset += 1;
	memcpy(payload + payload_offset, name, name_len);
	payload_offset += name_len;
	
	if(regw != NULL)
	{ memcpy(payload + payload_offset, &regw->type, 1); }
	else
	{ memset(payload + payload_offset, WLMIO_REGISTER_VALUE_EMPTY, 1); }
	payload_offset += 1;
	
	if(regw != NULL && regw->type != WLMIO_REGISTER_VALUE_EMPTY)
	{
		// array length
		memcpy(payload + payload_offset, &regw->length, register_value_length_width[regw->type]);
		payload_offset += register_value_length_width[regw->type];
		
		// register value
		const size_t bytes = (regw->length * register_value_bit_width[regw->type]) >> 3;
		assert(bytes <= 256);
		memcpy(payload + payload_offset, regw->value, bytes);
		payload_offset += bytes;
	}
	
	const CanardTransfer tfr_tx =
	{
		.timestamp_usec = 0,
		.priority = CanardPriorityNominal,
		.transfer_kind = CanardTransferKindRequest,
		.port_id = 384,
		.remote_node_id = node_id,
		.transfer_id = tfr_ids[node_id],
		.payload_size = payload_offset,
		.payload = payload
	};
	canardTxPush(&canard, &tfr_tx);
	tfr_ids[node_id] = (tfr_ids[node_id] + 1) & 0x1F;
	
	free(payload);
	
	async_add(make_rsp_specifier(&tfr_tx), timeout, regr, callback, uparam);
	uavcan_send();

	r = 0;

exit1:
	return r;
}


int32_t wlmio_execute_command(const uint8_t node_id, const uint16_t command, const void* const param, size_t param_len, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{	
	static uint8_t tfr_ids[128] = { 0 };
	
	if(node_id > CANARD_NODE_ID_MAX || (param == NULL && param_len > 0)) { return -EINVAL; }
	
	void* const payload = malloc(115);
	if(payload == NULL) { return -ENOMEM; }

	size_t payload_offset = 0;

	memcpy(payload + payload_offset, &command, 2);
	payload_offset += 2;

	param_len = param_len > 112 ? 112 : param_len;
	memcpy(payload + payload_offset, &param_len, 1);
	payload_offset += 1;

	if(param_len > 0)
	{
		memcpy(payload + payload_offset, param, param_len);
		payload_offset += param_len;
	}

	const CanardTransfer tfr_tx =
	{
		.timestamp_usec = 0,
		.priority = CanardPriorityNominal,
		.transfer_kind = CanardTransferKindRequest,
		.port_id = 435,
		.remote_node_id = node_id,
		.transfer_id = tfr_ids[node_id],
		.payload_size = payload_offset,
		.payload = payload
	};
	canardTxPush(&canard, &tfr_tx);
	free(payload);
	
	tfr_ids[node_id] = (tfr_ids[node_id] + 1) & 0x1F;

	async_add(make_rsp_specifier(&tfr_tx), timeout, NULL, callback, uparam);
	uavcan_send();
	
	return 0;
}


void wlmio_set_status_callback(void (* const callback)(uint8_t node_id, const struct wlmio_status* old_status, const struct wlmio_status* new_status))
{
  user_callback = callback;
}


int64_t wlmio_get_epoll_fd(void)
{
	return epollfd;
}


uint8_t wlmio_get_node_id(void)
{
	assert(epollfd >= 0);
	return canard.node_id;
}
