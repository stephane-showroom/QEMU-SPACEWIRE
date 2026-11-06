/********************************************************************************
 *                                                                              *
 *    ▀▄   ▄▀                                                                   *
 *   ▄█▀███▀█▄    Created on: Nov 6, 2026                                       *
 *  █▀███████▀█   Author: Stephane                                              *
 *  █ █▀▀▀▀▀█ █                                                                 *
 *     ▀▀ ▀▀                                                                    *
 *                                                                              *
 * Copyright 2015 grlib_rmapgrspw.h                                             *
 * Released under the "CeCILL-C" license                                        *
 * http://www.cecill.info                                                       *
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
