#include <stddef.h>
#include <stdint.h>

#include <errno.h>
#include <string.h>

#include "wlmio.h"


static uint8_t sync_flag = 0;
static int32_t sync_return;
static void sync_callback(const int32_t r, void* const uparam)
{
	sync_return = r;
	sync_flag = 1;
}


int32_t wlmio_get_node_info_sync(const uint8_t node_id, struct wlmio_node_info* const node_info)
{
	sync_flag = 0;

	int32_t r = wlmio_get_node_info(node_id, node_info, &sync_callback, NULL);
	if(r < 0)
	{ goto exit; }

	while(!sync_flag)
	{
    wlmio_wait_for_event();
    wlmio_tick();
  }

	r = sync_return;

exit:
	return r;
}


int32_t wlmio_register_list_sync(const uint8_t node_id, const uint16_t index, char* const name)
{
	sync_flag = 0;

	int32_t r = wlmio_register_list(node_id, index, name, &sync_callback, NULL);
	if(r < 0)
	{ goto exit; }

	while(!sync_flag)
	{
    wlmio_wait_for_event();
    wlmio_tick();
  }

	r = sync_return;

exit:
	return r;
}


int32_t wlmio_register_access_sync(const uint8_t node_id, const char* const name, const struct wlmio_register_access* const regw, struct wlmio_register_access* const regr)
{
	sync_flag = 0;

	int32_t r = wlmio_register_access(node_id, name, regw, regr, &sync_callback, NULL);
	if(r < 0)
	{ goto exit; }

	while(!sync_flag)
	{
    wlmio_wait_for_event();
    wlmio_tick();
  }

	r = sync_return;

exit:
	return r;
}


int32_t wlmio_execute_command_sync(const uint8_t node_id, const uint16_t command, const void* const param, size_t param_len)
{
	sync_flag = 0;

	int32_t r = wlmio_execute_command(node_id, command, param, param_len, &sync_callback, NULL);
	if(r < 0)
	{ goto exit; }

	while(!sync_flag)
	{
    wlmio_wait_for_event();
    wlmio_tick();
  }

	r = sync_return;

exit:
	return r;
}


int32_t wlmio_vpe6010_read_sync(const uint8_t node_id, struct wlmio_vpe6010_input* const dst)
{
  sync_flag = 0;

	int32_t r = wlmio_vpe6010_read(node_id, dst, sync_callback, NULL);
	if(r < 0)
	{ goto exit; }

	while(!sync_flag)
	{
    wlmio_wait_for_event();
    wlmio_tick();
  }

	r = sync_return;

exit:
	return r;
}


int32_t wlmio_vpe6030_write_sync(const uint8_t node_id, const uint8_t ch, const uint8_t v)
{
	sync_flag = 0;

	int32_t r = wlmio_vpe6030_write(node_id, ch, v, sync_callback, NULL);
	if(r < 0)
	{ goto exit; }

	while(!sync_flag)
	{
    wlmio_wait_for_event();
    wlmio_tick();
  }

	r = sync_return;

exit:
	return r;
}


int32_t wlmio_vpe6040_read_sync(const uint8_t node_id, const uint8_t ch, uint16_t* const v)
{
  sync_flag = 0;

	int32_t r = wlmio_vpe6040_read(node_id, ch, v, sync_callback, NULL);
	if(r < 0)
	{ goto exit; }

	while(!sync_flag)
	{
    wlmio_wait_for_event();
    wlmio_tick();
  }

	r = sync_return;

exit:
	return r;
}


int32_t wlmio_vpe6040_configure_sync(const uint8_t node_id, const uint8_t ch, const uint8_t mode)
{
	sync_flag = 0;

	int32_t r = wlmio_vpe6040_configure(node_id, ch, mode, sync_callback, NULL);
	if(r < 0)
	{ goto exit; }

	while(!sync_flag)
	{
    wlmio_wait_for_event();
    wlmio_tick();
  }

	r = sync_return;

exit:
	return r;
}


int32_t wlmio_vpe6050_write_sync(const uint8_t node_id, const uint8_t ch, const uint16_t v)
{
	sync_flag = 0;

	int32_t r = wlmio_vpe6050_write(node_id, ch, v, sync_callback, NULL);
	if(r < 0)
	{ goto exit; }

	while(!sync_flag)
	{
    wlmio_wait_for_event();
    wlmio_tick();
  }

	r = sync_return;

exit:
	return r;
}


int32_t wlmio_vpe6050_configure_sync(const uint8_t node_id, const uint8_t ch, const uint8_t mode)
{
	sync_flag = 0;

	int32_t r = wlmio_vpe6050_configure(node_id, ch, mode, sync_callback, NULL);
	if(r < 0)
	{ goto exit; }

	while(!sync_flag)
	{
    wlmio_wait_for_event();
    wlmio_tick();
  }

	r = sync_return;

exit:
	return r;
}


