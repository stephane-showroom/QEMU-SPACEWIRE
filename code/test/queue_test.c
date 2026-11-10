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
 * Checks device_simu at message-queue level: this program takes the place of
 * QEMU on /spwq and /devq.
 */

#include <fcntl.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "grlib_mqgrspw.h"

#define QUEUE_MSG_COUNT   10
#define ACK_SIZE          7

static mqd_t to_simu;
static mqd_t from_simu;

static void fail(const char *message)
{
  printf("TEST FAILED: %s\n", message);
  exit(EXIT_FAILURE);
}

static void send_to_simu(const uint8_t *message, size_t size)
{
  if (mq_send(to_simu, (const char *) message, size, 0) != 0) {
    fail("mq_send");
  }
}

static ssize_t receive_from_simu(uint8_t *message)
{
  struct timespec deadline;

  clock_gettime(CLOCK_REALTIME, &deadline);
  deadline.tv_sec += 1;

  return mq_timedreceive(from_simu, (char *) message, C_MAXQ_SIZE, NULL, &deadline);
}

static void expect(const uint8_t *expected, size_t size, const char *what)
{
  uint8_t message[C_MAXQ_SIZE];

  if (receive_from_simu(message) != (ssize_t) size
      || memcmp(message, expected, size) != 0) {
    fail(what);
  }
}

static void expect_nothing(const char *what)
{
  uint8_t message[C_MAXQ_SIZE];

  if (receive_from_simu(message) >= 0) {
    fail(what);
  }
}

int main(void)
{
  struct mq_attr attr = {
    .mq_maxmsg = QUEUE_MSG_COUNT,
    .mq_msgsize = C_MAXQ_SIZE,
  };
  /* The address and protocol identifier are letters on purpose: they are kept. */
  static const uint8_t data[] = { 2, C_MSGQ_DATA, 0, 4, 0, 0, 0, 'a', 'b', 'c', 0xcc };
  static const uint8_t upper[] = { 2, C_MSGQ_DATA, 0, 4, 0, 0, 0, 'a', 'b', 'C', 0xcc };
  static const uint8_t header_data[] = { 3, C_MSGQ_DATA, 3, 'a', 'b', 'c', 1, 0, 0, 0, 'd' };
  static const uint8_t header_upper[] = { 3, C_MSGQ_DATA, 3, 'a', 'b', 'C', 1, 0, 0, 0, 'D' };
  static const uint8_t ack_core3[ACK_SIZE] = { 3, C_MSGQ_ACK };
  static const uint8_t ack_core2[ACK_SIZE] = { 2, C_MSGQ_ACK };
  static const uint8_t time_code[] = { 1, C_MSGQ_TIME, 0, 1, 0, 0, 0, 0x2a };
  static const uint8_t malformed[] = { 0, C_MSGQ_DATA, 0, 9, 0, 0, 0, 0xaa };
  static const uint8_t ack_core0[ACK_SIZE] = { 0, C_MSGQ_ACK };

  to_simu = mq_open(C_SPWQ, O_CREAT | O_WRONLY, 0644, &attr);
  from_simu = mq_open(C_DEVQ, O_CREAT | O_RDONLY, 0644, &attr);
  if (to_simu == (mqd_t) -1 || from_simu == (mqd_t) -1) {
    fail("mq_open");
  }

  send_to_simu(data, sizeof(data));
  expect(ack_core2, sizeof(ack_core2), "data packet not acknowledged");
  expect(upper, sizeof(upper), "data packet not sent back in upper case");

  send_to_simu(header_data, sizeof(header_data));
  expect(ack_core3, sizeof(ack_core3), "packet with header not acknowledged");
  expect(header_upper, sizeof(header_upper), "packet with header not in upper case");

  send_to_simu(time_code, sizeof(time_code));
  expect(time_code, sizeof(time_code), "time code not looped back");

  send_to_simu(malformed, sizeof(malformed));
  expect(ack_core0, sizeof(ack_core0), "malformed packet not acknowledged");
  expect_nothing("malformed packet looped back");

  printf("TEST OK\n");
  return EXIT_SUCCESS;
}
