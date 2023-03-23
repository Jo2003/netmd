#!/bin/bash

FNAME=libnetmd.h
HEADERS=("const.h" "error.h" "log.h" "common.h" "libnetmd_intern.h" "netmd_dev.h" "netmd_transfer.h" "patch.h" "CMDiscHeader.h" "secure.h" "trackinformation.h" "utils.h")

cat << EOF > ${FNAME}
/*
 * libnetmd_intern.h
 *
 * This file is part of libnetmd, a library for accessing Sony NetMD devices.
 *
 * Copyright (C) 2002, 2003 Marc Britten
 * Copyright (C) 2011 Alexander Sulfrian
 * Copyright (C) 2021 - 2023 Jo2003 (olenka.joerg@gmail.com)
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
#ifndef LIBNETMD_H
#define LIBNETMD_H
#include <stdio.h>
#include <libusb-1.0/libusb.h>
#include <stdint.h>

#ifdef __cpluplus
extern "C" {
#endif /* __cplusplus */
EOF

for h in ${HEADERS[@]} ; do
    echo "Processing header ${h} ..."
    gawk '/\/\* copy start \*\//{flag=1;next}/\/\* copy end \*\//{flag=0}flag' ${h} >> ${FNAME}
done

cat << EOF >> ${FNAME}
#ifdef __cpluplus
}
#endif /* __cplusplus */
#endif /* LIBNETMD_H */
EOF