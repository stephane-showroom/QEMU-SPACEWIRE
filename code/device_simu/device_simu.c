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
 * SpaceWire device simulator: sends back everything the guest sends through
 * the QEMU GRSPW device.
 *
 * Data packets are acknowledged, because their transmission only ends in the
 * guest with this acknowledge, and sent back in upper case on the same core.
 * The address and the protocol identifier are kept. Time codes are sent back
 * unchanged.
 *
 * A packet comes back with its original destination byte, so the guest must
 * address its own node, or enable open packet mode.
 */

#include <ctype.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "grlib_mqgrspw.h"

#define QUEUE_MSG_COUNT   10    /* Same attributes as grlib_mqgrspw_init() */
#define PACKET_PREFIX     2     /* Address and protocol identifier */

static void die(const char *message)
{
  perror(message);
  exit(EXIT_FAILURE);
}

static void send_message(mqd_t queue, const uint8_t *message, size_t size)
{
  if (mq_send(queue, (const char *) message, size, 0) != 0) {
    die("mq_send");
  }
}

static void send_ack(mqd_t queue, uint8_t core)
{
  uint8_t ack[C_HEADER_IDX + 1 + C_DATA_SIZE] = { 0 };

  ack[C_SPW_ID_IDX] = core;
  ack[C_PWORD_IDX] = C_MSGQ_ACK;

  send_message(queue, ack, sizeof(ack));
}

/* QEMU trusts the sizes of the messages it receives. */
static int is_well_formed(const uint8_t *message, size_t size)
{
  size_t data_offset;
  uint32_t data_size;

  if (size < C_HEADER_IDX + 1 + C_DATA_SIZE) {
    return 0;
  }

  data_offset = C_HEADER_IDX + 1 + message[C_HEADER_IDX];
  if (size < data_offset + C_DATA_SIZE) {
    return 0;
  }

  memcpy(&data_size, &message[data_offset], sizeof(data_size));

  return size - data_offset - C_DATA_SIZE == data_size;
}

/* A packet is the concatenation of the header and the data. */
static void packet_to_upper(uint8_t *message)
{
  uint32_t header_size = message[C_HEADER_IDX];
  uint8_t *header = &message[C_HEADER_IDX + 1];
  uint8_t *data = &header[header_size + C_DATA_SIZE];
  uint32_t data_size;
  uint32_t i;

  memcpy(&data_size, &header[header_size], sizeof(data_size));

  for (i = PACKET_PREFIX; i < header_size; ++i) {
    header[i] = (uint8_t) toupper(header[i]);
  }

  i = header_size < PACKET_PREFIX ? PACKET_PREFIX - header_size : 0;
  for (; i < data_size; ++i) {
    data[i] = (uint8_t) toupper(data[i]);
  }
}

int main(void)
{
  struct mq_attr attr = {
    .mq_maxmsg = QUEUE_MSG_COUNT,
    .mq_msgsize = C_MAXQ_SIZE,
  };
  mqd_t from_qemu;
  mqd_t to_qemu;

  from_qemu = mq_open(C_SPWQ, O_CREAT | O_RDONLY, 0644, &attr);
  if (from_qemu == (mqd_t) -1) {
    die("mq_open " C_SPWQ);
  }

  to_qemu = mq_open(C_DEVQ, O_CREAT | O_WRONLY, 0644, &attr);
  if (to_qemu == (mqd_t) -1) {
    die("mq_open " C_DEVQ);
  }

  for (;;) {
    uint8_t message[C_MAXQ_SIZE];
    ssize_t size;

    size = mq_receive(from_qemu, (char *) message, sizeof(message), NULL);
    if (size < 0) {
      die("mq_receive");
    }

    if (size < C_HEADER_IDX + 1) {
      fprintf(stderr, "device_simu: dropped a truncated message\n");
      continue;
    }

    /* RMAP replies are for an initiator: there is nothing to send back. */
    if (message[C_PWORD_IDX] == C_MSGQ_RMAP) {
      continue;
    }

    if (message[C_PWORD_IDX] == C_MSGQ_DATA) {
      send_ack(to_qemu, message[C_SPW_ID_IDX]);
    }

    if (is_well_formed(message, (size_t) size)) {
      if (message[C_PWORD_IDX] == C_MSGQ_DATA) {
        packet_to_upper(message);
      }
      send_message(to_qemu, message, (size_t) size);
    } else {
      fprintf(stderr, "device_simu: dropped a malformed message\n");
    }
  }
}
