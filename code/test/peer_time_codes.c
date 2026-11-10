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
 * Peer that acknowledges the first packet and answers with the time codes 62,
 * 63 and 0, to check the wrap of the time counter.
 */

#include <fcntl.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "grlib_mqgrspw.h"

#define QUEUE_MSG_COUNT   10
#define TICK_PERIOD_NS    20000000L   /* The IRQMP latches one pending interrupt */

int main(void)
{
  static const uint8_t time_codes[] = { 62, 63, 0 };
  static const struct timespec tick_period = { .tv_nsec = TICK_PERIOD_NS };
  struct mq_attr attr = {
    .mq_maxmsg = QUEUE_MSG_COUNT,
    .mq_msgsize = C_MAXQ_SIZE,
  };
  mqd_t from_qemu = mq_open(C_SPWQ, O_CREAT | O_RDONLY, 0644, &attr);
  mqd_t to_qemu = mq_open(C_DEVQ, O_CREAT | O_WRONLY, 0644, &attr);
  uint8_t message[C_MAXQ_SIZE];
  uint8_t ack[C_HEADER_IDX + 1 + C_DATA_SIZE] = { 0 };
  size_t i;

  if (from_qemu == (mqd_t) -1 || to_qemu == (mqd_t) -1) {
    perror("mq_open");
    return EXIT_FAILURE;
  }

  if (mq_receive(from_qemu, (char *) message, sizeof(message), NULL) < 0) {
    perror("mq_receive");
    return EXIT_FAILURE;
  }

  ack[C_SPW_ID_IDX] = message[C_SPW_ID_IDX];
  ack[C_PWORD_IDX] = C_MSGQ_ACK;
  mq_send(to_qemu, (const char *) ack, sizeof(ack), 0);

  for (i = 0; i < sizeof(time_codes); ++i) {
    uint8_t code[C_HEADER_IDX + 1 + C_DATA_SIZE + 1] = { 0 };

    code[C_SPW_ID_IDX] = message[C_SPW_ID_IDX];
    code[C_PWORD_IDX] = C_MSGQ_TIME;
    code[C_HEADER_IDX + 1] = 1;
    code[C_HEADER_IDX + 1 + C_DATA_SIZE] = time_codes[i];
    nanosleep(&tick_period, NULL);
    mq_send(to_qemu, (const char *) code, sizeof(code), 0);
  }

  return EXIT_SUCCESS;
}
