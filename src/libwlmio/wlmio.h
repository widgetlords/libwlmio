#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if 0
enum wlmio_transfer
{
	WLMIO_TRANSFER_MESSAGE = 0,
	WLMIO_TRANSFER_RESPONSE = 1,
	WLMIO_TRANSFER_REQUEST = 2
};

enum wlmio_priority
{
	WLMIO_PRIORITY_EXCEPTIONAL = 0,
	WLMIO_PRIORITY_IMMEDIATE = 1,
	WLMIO_PRIORITY_FAST = 2,
	WLMIO_PRIORITY_HIGH = 3,
	WLMIO_PRIORITY_NOMINAL = 4,
	WLMIO_PRIORITY_LOW = 5,
	WLMIO_PRIORITY_SLOW = 6,
	WLMIO_PRIORITY_OPTIONAL = 7
};

struct wlmio_transfer
{
	uint64_t timestamp_usec;
	uint8_t priority;
	uint8_t transfer_type;
	uint16_t port_id;
	uint8_t remote_node_id;
	size_t payload_size;
	void* payload;
};
#endif

enum wlmio_health
{
  WLMIO_HEALTH_NOMINAL = 0,
  WLMIO_HEALTH_ADVISORY = 1,
  WLMIO_HEALTH_CAUTION = 2,
  WLMIO_HEALTH_WARNING = 3
};

enum wlmio_mode
{
  WLMIO_MODE_OPERATIONAL = 0,
  WLMIO_MODE_INITIALIZATION = 1,
  WLMIO_MODE_MAINTENANCE = 2,
  WLMIO_MODE_SOFTWARE_UPDATE = 3,
  WLMIO_MODE_OFFLINE = 7
};

struct wlmio_status
{
  uint32_t uptime;
  uint8_t health;
  uint8_t mode;
  uint8_t vendor_status;
};

enum wlmio_register_value
{
	WLMIO_REGISTER_VALUE_EMPTY = 0U,
	WLMIO_REGISTER_VALUE_STRING = 1U,
	WLMIO_REGISTER_VALUE_UNSTRUCTURED = 2U,
	WLMIO_REGISTER_VALUE_BIT = 3U,
	WLMIO_REGISTER_VALUE_INT64 = 4U,
	WLMIO_REGISTER_VALUE_INT32 = 5U,
	WLMIO_REGISTER_VALUE_INT16 = 6U,
	WLMIO_REGISTER_VALUE_INT8 = 7U,
	WLMIO_REGISTER_VALUE_UINT64 = 8U,
	WLMIO_REGISTER_VALUE_UINT32 = 9U,
	WLMIO_REGISTER_VALUE_UINT16 = 10U,
	WLMIO_REGISTER_VALUE_UINT8 = 11U,
	WLMIO_REGISTER_VALUE_FLOAT64 = 12U,
	WLMIO_REGISTER_VALUE_FLOAT32 = 13U,
	WLMIO_REGISTER_VALUE_FLOAT16 = 14U
};

struct wlmio_register_access
{
	uint8_t type;
	uint16_t length;
	uint8_t value[256];
};

enum wlmio_command
{
  WLMIO_COMMAND_STORE_PERSISTENT_STATES = 65530,
  WLMIO_COMMAND_EMERGENCY_STOP = 65531,
  WLMIO_COMMAND_FACTORY_RESET = 65532,
  WLMIO_COMMAND_BEGIN_SOFTWARE_UPDATE = 65533,
  WLMIO_COMMAND_POWER_OFF = 65534,
  WLMIO_COMMAND_RESTART = 65535
};

enum wlmio_command_status
{
  WLMIO_COMMAND_STATUS_SUCCESS = 0,
  WLMIO_COMMAND_STATUS_FAILURE = 1,
  WLMIO_COMMAND_STATUS_NOT_AUTHORIZED = 2,
  WLMIO_COMMAND_STATUS_BAD_COMMAND = 3,
  WLMIO_COMMAND_STATUS_BAD_PARAMETER = 4,
  WLMIO_COMMAND_STATUS_BAD_STATE = 5,
  WLMIO_COMMAND_STATUS_INTERNAL_ERROR = 6
};

