#ifndef NETMD_TRANSFER_H
#define NETMD_TRANSFER_H
#include "common.h"
#include "error.h"

/* copy start */

//------------------------------------------------------------------------------
//! @brief      send audio file to netmd device
//!
//! @param      devh[in]     device handle
//! @param      filename[in] audio track file name
//! @param      in_title[in] track title
//! @param      otf[in]      on the fly convert flag
//!
//! @return     netmd_error
//! @see        betmd_error
//------------------------------------------------------------------------------
netmd_error netmd_send_track(netmd_dev_handle *devh, const char *filename, const char *in_title, unsigned char otf);

/* copy end */

#endif // NETMD_TRANSFER_H