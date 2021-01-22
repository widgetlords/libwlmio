#include <stddef.h>
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>

#include <wlmio.h>


const char* health_to_str(const uint8_t health)
{
  switch(health)
  {
    case WLMIO_HEALTH_NOMINAL:
      return "NOMINAL";

    case WLMIO_HEALTH_ADVISORY:
      return "ADVISORY";

    case WLMIO_HEALTH_CAUTION:
      return "CAUTION";

    case WLMIO_HEALTH_WARNING:
      return "WARNING";

    default:
      return "UNKNOWN";
  }
}


const char* mode_to_str(const uint8_t mode)
{
  switch(mode)
  {
    case WLMIO_MODE_OPERATIONAL:
      return "OPERATIONAL";

    case WLMIO_MODE_INITIALIZATION:
      return "INITIALIZATION";

    case WLMIO_MODE_MAINTENANCE:
      return "MAINTENANCE";

    case WLMIO_MODE_SOFTWARE_UPDATE:
      return "SOFTWARE UPDATE";

    case WLMIO_MODE_OFFLINE:
      return "OFFLINE";

    default:
      return "UNKNOWN";
  }
}


struct infop
{
  uint8_t node_id;
  struct wlmio_node_info info;
};
void info_callback(const int32_t r, void* uparam)
{
  struct infop* const t = uparam;

  if(r < 0)
  {
    printf("Failed to retrieve info for node %d\n", t->node_id);
    goto exit;
  }

  printf("Node %d is a %s\n", t->node_id, t->info.name);

exit:
  free(uparam);
}


void status_callback(const uint8_t node_id, const struct wlmio_status* const old_status, const struct wlmio_status* const new_status)
{
  if(new_status->mode == WLMIO_MODE_OFFLINE)
  {
    printf("Node %d has gone offline\n", node_id);
    return;
  }
  else if(old_status->mode == WLMIO_MODE_OFFLINE)
  {
    printf("Node %d has come online\n", node_id);
    struct infop* t = malloc(sizeof(struct infop));
    t->node_id = node_id;
    wlmio_get_node_info(node_id, &t->info, info_callback, t);
    return;
  }

  const uint32_t days = new_status->uptime / (24UL * 3600UL);
  const uint32_t hours = (new_status->uptime - days * 24UL * 3600UL) / 3600UL;
  const uint32_t minutes = (new_status->uptime - days * 24UL * 3600UL - hours * 3600UL) / 60UL;
  const uint32_t seconds = new_status->uptime - days * 24UL * 3600UL - hours * 3600UL - minutes * 60UL;
  printf(
    "Node %d, Uptime %03dd %02dh %02dm %02ds, Health %s, Mode %s, Status 0x%02X\n",
    node_id,
    days,
    hours,
    minutes,
    seconds,
    health_to_str(new_status->health),
    mode_to_str(new_status->mode),
    new_status->vendor_status
  );
}


int main(int argc, char** argv)
{
  // initialize libwlmio
	if(wlmio_init() < 0)
	{
		fprintf(stderr, "Failed to initialize libwlmio\n");
		return EXIT_FAILURE;
	}

  wlmio_set_status_callback(&status_callback);

  while(1)
  {
    wlmio_wait_for_event();
    wlmio_tick();
  }

  return 0;
}