struct wlmio_node_version
{
	uint8_t major;
	uint8_t minor;
};

enum wlmio_node_info_flags
{
	WLMIO_NODE_INFO_CRC = 1,
	WLMIO_NODE_INFO_CERTIFICATE_OF_AUTHENTICITY = 2
};

struct wlmio_node_info
{
	struct wlmio_node_version protocol_version;
	struct wlmio_node_version hardware_version;
	struct wlmio_node_version software_version;
	uint64_t software_vcs_revision_id;
	uint8_t unique_id[16];
	char name[51];
	uint64_t software_image_crc;
	uint8_t certificate_of_authenticity[222];
	uint32_t flags;
};


/**
 * Initializes the library
 * 
 * @return Returns 0 if success else -1
*/
int32_t wlmio_init(void);


/**
 * Executes all currently pending events and returns
 * 
 * @return Returns 0 if success else -1
*/
int32_t wlmio_tick(void);


int wlmio_shutdown(void);
void wlmio_wait_for_event(void);
void wlmio_set_timeout(uint64_t us);
int64_t wlmio_get_epoll_fd(void);

void wlmio_set_status_callback(void (* callback)(uint8_t node_id, const struct wlmio_status* old_status, const struct wlmio_status* new_status));

uint8_t wlmio_get_node_id(void);

/**
 * List the registers present on a node one at a time.
 * 
 * @param name The name pointer must point to an array of at least 51 bytes
*/
int32_t wlmio_register_list(uint8_t node_id, uint16_t index, char* name, void (* callback)(int32_t r, void* uparam), void* uparam);
int32_t wlmio_register_access(uint8_t node_id, const char* name, const struct wlmio_register_access* regw, struct wlmio_register_access* regr, void (* callback)(int32_t r, void* uparam), void* uparam);

int32_t wlmio_execute_command(uint8_t node_id, uint16_t command, const void* param, size_t param_len, void (* callback)(int32_t r, void* uparam), void* uparam);

int32_t wlmio_get_node_info(uint8_t node_id, struct wlmio_node_info* node_info, void (* callback)(int32_t r, void* uparam), void* uparam);


// module specific functions

int32_t wlmio_node_set_sample_interval(uint8_t node_id, uint16_t sample_interval, void (* callback)(int32_t r, void* uparam), void* uparam);

struct wlmio_vpe6010_input
{
	uint16_t ma_5v;
	uint16_t mv_5v;
	uint16_t mv_24v1;
	uint16_t mv_24v2;
	uint16_t mv_24v;
	uint16_t ma_24v;
};
int32_t wlmio_vpe6010_read(uint8_t node_id, struct wlmio_vpe6010_input* dst, void (* callback)(int32_t r, void* uparam), void* uparam);

int32_t wlmio_vpe6030_write(uint8_t node_id, uint8_t ch, uint8_t v, void (* callback)(int32_t r, void* uparam), void* uparam);

enum wlmio_vpe6040
{
	WLMIO_VPE6040_MODE_5V = 0,
	WLMIO_VPE6040_MODE_MA = 1,
	WLMIO_VPE6040_MODE_10V = 2
};
int32_t wlmio_vpe6040_read(uint8_t node_id, uint8_t ch, uint16_t* v, void (* callback)(int32_t r, void* uparam), void* uparam);
int32_t wlmio_vpe6040_configure(uint8_t node_id, uint8_t ch, uint8_t mode, void (* callback)(int32_t r, void* uparam), void* uparam);

enum wlmio_vpe6050
{
	WLMIO_VPE6050_MODE_SOURCE = 0,
	WLMIO_VPE6050_MODE_SINK = 1
};
int32_t wlmio_vpe6050_write(uint8_t node_id, uint8_t ch, uint16_t v, void (* callback)(int32_t r, void* uparam), void* uparam);
int32_t wlmio_vpe6050_configure(uint8_t node_id, uint8_t ch, uint8_t mode, void (* callback)(int32_t r, void* uparam), void* uparam);

