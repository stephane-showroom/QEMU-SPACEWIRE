/********************************************************************************
 *                                                                              *
 *    ▀▄   ▄▀                                                                   *
 *   ▄█▀███▀█▄    Created on: Jul 5, 2015                                       *
 *  █▀███████▀█   Author: Stephane                                              *
 *  █ █▀▀▀▀▀█ █                                                                 *
 *     ▀▀ ▀▀                                                                    *
 *                                                                              *
 * Copyright 2015 grlib_apbgrspw.h                                              *
 * Released under the "CeCILL-C" license                                        *
 * http://www.cecill.info                                                       *
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
