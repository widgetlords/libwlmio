#define _XOPEN_SOURCE 600

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <poll.h>
#include <pty.h>
#include <termios.h>
#include <unistd.h>

#include <argp.h>
#include <gpiod.h>

struct gpiod_chip* chip;
struct gpiod_line* line;


void driver_init(unsigned int driver)
{
  chip = gpiod_chip_open_by_name("gpiochip0");
  if(chip == NULL)
  {
    fprintf(stderr, "Unable to open GPIO chip\n");
    exit(EXIT_FAILURE);
  }

  line = gpiod_chip_get_line(chip, driver);
  if(line == NULL)
  {
    fprintf(stderr, "Unable to get GPIO line\n");
    exit(EXIT_FAILURE);
  }

  int r = gpiod_line_request_output(line, "modbusd", 0);
  if(r < 0)
  {
    fprintf(stderr, "Unable to reserve GPIO line\n");
    exit(EXIT_FAILURE);
  }
}


void driver_cleanup(void)
{
  assert(chip);

  gpiod_chip_close(chip);
}


void driver_enable(const uint8_t e)
{
  assert(line);

  int r = gpiod_line_set_value(line, e != 0);
  if(r < 0)
  {
    fprintf(stderr, "Unable to set GPIO value\n");
    exit(EXIT_FAILURE);
  }
}


uint32_t get_baud(const speed_t baud)
{
  switch(baud)
  {
    case B50:
      return 50UL;

    case B75:
      return 75UL;

    case B110:
      return 110UL;

    case B134:
      return 134UL;

    case B150:
      return 150UL;

    case B200:
      return 200UL;

    case B300:
      return 300UL;

    case B600:
      return 600UL;

    case B1200:
      return 1200UL;

    case B1800:
      return 1800UL;

    case B2400:
      return 2400UL;

    case B4800:
      return 4800UL;

    case B9600:
      return 9600UL;

    case B19200:
    default:
      return 19200UL;

    case B38400:
      return 38400UL;

    case B57600:
      return 57600UL;

    case B115200:
      return 115200UL;

    case B230400:
      return 230400UL;

    case B460800:
      return 460800UL;

    case B500000:
      return 500000UL;

    case B576000:
      return 576000UL;

    case B921600:
      return 921600UL;

    case B1000000:
      return 1000000UL;

    case B1152000:
      return 1152000UL;

    case B1500000:
      return 1500000UL;

    case B2000000:
      return 2000000UL;

    case B2500000:
      return 2500000UL;

    case B3000000:
      return 3000000UL;

    case B3500000:
      return 3500000UL;

    case B4000000:
      return 4000000UL;
    }
}


uint32_t get_size(const struct termios* const cfg)
{
  const uint32_t csize = cfg->c_cflag & CSIZE;
  switch(csize)
  {
    case CS5:
      return 5UL;

    case CS6:
      return 6UL;

    case CS7:
      return 7UL;

    case CS8:
    default:
      return 8UL;
  }
}


uint32_t get_byte_time(const struct termios* const cfg)
{
  const uint32_t baud = get_baud(cfgetispeed(cfg));
  const uint32_t stop_bits = cfg->c_cflag & CSTOPB ? 2 : 1;
  const uint32_t data_bits = get_size(cfg);
  const uint32_t parity = cfg->c_cflag & PARENB ? 1 : 0;
  return 1000000UL * (1 + data_bits + parity + stop_bits) / baud;
}


/* The options we understand. */
static struct argp_option options[] = {
  {"stty", 's', "FILE", 0, "Specify serial TTY" },
  {"vtty", 'v', "FILE", 0, "Specify virtual TTY" },
  {"driver", 'd', "GPIO", 0, "GPIO pin used for transmit enable" },
  { 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
  const char* stty;
  const char* vtty;
  unsigned int driver;
};

/* Parse a single option. */
static error_t parse_opt(int key, char* arg, struct argp_state* state)
{
  struct arguments* arguments = state->input;

  char* endptr;

  switch(key)
  {
    case 's':
      arguments->stty = arg;
      break;

    case 'v':
      arguments->vtty = arg;
      break;

    case 'd':
      errno = 0;
      unsigned long value = strtoul(arg, &endptr, 0);
      if(*endptr != '\0' || endptr == arg)
      { return EINVAL; }

      arguments->driver = value;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

static struct argp argp = { options, parse_opt, NULL, NULL };

struct arguments arguments =
{
  .stty = "/dev/ttyAMA1",
  .vtty = "/tmp/modbus",
  .driver = 18U
};

int uart = -1;


void cleanup(void)
{
  driver_cleanup();

  close(uart);

  unlink(arguments.vtty);
}


int main(const int argc, char** const argv)
{
  int r = argp_parse(&argp, argc, argv, 0, 0, &arguments);
  if(r != 0)
  {
    fprintf(stderr, "%s\n", strerror(r));
    return 1;
  }

  atexit(&cleanup);

  driver_init(arguments.driver);

  uart = open(arguments.stty, O_RDWR | O_NOCTTY);
  if(uart < 0)
  {
    fprintf(stderr, "Could not open %s: %s\n", arguments.stty, strerror(errno));
    return 1;
  }

  if(!isatty(uart))
  {
    fprintf(stderr, "%s is not a tty\n", arguments.stty);
    return 1;
  }

  int master, slave;
  r = openpty(&master, &slave, NULL, NULL, NULL);
  // close(slave);
  unlink(arguments.vtty);
  r = symlink(ptsname(master), arguments.vtty);
  if(r < 0)
  {
    fprintf(stderr, "Could not symlink %s: %s\n", arguments.vtty, strerror(errno));
    return 1;
  }

 	struct pollfd fds[2];
	fds[0].fd = master;
	fds[0].events = POLLIN;
	fds[1].fd = uart;
	fds[1].events = POLLIN;
	
	while(1)
	{
		poll(fds, 2, -1);

    if(fds[0].revents & POLLIN)
    {
      struct termios cfg;
      tcgetattr(master, &cfg);
      tcsetattr(uart, TCSANOW, &cfg);

      uint8_t buffer[256];
      int len = read(master, buffer, 256);
      if(len > 0)
      {
        driver_enable(1);
        write(uart, buffer, len);

        // tcdrain() is horribly slow, taking ~20ms longer than expected which means the response gets clobbered.
        // So we calculate the expected transmission time and wait.
        usleep(get_byte_time(&cfg) * len); 

        driver_enable(0);
      }
    }

    if(fds[1].revents & POLLIN)
    {
      uint8_t buffer[256];
      int len = read(uart, buffer, 256);
      if(len > 0)
      { write(master, buffer, len); }
    }
  }

  return 0;
}