int32_t wlmio_vpe6060_read_sync(const uint8_t node_id, const uint8_t ch, uint32_t* const v)
{
	sync_flag = 0;

	int32_t r = wlmio_vpe6060_read(node_id, ch, v, sync_callback, NULL);
	if(r < 0)
	{ goto exit; }

	while(!sync_flag)
	{
    wlmio_wait_for_event();
    wlmio_tick();
  }

	r = sync_return;

exit:
	return r;
}


int32_t wlmio_vpe6060_configure_sync(const uint8_t node_id, const uint8_t ch, const uint8_t mode, const uint8_t polarity, const uint8_t bias)
{
	sync_flag = 0;

	int32_t r = wlmio_vpe6060_configure(node_id, ch, mode, polarity, bias, sync_callback, NULL);
	if(r < 0)
	{ goto exit; }

	while(!sync_flag)
	{
    wlmio_wait_for_event();
    wlmio_tick();
  }

	r = sync_return;

exit:
	return r;
}


int32_t wlmio_vpe6070_write_sync(const uint8_t node_id, const uint8_t ch, const uint16_t v)
{
	sync_flag = 0;

	int32_t r = wlmio_vpe6070_write(node_id, ch, v, sync_callback, NULL);
	if(r < 0)
	{ goto exit; }

	while(!sync_flag)
	{
    wlmio_wait_for_event();
    wlmio_tick();
  }

	r = sync_return;

exit:
	return r;
}


int32_t wlmio_vpe6080_read_sync(const uint8_t node_id, const uint8_t ch, uint16_t* const v)
{
	sync_flag = 0;

	int32_t r = wlmio_vpe6080_read(node_id, ch, v, sync_callback, NULL);
	if(r < 0)
	{ goto exit; }

	while(!sync_flag)
	{
    wlmio_wait_for_event();
    wlmio_tick();
  }

	r = sync_return;

exit:
	return r;
}


int32_t wlmio_vpe6080_configure_sync(const uint8_t node_id, const uint8_t ch, const uint8_t enabled, const uint16_t beta, const uint16_t t0)
{
	sync_flag = 0;

	int32_t r = wlmio_vpe6080_configure(node_id, ch, enabled, beta, t0, sync_callback, NULL);
	if(r < 0)
	{ goto exit; }

	while(!sync_flag)
	{
    wlmio_wait_for_event();
    wlmio_tick();
  }

	r = sync_return;

exit:
	return r;
}


int32_t wlmio_node_set_sample_interval_sync(const uint8_t node_id, const uint16_t sample_interval)
{
	sync_flag = 0;

	int32_t r = wlmio_node_set_sample_interval(node_id, sample_interval, sync_callback, NULL);
	if(r < 0)
	{ goto exit; }

	while(!sync_flag)
	{
    wlmio_wait_for_event();
    wlmio_tick();
  }

	r = sync_return;

exit:
	return r;
}


int32_t wlmio_vpe6090_read_sync(const uint8_t node_id, const uint8_t ch, uint16_t* const v)
{
  sync_flag = 0;

  int32_t r = wlmio_vpe6090_read(node_id, ch, v, sync_callback, NULL);
  if(r < 0)
  { goto exit; }

  while(!sync_flag)
  {
    wlmio_wait_for_event();
    wlmio_tick();
  }

  r = sync_return;

exit:
  return r;
}


int32_t wlmio_vpe6090_configure_sync(const uint8_t node_id, const uint8_t ch, const uint8_t type)
{
  sync_flag = 0;

  int32_t r = wlmio_vpe6090_configure(node_id, ch, type, sync_callback, NULL);
  if(r < 0)
  { goto exit; }

  while(!sync_flag)
  {
    wlmio_wait_for_event();
    wlmio_tick();
  }

  r = sync_return;

exit:
  return r;
}


int32_t wlmio_vpe6180_read_sync(const uint8_t node_id, const uint8_t ch, uint16_t* const v)
{
  sync_flag = 0;

  int32_t r = wlmio_vpe6180_read(node_id, ch, v, sync_callback, NULL);
  if(r < 0)
  { goto exit; }

  while(!sync_flag)
  {
    wlmio_wait_for_event();
    wlmio_tick();
  }

  r = sync_return;

exit:
  return r;
}


int32_t wlmio_vpe6190_configure_sync(const uint8_t node_id, const uint8_t ch, uint8_t enable)
{
  sync_flag = 0;

  int32_t r = wlmio_vpe6190_configure(node_id, ch, enable, sync_callback, NULL);
  if(r < 0)
  { goto exit; }

  while(!sync_flag)
  {
    wlmio_wait_for_event();
    wlmio_tick();
  }

  r = sync_return;

exit:
  return r;
}


int32_t wlmio_vpe6190_read_sync(const uint8_t node_id, const uint8_t ch, uint32_t* const v)
{
  sync_flag = 0;

  int32_t r = wlmio_vpe6190_read(node_id, ch, v, sync_callback, NULL);
  if(r < 0)
  { goto exit; }

  while(!sync_flag)
  {
    wlmio_wait_for_event();
    wlmio_tick();
  }

  r = sync_return;

exit:
  return r;
}