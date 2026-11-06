/********************************************************************************
 *                                                                              *
 *    ▀▄   ▄▀                                                                   *
 *   ▄█▀███▀█▄    Created on: Nov 6, 2026                                       *
 *  █▀███████▀█   Author: Stephane                                              *
 *  █ █▀▀▀▀▀█ █                                                                 *
 *     ▀▀ ▀▀                                                                    *
 *                                                                              *
 * Copyright 2015 peer_data_before_ack.c                                        *
 * Released under the "CeCILL-C" license                                        *
 * http://www.cecill.info                                                       *
 ********************************************************************************/

/*
 * Peer that answers before acknowledging, like a device that sends a packet of
 * its own while the guest is transmitting.
 */

#include <fcntl.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "grlib_mqgrspw.h"

#define QUEUE_MSG_COUNT   10

int main(void)
{
  struct mq_attr attr = {
    .mq_maxmsg = QUEUE_MSG_COUNT,
    .mq_msgsize = C_MAXQ_SIZE,
  };
  mqd_t from_qemu = mq_open(C_SPWQ, O_CREAT | O_RDONLY, 0644, &attr);
  mqd_t to_qemu = mq_open(C_DEVQ, O_CREAT | O_WRONLY, 0644, &attr);

  if (from_qemu == (mqd_t) -1 || to_qemu == (mqd_t) -1) {
    perror("mq_open");
    return EXIT_FAILURE;
  }

  for (;;) {
    uint8_t message[C_MAXQ_SIZE];
    uint8_t ack[C_HEADER_IDX + 1 + C_DATA_SIZE] = { 0 };
    ssize_t size = mq_receive(from_qemu, (char *) message, sizeof(message), NULL);

    if (size < 0) {
      perror("mq_receive");
      return EXIT_FAILURE;
    }

    ack[C_SPW_ID_IDX] = message[C_SPW_ID_IDX];
    ack[C_PWORD_IDX] = C_MSGQ_ACK;

    mq_send(to_qemu, (const char *) message, (size_t) size, 0);
    mq_send(to_qemu, (const char *) ack, sizeof(ack), 0);
  }
}
