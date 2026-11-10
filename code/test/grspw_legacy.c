/* SPDX-License-Identifier: GPL-2.0-or-later */
/********************************************************************************
 *                                                                              *
 *    ▀▄   ▄▀                                                                   *
 *   ▄█▀███▀█▄    Created on: Nov 6, 2026                                       *
 *  █▀███████▀█   Author: Stephane GONZALEZ                                     *
 *  █ █▀▀▀▀▀█ █                                                                 *
 *     ▀▀ ▀▀                                                                    *
 *                                                                              *
 * Copyright (C) 2015-2026 Stephane GONZALEZ                                    *
 * Released under the GNU GPL v2 or later, see LICENSE                          *
 ********************************************************************************/

/*
 * GRSPW scenarios for the RTEMS legacy driver (grspw.c), which relies on
 * interrupts. Each scenario is selected at build time with -DSCENARIO_<name>,
 * the core with -DCORE=<n>, and prints TEST OK on success. They run against
 * device_simu.
 */

#include <rtems.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <drvmgr/drvmgr.h>
#include <grlib/ambapp.h>
#include <grlib/ambapp_bus_grlib.h>
#include <grlib/grspw.h>

#define NODE_ADDRESS       0x42
#define PROTOCOL_ID        0xF0
#define READ_TIMEOUT       200    /* Ticks */
#define PACKET_SIZE        32
#define EXCHANGE_COUNT     10
#define MANY_PACKETS       300    /* More than the 64 TX descriptors */
#define MAX_PACKET_SIZE    1024   /* Default limit of the driver */

static int device;

static void fail(const char *message)
{
  printf("TEST FAILED: %s (errno %d)\n", message, errno);
  exit(1);
}

static void start_link(void)
{
  if (ioctl(device, SPACEWIRE_IOCTRL_START, -1) != 0) {
    fail("SPACEWIRE_IOCTRL_START");
  }
}

/* The read blocks on a semaphore that only the interrupt handler releases. */
__attribute__((unused)) static void exchange(int sequence, int size)
{
  static char tx[MAX_PACKET_SIZE];
  static char rx[MAX_PACKET_SIZE];

  /* No lower-case letter: device_simu sends this content back unchanged. */
  memset(tx, 'A' + sequence % 26, size);
  tx[0] = NODE_ADDRESS;
  tx[1] = PROTOCOL_ID;
  tx[2] = (char) (sequence & 0x3f);

  if (write(device, tx, size) != size) {
    fail("write");
  }

  if (read(device, rx, sizeof(rx)) != size) {
    fail("read");
  }

  if (memcmp(tx, rx, size) != 0) {
    fail("unexpected packet content");
  }
}

static void run_scenario(void)
{
#if defined(SCENARIO_irq_rx_blocking)
  spw_stats stats;
  int i;

  for (i = 0; i < EXCHANGE_COUNT; ++i) {
    exchange(i, PACKET_SIZE);
  }

  if (ioctl(device, SPACEWIRE_IOCTRL_GET_STATISTICS, &stats) != 0) {
    fail("SPACEWIRE_IOCTRL_GET_STATISTICS");
  }

  /* packets_sent is only incremented by the interrupt handler. */
  if (stats.packets_sent != EXCHANGE_COUNT) {
    fail("the transmit interrupts were not all taken");
  }
#elif defined(SCENARIO_many_packets)
  int i;

  for (i = 0; i < MANY_PACKETS; ++i) {
    exchange(i, PACKET_SIZE);
  }
#elif defined(SCENARIO_stop_start)
  exchange(0, PACKET_SIZE);

  if (ioctl(device, SPACEWIRE_IOCTRL_STOP, 0) != 0) {
    fail("SPACEWIRE_IOCTRL_STOP");
  }

  start_link();
  exchange(1, PACKET_SIZE);
#elif defined(SCENARIO_clear_ctrl_bit)
  /* The ioctl reads the bit back: it fails if a control bit cannot be cleared. */
  if (ioctl(device, SPACEWIRE_IOCTRL_SET_LINK_ERR_IRQ, 1) != 0
      || ioctl(device, SPACEWIRE_IOCTRL_SET_LINK_ERR_IRQ, 0) != 0) {
    fail("SPACEWIRE_IOCTRL_SET_LINK_ERR_IRQ");
  }
#elif defined(SCENARIO_max_packet)
  exchange(0, MAX_PACKET_SIZE);
#elif defined(SCENARIO_data_before_ack)
  exchange(0, PACKET_SIZE);
#else
#error "Unknown scenario"
#endif
}

static rtems_task Init(rtems_task_argument arg)
{
  static struct drvmgr_bus_res resources = {
    .next = NULL,
    .resource = { DRVMGR_RES_EMPTY },
  };
  static struct grlib_config config;
  char path[16];

  (void) arg;

  /* The leon3 BSP is built without RTEMS_DRVMGR_STARTUP. */
  config.abus = ambapp_plb();
  config.resources = &resources;
  if (ambapp_grlib_root_register(&config) != 0) {
    fail("ambapp_grlib_root_register");
  }
  drvmgr_init();

  snprintf(path, sizeof(path), "/dev/grspw%d", CORE);
  device = open(path, O_RDWR);
  if (device < 0) {
    fail("open");
  }

  ioctl(device, SPACEWIRE_IOCTRL_SET_NODEADDR, NODE_ADDRESS);
  ioctl(device, SPACEWIRE_IOCTRL_SET_RXBLOCK, 1);
  ioctl(device, SPACEWIRE_IOCTRL_SET_TXBLOCK_ON_FULL, 1);
  ioctl(device, SPACEWIRE_IOCTRL_SET_KEEP_SOURCE, 1);
  ioctl(device, SPACEWIRE_IOCTRL_SET_READ_TIMEOUT, READ_TIMEOUT);
  start_link();

  run_scenario();

  printf("TEST OK\n");
  exit(0);
}

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

#define CONFIGURE_MAXIMUM_TASKS 1
#define CONFIGURE_MAXIMUM_SEMAPHORES 32
#define CONFIGURE_MAXIMUM_DRIVERS 8
#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS 16
#define CONFIGURE_UNIFIED_WORK_AREAS
#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
/* newlib's printf uses the FPU, which is forbidden in non-floating-point tasks. */
#define CONFIGURE_INIT_TASK_ATTRIBUTES RTEMS_FLOATING_POINT

#define CONFIGURE_INIT
#include <rtems/confdefs.h>

#define CONFIGURE_DRIVER_AMBAPP_GAISLER_GRSPW
#include <drvmgr/drvmgr_confdefs.h>
