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
 * RMAP initiator: exercises the RMAP target of the QEMU GRSPW device, see
 * section 77.6 of doc/grspw-1.md and ECSS-E-ST-50-52C.
 *
 * The CRC is first checked against the test patterns of the standard. The guest
 * then sends the address of a memory area in one DMA packet, from core 2 or 3,
 * and gets one DMA packet back when all the checks passed. It has a node address
 * of 0x42 and a destination key of 0x20. The other core has its RMAP target
 * disabled.
 */

#include <fcntl.h>
#include <mqueue.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "grlib_mqgrspw.h"

#define NODE_GUEST        0x42
#define NODE_INITIATOR    0x67
#define KEY_GUEST         0x20
#define KEY_WRONG         0x21
#define PROTOCOL_RMAP     0x01
#define PROTOCOL_USER     0xF0

#define INS_WRITE_ACK_INC     0x6C
#define INS_WRITE_ACK_SINGLE  0x68
#define INS_WRITE_NOACK_INC   0x64
#define INS_VERIFY_ACK_INC    0x7C
#define INS_READ_INC          0x4C
#define INS_READ_SINGLE       0x48
#define INS_RMW               0x5C
#define INS_UNUSED_REPLY      0x58

#define STATUS_OK             0
#define STATUS_GENERAL        1
#define STATUS_UNUSED         2
#define STATUS_KEY            3
#define STATUS_DATA_CRC       4
#define STATUS_EARLY_EOP      5
#define STATUS_TOO_MUCH_DATA  6
#define STATUS_VERIFY_OVERRUN 9
#define STATUS_NOT_AUTHORISED 10
#define STATUS_RMW_LENGTH     11

#define COMMAND_HEADER_SIZE   16
#define READ_REPLY_HEADER     12
#define WRITE_REPLY_HEADER    8

#define UNMAPPED_ADDRESS      0x10000000U
#define REPLY_TIMEOUT_NS      100000000L
#define GUEST_TIMEOUT_S       5
#define MSG_HEADER_SIZE       (C_HEADER_IDX + 1 + C_DATA_SIZE)
#define QUEUE_MSG_COUNT       10
#define NO_DATA               ((const uint8_t *) NULL)

typedef struct {
  const char *name;
  uint8_t header[32];
  size_t header_size;
  uint8_t header_crc;
  bool has_data;
  uint8_t data_crc;
} crc_pattern;

/* Annex A of ECSS-E-ST-50-52C: bytes covered by the CRC, with the expected CRC. */
#define DATA_1 \
  0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, \
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17
#define DATA_2 \
  0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, \
  0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF

static const uint8_t pattern_data_1[] = { DATA_1 };
static const uint8_t pattern_data_2[] = { DATA_2 };

static const crc_pattern patterns[] = {
  { "write command", { 0xFE, 0x01, 0x6C, 0x00, 0x67, 0x00, 0x00, 0x00, 0xA0, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x10 }, 15, 0x9F, true, 0x56 },
  { "write reply", { 0x67, 0x01, 0x2C, 0x00, 0xFE, 0x00, 0x00 }, 7, 0xED, false, 0 },
  { "read command", { 0xFE, 0x01, 0x4C, 0x00, 0x67, 0x00, 0x01, 0x00, 0xA0, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x10 }, 15, 0xC9, false, 0 },
  { "read reply", { 0x67, 0x01, 0x0C, 0x00, 0xFE, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x10 }, 11, 0x6D, true, 0x56 },
  { "write command with addresses", { 0xFE, 0x01, 0x6E, 0x00, 0x00, 0x99, 0xAA, 0xBB,
      0xCC, 0xDD, 0xEE, 0x00, 0x67, 0x00, 0x02, 0x00, 0xA0, 0x00, 0x00, 0x10, 0x00,
      0x00, 0x10 }, 23, 0x7F, true, 0xB4 },
  { "write reply with addresses", { 0x67, 0x01, 0x2E, 0x00, 0xFE, 0x00, 0x02 }, 7,
      0x1D, false, 0 },
  { "read command with addresses", { 0xFE, 0x01, 0x4D, 0x00, 0x99, 0xAA, 0xBB, 0xCC,
      0x67, 0x00, 0x03, 0x00, 0xA0, 0x00, 0x00, 0x10, 0x00, 0x00, 0x10 }, 19, 0xF7,
      false, 0 },
  { "read reply with addresses", { 0x67, 0x01, 0x0D, 0x00, 0xFE, 0x00, 0x03, 0x00,
      0x00, 0x00, 0x10 }, 11, 0x52, true, 0xB4 },
};

