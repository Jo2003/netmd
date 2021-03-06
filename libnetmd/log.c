/*
 * log.c
 *
 * This file is part of libnetmd, a library for accessing Sony NetMD devices.
 *
 * Copyright (C) 2004 Bertrik Sikken
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

#include <stdio.h>
#include <stdarg.h>

#include "log.h"

static netmd_loglevel trace_level = 0;
static FILE* fd_log = NULL;

/**
 * @brief      Sets the log file descriptor
 *
 * @param[in]  fdid  se fdid as stdout
 */
void netmd_log_set_fd(FILE* fdid)
{
    fd_log = fdid;
}

void netmd_set_log_level(netmd_loglevel level)
{
    trace_level = level;
}


void netmd_log_hex(netmd_loglevel level, const unsigned char* const buf, const size_t len)
{
    size_t i;
    size_t j = 0;
    int breakpoint = 0;

    if (!fd_log)
        fd_log = stdout;

    if (level > trace_level) {
        return;
    }

    for (i = 0; i < len; i++)
    {
        fprintf(fd_log, "%02x ", buf[i] & 0xff);
        breakpoint++;
        if(!((i + 1)%16) && i)
        {
            fprintf(fd_log, "\t\t");
            for(j = ((i+1) - 16); j < ((i+1)/16) * 16; j++)
            {
                if(buf[j] < 30)
                    fprintf(fd_log, ".");
                else
                    fprintf(fd_log, "%c", buf[j]);
            }
            fprintf(fd_log, "\n");
            breakpoint = 0;
        }
    }

    if(breakpoint == 16)
    {
        fprintf(fd_log, "\n");
        fflush(fd_log);
        return;
    }

    for(; breakpoint < 16; breakpoint++)
    {
        fprintf(fd_log, "   ");
    }
    fprintf(fd_log, "\t\t");

    for(j = len - (len%16); j < len; j++)
    {
        if(buf[j] < 30)
            fprintf(fd_log, ".");
        else
            fprintf(fd_log, "%c", buf[j]);
    }
    fprintf(fd_log, "\n");
    fflush(fd_log);
}


void netmd_log(netmd_loglevel level, const char* const fmt, ...)
{
    va_list arg;

    if (!fd_log)
        fd_log = stdout;

    if (level > trace_level) {
        return;
    }

    va_start(arg, fmt);
    vfprintf(fd_log, fmt, arg);
    va_end(arg);
    fflush(fd_log);
}
