/* netmdcli.h
 *      Copyright (C) 2017 René Rebe
 *      Copyright (C) 2002, 2003 Marc Britten
 *
 * This file is part of libnetmd.
 *
 * libnetmd is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Libnetmd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
#include <unistd.h>
#include <stdint.h>
#include <libnetmd.h>
#include <utils.h>

#ifdef __cplusplus
extern "C" {
#endif
void netmd_cli_set_log_fd(FILE* fd);
netmd_error netmd_cli_send_track(netmd_dev_handle *devh, const char *filename, const char *in_title, unsigned char onTheFlyConvert);
#ifdef __cplusplus
}
#endif