static mqd_t from_guest;
static mqd_t to_guest;
static int core_rmap;
static uint32_t target;
static uint16_t next_tid;
static int failures;

static uint8_t crc8(const uint8_t *data, size_t size)
{
  uint8_t crc = 0;

  while (size-- > 0) {
    int bit;

    crc ^= *data++;
    for (bit = 0; bit < 8; ++bit) {
      crc = (crc & 1) ? (uint8_t) ((crc >> 1) ^ 0xE0) : (uint8_t) (crc >> 1);
    }
  }

  return crc;
}

static void check(bool condition, const char *name)
{
  if (!condition) {
    printf("RMAP TEST FAILED: %s\n", name);
    fflush(stdout);
    ++failures;
  }
}

static void check_patterns(void)
{
  size_t i;

  for (i = 0; i < sizeof(patterns) / sizeof(patterns[0]); ++i) {
    const crc_pattern *pattern = &patterns[i];
    const uint8_t *data = pattern->data_crc == 0x56 ? pattern_data_1 : pattern_data_2;

    check(crc8(pattern->header, pattern->header_size) == pattern->header_crc, pattern->name);
    if (pattern->has_data) {
      check(crc8(data, sizeof(pattern_data_1)) == pattern->data_crc, pattern->name);
    }
  }
}

static void send_packet(int core, uint8_t word, const uint8_t *packet, uint32_t size)
{
  uint8_t message[C_MAXQ_SIZE] = { 0 };

  message[C_SPW_ID_IDX] = (uint8_t) core;
  message[C_PWORD_IDX] = word;
  memcpy(&message[C_HEADER_IDX + 1], &size, sizeof(size));
  if (size > 0) {
    memcpy(&message[MSG_HEADER_SIZE], packet, size);
  }

  if (mq_send(to_guest, (const char *) message, MSG_HEADER_SIZE + size, 0) != 0) {
    perror("mq_send");
    exit(EXIT_FAILURE);
  }
}

static ssize_t receive_message(uint8_t *message, long seconds, long nanoseconds)
{
  struct timespec deadline;

  clock_gettime(CLOCK_REALTIME, &deadline);
  deadline.tv_sec += seconds;
  deadline.tv_nsec += nanoseconds;
  if (deadline.tv_nsec >= 1000000000L) {
    deadline.tv_nsec -= 1000000000L;
    ++deadline.tv_sec;
  }

  return mq_timedreceive(from_guest, (char *) message, C_MAXQ_SIZE, NULL, &deadline);
}

static void wait_guest_address(void)
{
  uint8_t message[C_MAXQ_SIZE];
  const uint8_t *packet = &message[MSG_HEADER_SIZE];

  if (receive_message(message, GUEST_TIMEOUT_S, 0) < MSG_HEADER_SIZE + 6) {
    printf("RMAP TEST FAILED: no packet from the guest\n");
    exit(EXIT_FAILURE);
  }

  core_rmap = message[C_SPW_ID_IDX];
  send_packet(core_rmap, C_MSGQ_ACK, NULL, 0);
  target = (uint32_t) packet[2] << 24 | (uint32_t) packet[3] << 16
         | (uint32_t) packet[4] << 8 | packet[5];
}

/* Builds a command header and returns its size. */
static size_t build_header(uint8_t *packet, uint8_t instruction, uint8_t key,
                           const uint8_t *reply_address, uint32_t address, uint32_t length)
{
  size_t reply_size = (size_t) (instruction & 3) * 4;
  size_t tail = reply_size;
  uint16_t tid = next_tid++;

  packet[0] = NODE_GUEST;
  packet[1] = PROTOCOL_RMAP;
  packet[2] = instruction;
  packet[3] = key;
  if (reply_size > 0) {
    memcpy(&packet[4], reply_address, reply_size);
  }
  packet[tail + 4] = NODE_INITIATOR;
  packet[tail + 5] = (uint8_t) (tid >> 8);
  packet[tail + 6] = (uint8_t) tid;
  packet[tail + 7] = 0;
  packet[tail + 8] = (uint8_t) (address >> 24);
  packet[tail + 9] = (uint8_t) (address >> 16);
  packet[tail + 10] = (uint8_t) (address >> 8);
  packet[tail + 11] = (uint8_t) address;
  packet[tail + 12] = (uint8_t) (length >> 16);
  packet[tail + 13] = (uint8_t) (length >> 8);
  packet[tail + 14] = (uint8_t) length;
  packet[tail + 15] = crc8(packet, tail + 15);

  return tail + COMMAND_HEADER_SIZE;
}

