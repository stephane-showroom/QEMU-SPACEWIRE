/* SPDX-License-Identifier: GPL-2.0-or-later */
/********************************************************************************
 *                                                                              *
 *    ▀▄   ▄▀                                                                   *
 *   ▄█▀███▀█▄    Created on: Jul 5, 2015                                       *
 *  █▀███████▀█   Author: Stephane GONZALEZ                                     *
 *  █ █▀▀▀▀▀█ █                                                                 *
 *     ▀▀ ▀▀                                                                    *
 *                                                                              *
 * Copyright (C) 2015-2026 Stephane GONZALEZ                                    *
 * Released under the GNU GPL v2 or later, see LICENSE                          *
 ********************************************************************************/

#include "qemu/osdep.h"
#include <mqueue.h>
#include <pthread.h>
#include "qemu/main-loop.h"

#include "grlib_apbgrspw.h"
#include "grlib_coregrspw.h"
#include "grlib_comgrspw.h"
#include "grlib_mqgrspw.h"
#include "grlib_typesgrspw.h"
#include "grlib_reggrspw.h"
#include "grlib_loggrspw.h"
#include "grlib_utilgrspw.h"

void * grlib_coregrspw_rxthread(void *arg);

void grlib_coregrspw_init(t_grspw_core core[e_grspw_corenb])
{
  /* thread id */
  pthread_t rxthread = 0;

  grlib_mqgrspw_init();

  // create rx thread
  CHECK_FATAL(0 != pthread_create( &rxthread, NULL, grlib_coregrspw_rxthread, core ), "create rx thread fail");
}


uint8 g_rxdata[C_MAX_DATA_LEN];         ///< global rx data buffer
uint8 g_rxheader[C_MAX_HEADER_LEN];       ///< global rx header buffer

void * grlib_coregrspw_rxthread(void *arg)
{
  t_grspw_coreptr core[e_grspw_corenb] = { (t_grspw_coreptr) arg, (t_grspw_coreptr) arg + 1, (t_grspw_coreptr) arg + 2,
      (t_grspw_coreptr) arg + 3 };

  boolean must_stop = e_false;

  t_grspw_id grspw_id;                  ///< the spacewire id
  uint8 pword;                    ///< the protocol word value
  uint8 header_len;               ///< the header length
  uint32 data_len;                 ///< the data length;
  t_recv_att recv_att = { e_false, e_false, e_false };
  t_grsw_deviceptr deviceptr;
  do
  {

    grlib_mqgrspw_receive_devpacket(
        &grspw_id,                ///< the spacewire id
        &pword,                    ///< the protocol word value
        g_rxheader,                   ///< the header content
        &header_len,              ///< the header length
        g_rxdata,                     ///< the data content
        &data_len);                ///< the data length;

    deviceptr = &core[grspw_id]->device;

    // a SpaceWire packet is the concatenation of the header and the data
    if (header_len > 0)
    {
      memmove(&g_rxdata[header_len], g_rxdata, data_len);
      memcpy(g_rxdata, g_rxheader, header_len);
      data_len += header_len;
    }

    // guest RAM, registers and IRQ are touched: the BQL is required
    bql_lock();
    switch (pword)
    {
      case C_MSGQ_ACK:
        // upon device ack message, end the pending transmission
        grlib_comgrspw_txack(deviceptr);
        break;
      case C_MSGQ_DATA:
        // upon device data message, manage Rx incoming message
        grlib_comgrspw_rxenable(deviceptr, g_rxdata, data_len, &recv_att);
        break;
      case C_MSGQ_TIME:
        // upon device time message, up date the time code register
        grlib_comgrspw_rxtimecode(deviceptr, g_rxdata[0]);
        break;
    }
    bql_unlock();
  }
  while (!must_stop);

  /* cleanup */
  grlib_mqgrspw_devdestroy();

  return NULL;
}
