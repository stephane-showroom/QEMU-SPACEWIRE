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
 * The RTEMS grspw_pkt driver used with interrupts and its work task, the way an
 * application normally does: a list of packets is sent at once, and the
 * application waits for the reception. Runs against device_simu, on the core
 * selected at build time with -DCORE=<n>.
 */

#include <rtems.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <drvmgr/drvmgr.h>
#include <grlib/ambapp.h>
#include <grlib/ambapp_bus_grlib.h>
#include <grlib/grspw_pkt.h>

#define NODE_ADDRESS     0x42
#define PROTOCOL_ID      0xF0
#define PACKET_COUNT     8
#define PACKET_SIZE      32
#define WAIT_TICKS       500

static struct grspw_pkt tx_packets[PACKET_COUNT];
static struct grspw_pkt rx_packets[PACKET_COUNT];
static unsigned char tx_buffers[PACKET_COUNT][PACKET_SIZE] __attribute__((aligned(4)));
static unsigned char rx_buffers[PACKET_COUNT][PACKET_SIZE] __attribute__((aligned(4)));

static void fail(const char *message)
{
  printf("TEST FAILED: %s\n", message);
  exit(1);
}

static void make_list(struct grspw_list *list, struct grspw_pkt *packets)
{
  int i;

  for (i = 0; i < PACKET_COUNT - 1; ++i) {
    packets[i].next = &packets[i + 1];
  }
  packets[PACKET_COUNT - 1].next = NULL;
  list->head = &packets[0];
  list->tail = &packets[PACKET_COUNT - 1];
}

static void *open_channel(void)
{
  struct grspw_addr_config address;
  struct grspw_dma_config config;
  int link_options = LINKOPTS_ENABLE | LINKOPTS_START;
  int status_config = -1;
  int clock_divisor = -1;
  void *device = grspw_open(CORE);
  void *dma;

  if (device == NULL) {
    fail("grspw_open");
  }

  memset(&address, 0, sizeof(address));
  address.def_addr = NODE_ADDRESS;
  grspw_addr_ctrl(device, &address);
  grspw_link_ctrl(device, &link_options, &status_config, &clock_divisor);

  dma = grspw_dma_open(device, 0);
  if (dma == NULL) {
    fail("grspw_dma_open");
  }

  memset(&config, 0, sizeof(config));
  config.flags = DMAFLAG_NO_SPILL;
  config.rxmaxlen = PACKET_SIZE;
  config.rx_irq_en_cnt = 1;
  config.tx_irq_en_cnt = 1;
  if (grspw_dma_config(dma, &config) != 0 || grspw_dma_start(dma) != 0) {
    fail("grspw_dma_config or grspw_dma_start");
  }

  return dma;
}

static rtems_task Init(rtems_task_argument arg)
{
  static struct drvmgr_bus_res resources = {
    .next = NULL,
    .resource = { DRVMGR_RES_EMPTY },
  };
  static struct grlib_config config;
  struct grspw_dma_stats stats;
  struct grspw_list list;
  void *dma;
  int count = -1;
  int i;

  (void) arg;

  config.abus = ambapp_plb();
  config.resources = &resources;
  ambapp_grlib_root_register(&config);
  drvmgr_init();

  dma = open_channel();

  for (i = 0; i < PACKET_COUNT; ++i) {
    rx_packets[i].data = rx_buffers[i];

    tx_buffers[i][0] = NODE_ADDRESS;
    tx_buffers[i][1] = PROTOCOL_ID;
    memset(&tx_buffers[i][2], 'A' + i, PACKET_SIZE - 2);
    tx_packets[i].data = tx_buffers[i];
    tx_packets[i].dlen = PACKET_SIZE;
  }

  make_list(&list, rx_packets);
  if (grspw_dma_rx_prepare(dma, 0, &list, PACKET_COUNT) != 0) {
    fail("grspw_dma_rx_prepare");
  }

  make_list(&list, tx_packets);
  if (grspw_dma_tx_send(dma, 0, &list, PACKET_COUNT) != 0) {
    fail("grspw_dma_tx_send");
  }

  /* Only the interrupts can wake this task up. */
  if (grspw_dma_rx_wait(dma, PACKET_COUNT, GRSPW_OP_AND, 0, WAIT_TICKS) != 0) {
    fail("grspw_dma_rx_wait");
  }

  list.head = NULL;
  list.tail = NULL;
  if (grspw_dma_rx_recv(dma, 0, &list, &count) != 0 || count != PACKET_COUNT) {
    fail("grspw_dma_rx_recv");
  }

  for (i = 0; i < PACKET_COUNT; ++i) {
    if (rx_packets[i].dlen != PACKET_SIZE
        || memcmp(rx_buffers[i], tx_buffers[i], PACKET_SIZE) != 0) {
      fail("unexpected packet");
    }
  }

  grspw_dma_stats_read(dma, &stats);
  if (stats.irq_cnt == 0) {
    fail("no interrupt counted");
  }

  printf("TEST OK\n");
  exit(0);
}

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

#define CONFIGURE_MAXIMUM_TASKS 4
#define CONFIGURE_MAXIMUM_SEMAPHORES 32
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES 4
#define CONFIGURE_UNIFIED_WORK_AREAS
#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
/* newlib's printf uses the FPU, which is forbidden in non-floating-point tasks. */
#define CONFIGURE_INIT_TASK_ATTRIBUTES RTEMS_FLOATING_POINT

#define CONFIGURE_INIT
#include <rtems/confdefs.h>

#define CONFIGURE_DRIVER_AMBAPP_GAISLER_GRSPW2
#include <drvmgr/drvmgr_confdefs.h>
