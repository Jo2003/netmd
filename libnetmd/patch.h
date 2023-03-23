/**
 * Copyright (C) 2023 Jo2003 (olenka.joerg@gmail.com)
 * This file is part of netmd
 *
 * cd2netmd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cd2netmd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 */
#ifndef PATCH_H
#define PATCH_H

#include "common.h"
#include "error.h"

/* copy start */

//------------------------------------------------------------------------------
//! @brief      appy SP patch
//!
//! @param[in]  devh         device handle
//! @param[in]  chan_no      number of audio channels (1: mono, 2: stereo)
//!
//! @return     netmd_error
//! @see        netmd_error
//------------------------------------------------------------------------------
netmd_error netmd_apply_sp_patch(netmd_dev_handle *devh, int chan_no);

//------------------------------------------------------------------------------
//! @brief      undo SP upload patch
//!
//! @param[in]  devh  device handle
//------------------------------------------------------------------------------
void netmd_undo_sp_patch(netmd_dev_handle *devh);

//------------------------------------------------------------------------------
//! @brief      check if device supports sp upload
//!
//! @param[in]  devh  device handle
//!
//! @return     0 -> no support; esle
//------------------------------------------------------------------------------
int netmd_dev_supports_sp_upload(netmd_dev_handle *devh);

/* copy end */

#endif // PATCH_H
