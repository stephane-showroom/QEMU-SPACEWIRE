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
 * SpaceWire echo over the GRSPW core 0, using the RTEMS grspw_pkt driver.
 *
 * The application sends text packets to its own node address. device_simu
 * sends them back in upper case, which shows that they came through the link.
 */

#include <rtems.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <drvmgr/drvmgr.h>
#include <grlib/ambapp.h>
#include <grlib/ambapp_bus_grlib.h>
#include <grlib/grspw_pkt.h>

#ifndef SPW_CORE
#define SPW_CORE         0
#endif
#define SPW_NODE_ADDRESS 0x42
#define SPW_PROTOCOL_ID  0xF0   /* User-defined protocol identifier */
#define TIMEOUT_TICKS    500

/* Packet: [destination node][protocol id][text] */
#define SPW_HEADER_SIZE  2
#define SPW_MAX_PACKET   512

/* Everything is polled here: the driver's work task is not needed. */
int grspw_work_task_priority = -1;

static const char *const messages[] = {
  "hello spacewire",
  "rtems on leon3",
  "over grspw core 0",
};

static unsigned char tx_buffer[SPW_MAX_PACKET] __attribute__((aligned(4)));
static unsigned char rx_buffer[SPW_MAX_PACKET] __attribute__((aligned(4)));

static void fail(const char *message)
{
  printf("SPW ECHO FAILED: %s\n", message);
  exit(1);
}

/*
 * The leon3 BSP is built without RTEMS_DRVMGR_STARTUP, so its console and
 * clock do not depend on the driver manager and the application starts it.
 */
static void start_driver_manager(void)
{
  static struct drvmgr_bus_res resources = {
    .next = NULL,
    .resource = { DRVMGR_RES_EMPTY },
  };
  static struct grlib_config config;

  config.abus = ambapp_plb();
  config.resources = &resources;

  if (ambapp_grlib_root_register(&config) != 0) {
    fail("cannot register the AMBA root bus");
  }

  drvmgr_init();
}

static void *open_dma_channel(void)
{
  struct grspw_addr_config address;
  struct grspw_dma_config dma_config;
  int link_options = LINKOPTS_ENABLE | LINKOPTS_START;
  int status_config = -1;
  int clock_divisor = -1;
  void *device;
  void *dma;

  printf("GRSPW devices found: %d\n", grspw_dev_count());

  device = grspw_open(SPW_CORE);
  if (device == NULL) {
    fail("grspw_open");
  }

  memset(&address, 0, sizeof(address));
  address.def_addr = SPW_NODE_ADDRESS;
  grspw_addr_ctrl(device, &address);

  grspw_link_ctrl(device, &link_options, &status_config, &clock_divisor);
  if (grspw_link_state(device) != SPW_LS_RUN) {
    fail("the link is not in the run state");
  }

  dma = grspw_dma_open(device, 0);
  if (dma == NULL) {
    fail("grspw_dma_open");
  }

  memset(&dma_config, 0, sizeof(dma_config));
  dma_config.flags = DMAFLAG_NO_SPILL;
  dma_config.rxmaxlen = SPW_MAX_PACKET;
  if (grspw_dma_config(dma, &dma_config) != 0) {
    fail("grspw_dma_config");
  }

  if (grspw_dma_start(dma) != 0) {
    fail("grspw_dma_start");
  }

  return dma;
}

static void prepare_reception(void *dma, struct grspw_pkt *packet)
{
  struct grspw_list list = { packet, packet };

  memset(packet, 0, sizeof(*packet));
  packet->data = rx_buffer;

  if (grspw_dma_rx_prepare(dma, 0, &list, 1) != 0) {
    fail("grspw_dma_rx_prepare");
  }
}

static void send_packet(void *dma, struct grspw_pkt *packet, const char *text)
{
  struct grspw_list list = { packet, packet };
  size_t text_size = strlen(text);
  int ticks;

  tx_buffer[0] = SPW_NODE_ADDRESS;
  tx_buffer[1] = SPW_PROTOCOL_ID;
  memcpy(&tx_buffer[SPW_HEADER_SIZE], text, text_size);

  memset(packet, 0, sizeof(*packet));
  packet->data = tx_buffer;
  packet->dlen = SPW_HEADER_SIZE + text_size;

  if (grspw_dma_tx_send(dma, 0, &list, 1) != 0) {
    fail("grspw_dma_tx_send");
  }

  for (ticks = 0; ticks < TIMEOUT_TICKS; ++ticks) {
    int count = -1;

    list.head = NULL;
    list.tail = NULL;
    if (grspw_dma_tx_reclaim(dma, 0, &list, &count) != 0) {
      fail("grspw_dma_tx_reclaim");
    }

    if (count == 1) {
      if ((packet->flags & TXPKT_FLAG_TX) == 0) {
        fail("the packet was not transmitted");
      }
      return;
    }

    rtems_task_wake_after(1);
  }

  fail("timeout while sending");
}

static void receive_packet(void *dma, struct grspw_pkt *packet, const char *text)
{
  size_t text_size = strlen(text);
  size_t i;
  int ticks;

  for (ticks = 0; ticks < TIMEOUT_TICKS; ++ticks) {
    struct grspw_list list = { NULL, NULL };
    int count = -1;

    if (grspw_dma_rx_recv(dma, 0, &list, &count) != 0) {
      fail("grspw_dma_rx_recv");
    }

    if (count == 1) {
      break;
    }

    rtems_task_wake_after(1);
  }

  if (ticks == TIMEOUT_TICKS) {
    fail("timeout while receiving");
  }

  if (packet->dlen != SPW_HEADER_SIZE + text_size
      || rx_buffer[0] != SPW_NODE_ADDRESS
      || rx_buffer[1] != SPW_PROTOCOL_ID) {
    fail("unexpected packet header or size");
  }

  for (i = 0; i < text_size; ++i) {
    if (rx_buffer[SPW_HEADER_SIZE + i] != toupper((unsigned char) text[i])) {
      fail("unexpected packet content");
    }
  }

  printf("received: %.*s\n", (int) text_size, &rx_buffer[SPW_HEADER_SIZE]);
}

static rtems_task Init(rtems_task_argument arg)
{
  struct grspw_pkt tx_packet;
  struct grspw_pkt rx_packet;
  void *dma;
  size_t i;

  (void) arg;

  start_driver_manager();
  dma = open_dma_channel();

  for (i = 0; i < sizeof(messages) / sizeof(messages[0]); ++i) {
    prepare_reception(dma, &rx_packet);
    printf("sending:  %s\n", messages[i]);
    send_packet(dma, &tx_packet, messages[i]);
    receive_packet(dma, &rx_packet, messages[i]);
  }

  printf("SPW ECHO OK\n");
  exit(0);
}

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

#define CONFIGURE_MAXIMUM_TASKS 1
#define CONFIGURE_MAXIMUM_SEMAPHORES 16
#define CONFIGURE_UNIFIED_WORK_AREAS
#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
/* newlib's printf uses the FPU, which is forbidden in non-floating-point tasks. */
#define CONFIGURE_INIT_TASK_ATTRIBUTES RTEMS_FLOATING_POINT
#define CONFIGURE_USE_DEVFS_AS_BASE_FILESYSTEM

#define CONFIGURE_INIT
#include <rtems/confdefs.h>

#define CONFIGURE_DRIVER_AMBAPP_GAISLER_GRSPW2
#include <drvmgr/drvmgr_confdefs.h>
