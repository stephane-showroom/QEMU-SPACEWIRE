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

#ifndef GRLIB_APBGRSPW_H_
#define GRLIB_APBGRSPW_H_

#include "hw/core/sysbus.h"
#include "chardev/char-fe.h"

#include "grlib_typesgrspw.h"
#include "grlib_reggrspw.h"
#include "grlib_comgrspw.h"

#define TYPE_GRLIB_APB_GRSPW "grlib-grspw"


/**
 * Spacewire links core definition
 * */
typedef struct {
  MemoryRegion iomem;       ///< spacewire memory region mapping

  CharFrontend chr;        ///< optional character backend (chrdevN property)

  t_grsw_device device;

} t_grspw_core, *t_grspw_coreptr;


#endif /* GRLIB_APBGRSPW_H_ */
