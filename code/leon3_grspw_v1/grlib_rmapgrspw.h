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

#ifndef GRLIB_RMAPGRSPW_H_
#define GRLIB_RMAPGRSPW_H_

#include "grlib_typesgrspw.h"
#include "grlib_comgrspw.h"

uint8 grlib_rmapgrspw_crc(const uint8 data[], const uint32 len);
boolean grlib_rmapgrspw_enabled(const t_grspw_regptr grspw_regptr);
boolean grlib_rmapgrspw_is_command(const t_grspw_regptr grspw_regptr, const uint8 data[], const uint32 len);
void grlib_rmapgrspw_command(t_grsw_deviceptr deviceptr, const uint8 data[], const uint32 len);
uint32 grlib_rmapgrspw_rxflags(const t_grspw_regptr grspw_regptr, const uint8 data[], const uint32 len);

#endif /* GRLIB_RMAPGRSPW_H_ */
