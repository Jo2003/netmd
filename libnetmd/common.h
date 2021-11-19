#ifndef LIBNETMD_COMMON_H
#define LIBNETMD_COMMON_H

#include <libusb-1.0/libusb.h>

/**
   Typedef that nearly all netmd_* functions use to identify the USB connection
   with the minidisc player.
*/
typedef libusb_device_handle *netmd_dev_handle;

/**
  Function to exchange command/response buffer with minidisc player.

  @param dev device handle
  @param cmd buffer with the command, that should be send to the player
  @param cmdlen length of the command
  @param rsp buffer where the response should be written to
  @return number of bytes received if >0, or error if <0
*/
int netmd_exch_message(netmd_dev_handle *dev, unsigned char *cmd,
                       const size_t cmdlen, unsigned char *rsp);

//------------------------------------------------------------------------------
//! @brief      exchange command / response (extended version)
//!
//! @param      devh    The devh
//! @param      cmd     The command
//! @param[in]  cmdlen  The cmdlen
//! @param      rspPtr  The rsp pointer (must be freed afterwards if not NULL)
//!
//! @return     < 0 -> error; else -> received bytes
//------------------------------------------------------------------------------
int netmd_exch_message_ex(netmd_dev_handle *devh, unsigned char *cmd,
                          const size_t cmdlen, unsigned char **rspPtr);

/**
  Function to send a command to the minidisc player.

  @param dev device handle
  @param cmd buffer with the command, that should be send to the player
  @param cmdlen length of the command
*/
int netmd_send_message(netmd_dev_handle *dev, unsigned char *cmd,
                       const size_t cmdlen);


/**
  Function to recieve a response from the minidisc player.

  @param rsp buffer where the response should be written to
  @return number of bytes received if >0, or error if <0
*/
int netmd_recv_message(netmd_dev_handle *dev, unsigned char *rsp);

//------------------------------------------------------------------------------
//! @brief      receive a message (extended version)
//!
//! @param      devh    The devh
//! @param      rspPtr  The rsp pointer (must be freed afterwards)
//!
//! @return     length of data stored in *rspPtr
//------------------------------------------------------------------------------
int netmd_recv_message_ex(netmd_dev_handle *devh, unsigned char** rspPtr);

/**
   Wait for the device to respond to commands. Should only be used
   when the device needs to be given "breathing room" and is not
   expected to have anything to send.

   @param dev device handle
   @return 1 if success, 0 if there was no response
*/
int netmd_wait_for_sync(netmd_dev_handle* dev);

#endif
