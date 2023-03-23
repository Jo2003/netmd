/*
 * libnetmd_intern.h
 *
 * This file is part of libnetmd, a library for accessing Sony NetMD devices.
 *
 * Copyright (C) 2002, 2003 Marc Britten
 * Copyright (C) 2011 Alexander Sulfrian
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#ifndef LIBNETMD_INTERN_H
#define LIBNETMD_INTERN_H
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include <libusb-1.0/libusb.h>

#include "const.h"
#include "error.h"
#include "common.h"
#include "playercontrol.h"
#include "log.h"
#include "secure.h"
#include "netmd_dev.h"
#include "trackinformation.h"
#include "CMDiscHeader.h"
#include "patch.h"
#include "netmd_transfer.h"

/* copy start */

/**
   Data about a group, start track, finish track and name. Used to generate disc
   header info.
*/
typedef struct netmd_group
{
    uint16_t start;
    uint16_t finish;
    char* name;
} netmd_group_t;

/**
   Basic track data.
*/
struct netmd_track
{
    int track;
    int minute;
    int second;
    int tenth;
};

/**
   stores hex value from protocol and text value of name
*/
typedef struct netmd_pair
{
    unsigned char hex;
    const char* const name;
} netmd_pair_t;

/**
   stores misc data for a minidisc
*/
typedef struct {
    size_t header_length;
    struct netmd_group *groups;
    unsigned int group_count;
} minidisc;

typedef enum {
    // TD - text database
    discTitleTD,
    audioUTOC1TD,
    audioUTOC4TD,
    DSITD,
    audioContentsTD,
    rootTD,

    discSubunitIndentifier,
    operatingStatusBlock, // Real name unknown
} netmd_descriptor_t;

typedef struct
{
    netmd_descriptor_t descr;
    uint8_t data[3];
    size_t sz;
} netmd_descr_val_t;

typedef enum {
    nda_openread = 0x01,
    nda_openwrite = 0x03,
    nda_close = 0x00,
} netmd_descriptor_action_t;

/**
   Global variable containing netmd_group data for each group. There will be
   enough for group_count total in the alloced memory
*/
extern struct netmd_group* groups;
extern struct netmd_pair const trprot_settings[];
extern struct netmd_pair const bitrates[];
extern struct netmd_pair const unknown_pair;

/**
   enum through an array of pairs looking for a specific hex code.
   @param hex hex code to find.
   @param pair array of pairs to look through.
*/
struct netmd_pair const* find_pair(int hex, struct netmd_pair const* pair);

/**
 * @brief      get track time
 *
 * @param      dev     The MD dev
 * @param[in]  track   The track number
 * @param      buffer  The time buffer
 *
 * @return     int
 */
int netmd_request_track_time(netmd_dev_handle* dev, const uint16_t track, struct netmd_track* buffer);

/**
   Sets title for the specified track. If making multiple changes,
   call netmd_cache_toc before netmd_set_title and netmd_sync_toc
   afterwards.

   @param dev pointer to device returned by netmd_open
   @param track Zero based index of track your requesting.
   @param buffer buffer to hold the name.
   @return returns 0 for fail 1 for success.
*/
int netmd_set_title(netmd_dev_handle* dev, const uint16_t track, const char* const buffer);

/**
   Moves track around the disc.

   @param dev pointer to device returned by netmd_open
   @param start Zero based index of track to move
   @param finish Zero based track to make it
   @return 0 for fail 1 for success
*/
int netmd_move_track(netmd_dev_handle* dev, const uint16_t start, const uint16_t finish);

/**
   Sets title for the specified track.

   @param dev pointer to device returned by netmd_open
   @param md pointer to minidisc structure
   @param group Zero based index of group your renaming (zero is disc title).
   @param title buffer holding the name.
   @return returns 0 for fail 1 for success.
*/
int netmd_set_group_title(netmd_dev_handle* dev, HndMdHdr md, unsigned int group, const char* title);

/**
   sets up buffer containing group info.

  @param dev pointer to device returned by netmd_open
  @param md pointer to minidisc structure
  @return total size of disc header Group[0] is disc name.  You need to make
          sure you call clean_disc_info before recalling
*/
int netmd_initialize_disc_info(netmd_dev_handle* devh, HndMdHdr* md);