static size_t add_data(uint8_t *packet, size_t header_size, const uint8_t *data, size_t size)
{
  memcpy(&packet[header_size], data, size);
  packet[header_size + size] = crc8(data, size);

  return header_size + size + 1;
}

/*
 * Sends a command and checks the reply. expected_data is the data of a read or
 * RMW reply, NO_DATA for a write reply or when the reply has no data field.
 */
static void exchange(const char *name, const uint8_t *packet, size_t size, int prefix,
                     uint8_t status, bool read_reply, const uint8_t *expected_data,
                     size_t data_size)
{
  uint8_t message[C_MAXQ_SIZE];
  const uint8_t *reply = &message[MSG_HEADER_SIZE + prefix];
  size_t header_size = read_reply ? READ_REPLY_HEADER : WRITE_REPLY_HEADER;
  size_t expected = prefix + header_size;
  ssize_t received;

  send_packet(core_rmap, C_MSGQ_DATA, packet, (uint32_t) size);

  received = receive_message(message, 0, REPLY_TIMEOUT_NS);
  if (received < 0 || message[C_PWORD_IDX] != C_MSGQ_RMAP) {
    check(false, name);
    return;
  }

  if (expected_data != NO_DATA) {
    expected += data_size + 1;
  }

  check((size_t) received == MSG_HEADER_SIZE + expected, name);
  check(reply[0] == NODE_INITIATOR && reply[1] == PROTOCOL_RMAP, name);
  check((reply[2] & 0xC0) == 0 && reply[3] == status && reply[4] == NODE_GUEST, name);
  check(crc8(reply, header_size) == 0, name);

  if (expected_data != NO_DATA) {
    check(memcmp(&reply[READ_REPLY_HEADER], expected_data, data_size) == 0, name);
    check(crc8(&reply[READ_REPLY_HEADER], data_size + 1) == 0, name);
  }
}

static void expect_no_reply(const char *name, int core, const uint8_t *packet, size_t size)
{
  uint8_t message[C_MAXQ_SIZE];

  send_packet(core, C_MSGQ_DATA, packet, (uint32_t) size);
  check(receive_message(message, 0, REPLY_TIMEOUT_NS) < 0, name);
}

static void write_memory(const char *name, uint8_t instruction, uint8_t key, uint32_t address,
                         const uint8_t *data, size_t size, uint8_t status)
{
  uint8_t packet[256];
  size_t header = build_header(packet, instruction, key, NULL, address, (uint32_t) size);

  exchange(name, packet, add_data(packet, header, data, size), 0, status, false, NO_DATA, 0);
}

static void read_memory(const char *name, uint8_t instruction, uint32_t address,
                        const uint8_t *expected, size_t size, uint8_t status)
{
  uint8_t packet[64];
  size_t header = build_header(packet, instruction, KEY_GUEST, NULL, address, (uint32_t) size);

  exchange(name, packet, header, 0, status, true, expected, size);
}

static void test_write_read(void)
{
  static const uint8_t data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  static const uint8_t verified[] = { 0xDE, 0xAD, 0xBE, 0xEF };
  static const uint8_t unaligned[] = { 0xA5, 0x5A };
  static const uint8_t no_ack[] = { 9, 10, 11, 12 };
  uint8_t packet[64];
  size_t header;

  write_memory("incrementing write", INS_WRITE_ACK_INC, KEY_GUEST, target, data, sizeof(data), STATUS_OK);
  read_memory("incrementing read", INS_READ_INC, target, data, sizeof(data), STATUS_OK);

  write_memory("verified write", INS_VERIFY_ACK_INC, KEY_GUEST, target + 8, verified, sizeof(verified), STATUS_OK);
  read_memory("verified write readback", INS_READ_INC, target + 8, verified, sizeof(verified), STATUS_OK);

  write_memory("unaligned incrementing write", INS_WRITE_ACK_INC, KEY_GUEST, target + 13, unaligned, sizeof(unaligned), STATUS_OK);
  read_memory("unaligned incrementing read", INS_READ_INC, target + 13, unaligned, sizeof(unaligned), STATUS_OK);

  /* A single address write accesses the same word again and again. */
  write_memory("single address write", INS_WRITE_ACK_SINGLE, KEY_GUEST, target + 16, data, sizeof(data), STATUS_OK);
  read_memory("single address read", INS_READ_SINGLE, target + 16, &data[4], 4, STATUS_OK);

  header = build_header(packet, INS_WRITE_NOACK_INC, KEY_GUEST, NULL, target + 20, sizeof(no_ack));
  expect_no_reply("write without reply", core_rmap, packet, add_data(packet, header, no_ack, sizeof(no_ack)));
  read_memory("write without reply readback", INS_READ_INC, target + 20, no_ack, sizeof(no_ack), STATUS_OK);
}