enum wlmio_vpe6060
{
	WLMIO_VPE6060_MODE_BASIC = 0,
	WLMIO_VPE6060_MODE_FREQUENCY = 1,
	WLMIO_VPE6060_MODE_PULSE_COUNTER = 2,
	WLMIO_VPE6060_BIAS_NONE = 0,
	WLMIO_VPE6060_BIAS_PNP = 1,
	WLMIO_VPE6060_BIAS_NPN = 2,
	WLMIO_VPE6060_POLARITY_RISING = 0,
	WLMIO_VPE6060_POLARITY_FALLING = 1
};
int32_t wlmio_vpe6060_read(uint8_t node_id, uint8_t ch, uint32_t* v, void (* callback)(int32_t r, void* uparam), void* uparam);
int32_t wlmio_vpe6060_configure(uint8_t node_id, uint8_t ch, uint8_t mode, uint8_t polarity, uint8_t bias, void (* callback)(int32_t r, void* uparam), void* uparam);

int32_t wlmio_vpe6070_write(uint8_t node_id, uint8_t ch, uint16_t v, void (* callback)(int32_t r, void* uparam), void* uparam);

int32_t wlmio_vpe6080_read(uint8_t node_id, uint8_t ch, uint16_t* v, void (* callback)(int32_t r, void* uparam), void* uparam);
int32_t wlmio_vpe6080_configure(uint8_t node_id, uint8_t ch, uint8_t enabled, uint16_t beta, uint16_t t0, void (* callback)(int32_t r, void* uparam), void* uparam);


// synchronous/blocking wrapper functions

/**
 * List the registers present on a node one at a time.
 * 
 * Synchronous equivalent of wlmio_register_list()
 * 
 * @param name The name pointer must point to an array of at least 51 bytes
*/
int32_t wlmio_register_list_sync(uint8_t node_id, uint16_t index, char* name);
int32_t wlmio_register_access_sync(uint8_t node_id, const char* name, const struct wlmio_register_access* regw, struct wlmio_register_access* regr);
int32_t wlmio_execute_command_sync(uint8_t node_id, uint16_t command, const void* param, size_t param_len);
int32_t wlmio_get_node_info_sync(uint8_t node_id, struct wlmio_node_info* node_info);

int32_t wlmio_node_set_sample_interval_sync(uint8_t node_id, uint16_t sample_interval);
int32_t wlmio_vpe6010_read_sync(uint8_t node_id, struct wlmio_vpe6010_input* dst);
int32_t wlmio_vpe6030_write_sync(uint8_t node_id, uint8_t ch, uint8_t v);
int32_t wlmio_vpe6040_read_sync(uint8_t node_id, uint8_t ch, uint16_t* v);
int32_t wlmio_vpe6040_configure_sync(uint8_t node_id, uint8_t ch, uint8_t mode);
int32_t wlmio_vpe6050_write_sync(uint8_t node_id, uint8_t ch, uint16_t v);
int32_t wlmio_vpe6050_configure_sync(uint8_t node_id, uint8_t ch, uint8_t mode);
int32_t wlmio_vpe6060_read_sync(uint8_t node_id, uint8_t ch, uint32_t* v);
int32_t wlmio_vpe6060_configure_sync(uint8_t node_id, uint8_t ch, uint8_t mode, uint8_t polarity, uint8_t bias);
int32_t wlmio_vpe6070_write_sync(uint8_t node_id, uint8_t ch, uint16_t v);
int32_t wlmio_vpe6080_read_sync(uint8_t node_id, uint8_t ch, uint16_t* v);
int32_t wlmio_vpe6080_configure_sync(uint8_t node_id, uint8_t ch, uint8_t enabled, uint16_t beta, uint16_t t0);

#ifdef __cplusplus
}
#endif