void print_groups(HndMdHdr md);

int netmd_create_group(netmd_dev_handle* dev, HndMdHdr md, char* name, int first, int last);

int netmd_set_disc_title(netmd_dev_handle* dev, char* title, size_t title_length);

/**
   Moves track into group

   @param dev pointer to device returned by netmd_open
   @param md pointer to minidisc structure
   @param track Zero based track to add to group.
   @param group number of group (0 is title group).
*/
int netmd_put_track_in_group(netmd_dev_handle* dev, HndMdHdr md, const uint16_t track, const unsigned int group);

/**
   Removes track from group

   @param dev pointer to device returned by netmd_open
   @param md pointer to minidisc structure
   @param track Zero based track to add to group.
   @param group number of group (0 is title group).
*/
int netmd_pull_track_from_group(netmd_dev_handle* dev, HndMdHdr md, const uint16_t track, const unsigned int group);

/**
   Deletes group from disc (but not the tracks in it)

   @param dev pointer to device returned by netmd_open
   @param md pointer to minidisc structure
   @param group Zero based track to delete
*/
int netmd_delete_group(netmd_dev_handle* dev, HndMdHdr md, const unsigned int group);

/**
   Creates disc header out of groups and writes it to disc

   @param devh pointer to device returned by netmd_open
   @param md pointer to minidisc structure
*/
int netmd_write_disc_header(netmd_dev_handle* devh, HndMdHdr md);

/**
   Writes atrac file to device

   @param dev pointer to device returned by netmd_open
   @param szFile Full path to file to write.
   @return < 0 on fail else 1
   @bug doesnt work yet
*/
int netmd_write_track(netmd_dev_handle* devh, char* szFile);

/**
   Deletes track from disc (does not update groups)

   @param dev pointer to device returned by netmd_open
   @param track Zero based track to delete
*/
int netmd_delete_track(netmd_dev_handle* dev, const uint16_t track);

/**
   Erase all disc contents

   @param dev pointer to device returned by netmd_open
*/
int netmd_erase_disc(netmd_dev_handle* dev);

/* AV/C Description Spefication OPEN DESCRIPTOR (0x08),
 * subfunction "open for write" (0x03) */
int netmd_cache_toc(netmd_dev_handle* dev);

/* AV/C Description Spefication OPEN DESCRIPTOR (0x08),
 * subfunction "close" (0x00) */
int netmd_sync_toc(netmd_dev_handle* dev);

/* Calls need for Sharp devices */
int netmd_acquire_dev(netmd_dev_handle* dev);

int netmd_release_dev(netmd_dev_handle* dev);

/*! @brief      request disc title (raw header)

    @param      dev     The dev
    @param      buffer  The buffer
    @param[in]  size    The size

    @return     title size
 */
int netmd_request_raw_header(netmd_dev_handle* dev, char* buffer, size_t size);

int netmd_request_raw_header_ex(netmd_dev_handle* dev, char** buffer);

//------------------------------------------------------------------------------
//! @brief      get track count from MD device
//!
//! @param[in]  dev     The dev handle
//! @param[out] tcount  The tcount buffer
//!
//! @return     0 -> ok; -1 -> error
//------------------------------------------------------------------------------
int netmd_request_track_count(netmd_dev_handle* dev, uint16_t* tcount);

//------------------------------------------------------------------------------
//! @brief      get disc flags 
//!
//! @param      dev    The dev
//! @param      flags  buffer for the flags
//!
//! @return     0 -> ok; -1 -> error
//------------------------------------------------------------------------------
int netmd_request_disc_flags(netmd_dev_handle* dev, uint8_t* flags);

//------------------------------------------------------------------------------
//! @brief      change descriptor state
//!
//! @param      devh[in]   device handle
//! @param      descr[in]  descriptor
//! @param      act[in]    descriptor action
//!
//! @return     0 -> ok; -1 -> error
//------------------------------------------------------------------------------
int netmd_change_descriptor_state(netmd_dev_handle* devh, netmd_descriptor_t descr, netmd_descriptor_action_t act);

/* copy end */

#endif /* LIBNETMD_INTERN_H */