static void test_rmw(void)
{
  static const uint8_t initial[] = { 0x12, 0x34, 0x56, 0x78 };
  static const uint8_t command[] = { 0xFF, 0x00, 0xAA, 0x0F, 0xF0, 0x0F, 0xFF, 0x00 };
  static const uint8_t expected[] = { 0xF2, 0x30, 0xAA, 0x78 };
  uint8_t packet[64];
  size_t header;

  write_memory("rmw setup", INS_WRITE_ACK_INC, KEY_GUEST, target + 24, initial, sizeof(initial), STATUS_OK);

  header = build_header(packet, INS_RMW, KEY_GUEST, NULL, target + 24, sizeof(command));
  exchange("rmw", packet, add_data(packet, header, command, sizeof(command)), 0, STATUS_OK,
           true, initial, sizeof(initial));
  read_memory("rmw readback", INS_READ_INC, target + 24, expected, sizeof(expected), STATUS_OK);

  header = build_header(packet, INS_RMW, KEY_GUEST, NULL, target + 24, 6);
  exchange("rmw length 6", packet, add_data(packet, header, command, 6), 0, STATUS_RMW_LENGTH,
           true, NO_DATA, 3);
}

static void test_errors(void)
{
  static const uint8_t reference[] = { 0x55, 0x66, 0x77, 0x88 };
  static const uint8_t other[] = { 0x11, 0x22, 0x33, 0x44 };
  static const uint8_t five[] = { 1, 2, 3, 4, 5 };
  uint8_t packet[64];
  size_t header;
  size_t size;

  write_memory("errors setup", INS_WRITE_ACK_INC, KEY_GUEST, target + 28, reference, sizeof(reference), STATUS_OK);

  write_memory("invalid key", INS_WRITE_ACK_INC, KEY_WRONG, target + 28, other, sizeof(other), STATUS_KEY);

  header = build_header(packet, INS_VERIFY_ACK_INC, KEY_GUEST, NULL, target + 28, sizeof(other));
  size = add_data(packet, header, other, sizeof(other));
  packet[size - 1] ^= 0xFF;
  exchange("verified write with a bad data CRC", packet, size, 0, STATUS_DATA_CRC, false, NO_DATA, 0);

  header = build_header(packet, INS_WRITE_ACK_INC, KEY_GUEST, NULL, target + 28, sizeof(other));
  size = add_data(packet, header, other, sizeof(other));
  packet[header - 1] ^= 0xFF;
  expect_no_reply("bad header CRC", core_rmap, packet, size);

  expect_no_reply("incomplete header", core_rmap, packet, header - 1);

  read_memory("nothing written by the failed commands", INS_READ_INC, target + 28, reference, sizeof(reference), STATUS_OK);

  header = build_header(packet, INS_READ_SINGLE, KEY_GUEST, NULL, target + 1, 4);
  exchange("unaligned single address read", packet, header, 0, STATUS_NOT_AUTHORISED, true, NO_DATA, 4);

  write_memory("verified write of 3 bytes", INS_VERIFY_ACK_INC, KEY_GUEST, target, other, 3, STATUS_NOT_AUTHORISED);
  write_memory("verified write of 5 bytes", INS_VERIFY_ACK_INC, KEY_GUEST, target, five, sizeof(five), STATUS_VERIFY_OVERRUN);

  header = build_header(packet, INS_UNUSED_REPLY, KEY_GUEST, NULL, target, 4);
  exchange("unused command", packet, header, 0, STATUS_UNUSED, true, NO_DATA, 2);

  header = build_header(packet, INS_VERIFY_ACK_INC, KEY_GUEST, NULL, target + 28, sizeof(other));
  size = add_data(packet, header, other, sizeof(other));
  exchange("early EOP", packet, size - 2, 0, STATUS_EARLY_EOP, false, NO_DATA, 0);

  header = build_header(packet, INS_VERIFY_ACK_INC, KEY_GUEST, NULL, target + 28, sizeof(reference));
  size = add_data(packet, header, reference, sizeof(reference));
  packet[size] = 0;
  exchange("too much data", packet, size + 1, 0, STATUS_TOO_MUCH_DATA, false, NO_DATA, 0);

  header = build_header(packet, INS_READ_INC, KEY_GUEST, NULL, target, 4);
  packet[header] = 0;
  exchange("read with data", packet, header + 1, 0, STATUS_TOO_MUCH_DATA, true, NO_DATA, 4);

  write_memory("write to unmapped memory", INS_WRITE_ACK_INC, KEY_GUEST, UNMAPPED_ADDRESS, other, sizeof(other), STATUS_GENERAL);

  /* The read is done on the fly: the header is correct, the packet is truncated. */
  header = build_header(packet, INS_READ_INC, KEY_GUEST, NULL, UNMAPPED_ADDRESS, 4);
  exchange("read of unmapped memory", packet, header, 0, STATUS_OK, true, NO_DATA, 4);

  header = build_header(packet, INS_READ_INC, KEY_GUEST, NULL, target, 4);
  packet[header - 9] = 1;
  packet[header - 1] = crc8(packet, header - 1);
  exchange("extended address", packet, header, 0, STATUS_NOT_AUTHORISED, true, NO_DATA, 4);

  header = build_header(packet, INS_READ_INC, KEY_GUEST, NULL, target, 4);
  expect_no_reply("RMAP target disabled", core_rmap ^ 1, packet, header);
}

static void test_reply_address(void)
{
  static const uint8_t reply_address[] = { 0x00, 0x00, 0x00, 0x55 };
  static const uint8_t reference[] = { 0xCA, 0xFE, 0x12, 0x34 };
  uint8_t packet[64];
  size_t header;

  write_memory("reply address setup", INS_WRITE_ACK_INC, KEY_GUEST, target + 32, reference, sizeof(reference), STATUS_OK);

  header = build_header(packet, INS_READ_INC | 1, KEY_GUEST, reply_address, target + 32, sizeof(reference));
  exchange("reply address", packet, header, 1, STATUS_OK, true, reference, sizeof(reference));
}

/* A read reply goes to the DMA channel, with the DC bit of the descriptor set. */
static void send_bad_data_crc_packet(void)
{
  static const uint8_t data[] = { 1, 2, 3, 4 };
  uint8_t packet[32] = { NODE_GUEST, PROTOCOL_RMAP, 0x0C, 0, NODE_INITIATOR, 0, 0, 0, 0, 0, sizeof(data) };

  packet[11] = crc8(packet, 11);
  memcpy(&packet[12], data, sizeof(data));
  packet[16] = (uint8_t) (crc8(data, sizeof(data)) ^ 0xFF);

  send_packet(core_rmap, C_MSGQ_DATA, packet, 17);
}

static void send_end_of_test(void)
{
  static const uint8_t magic[] = { 'R', 'M', 'A', 'P', '-', 'O', 'K', '!' };
  uint8_t packet[] = { NODE_GUEST, PROTOCOL_USER, 0 };

  write_memory("final write", INS_WRITE_ACK_INC, KEY_GUEST, target, magic, sizeof(magic), STATUS_OK);
  send_packet(core_rmap, C_MSGQ_DATA, packet, sizeof(packet));
}

int main(void)
{
  struct mq_attr attr = {
    .mq_maxmsg = QUEUE_MSG_COUNT,
    .mq_msgsize = C_MAXQ_SIZE,
  };

  from_guest = mq_open(C_SPWQ, O_CREAT | O_RDONLY, 0644, &attr);
  to_guest = mq_open(C_DEVQ, O_CREAT | O_WRONLY, 0644, &attr);
  if (from_guest == (mqd_t) -1 || to_guest == (mqd_t) -1) {
    perror("mq_open");
    return EXIT_FAILURE;
  }

  check_patterns();
  wait_guest_address();
  test_write_read();
  test_rmw();
  test_errors();
  test_reply_address();
  send_bad_data_crc_packet();

  if (failures != 0) {
    printf("RMAP TEST: %d failed check(s)\n", failures);
    return EXIT_FAILURE;
  }

  send_end_of_test();
  printf("RMAP TEST OK\n");
  return EXIT_SUCCESS;
}
