/*
 * libnetmd.c
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

#include <stdio.h>
#include <unistd.h>

#include "common.h"
#include "libnetmd.h"
#include "log.h"
#include "utils.h"

/*! list of known codecs (mapped to protocol ID) that can be used in NetMD devices */
/*! Bertrik: the original interpretation of these numbers as codecs appears incorrect.
  These values look like track protection values instead */
struct netmd_pair const trprot_settings[] =
{
    {0x00, "UnPROT"},
    {0x03, "TrPROT"},
    {0, 0} /* terminating pair */
};

/*! list of known bitrates (mapped to protocol ID) that can be used in NetMD devices */
struct netmd_pair const bitrates[] =
{
    {NETMD_ENCODING_SP, "SP"},
    {NETMD_ENCODING_LP2, "LP2"},
    {NETMD_ENCODING_LP4, "LP4"},
    {0, 0} /* terminating pair */
};

struct netmd_pair const unknown_pair = {0x00, "UNKNOWN"};

struct netmd_pair const* find_pair(int hex, struct netmd_pair const* array)
{
    int i = 0;
    for(; array[i].name != NULL; i++)
    {
        if(array[i].hex == hex)
            return &array[i];
    }

    return &unknown_pair;
}


static unsigned char* sendcommand(netmd_dev_handle* devh, unsigned char* str, const size_t len, unsigned char* response, int rlen)
{
    int i, ret, size = 0;
    static unsigned char buf[256];

    ret = netmd_exch_message(devh, str, len, buf);
    if (ret < 0) {
        fprintf(stderr, "bad ret code, returning early\n");
        return NULL;
    }

    /* Calculate difference to expected response */
    if (response != NULL) {
        int c=0;
        for (i=0; i < netmd_min(rlen, size); i++) {
            if (response[i] != buf[i]) {
                c++;
            }
        }
        fprintf(stderr, "Differ: %d\n",c);
    }

    return buf;
}

static int request_disc_title(netmd_dev_handle* dev, char* buffer, size_t size)
{
    int ret = -1;
    size_t title_size = 0;
    unsigned char title_request[] = {0x00, 0x18, 0x06, 0x02, 0x20, 0x18,
                                     0x01, 0x00, 0x00, 0x30, 0x00, 0x0a,
                                     0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
                                     0x00};
    unsigned char title[255];

    ret = netmd_exch_message(dev, title_request, 0x13, title);
    if(ret < 0)
    {
        fprintf(stderr, "request_disc_title: bad ret code, returning early\n");
        return 0;
    }

    title_size = (size_t)ret;

    if(title_size == 0 || title_size == 0x13)
        return -1; /* bail early somethings wrong */

    if((title_size - 25) >= size)
    {
        printf("request_disc_title: title too large for buffer\n");
    }
    else
    {
        memset(buffer, 0, size);
        memcpy(buffer, (title + 25), title_size - 25);
        buffer[title_size - 25] = 0;

        netmd_log(NETMD_LOG_DEBUG, "Title control response:\n");
        netmd_log_hex(NETMD_LOG_DEBUG, title, 25);
    }

    return (int)title_size - 25;
}

static int request_disc_title_ex(netmd_dev_handle* dev, char** buffer)
{
    int ret = -1;
    uint16_t total = 1, remaining = 0, read = 0, chunkSz = 0;
    unsigned char hs1[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x01, 0x00};


    unsigned char title_request[] = {0x00, 0x18, 0x06, 0x02, 0x20, 0x18,
                                     0x01, 0x00, 0x00, 0x30, 0x00, 0x0a,
                                     0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
                                     0x00};
    unsigned char tmpBuf[255];
    unsigned char *pResp = NULL;

    uint16_t* pRemains = (uint16_t*)&title_request[15];
    uint16_t* pDone    = (uint16_t*)&title_request[17];

    *buffer = NULL;

    /* handshake 1 */
    netmd_exch_message(dev, hs1, 8, tmpBuf);

    while (read < total)
    {
        /* prepare request */
        *pRemains = netmd_htons(remaining);
        *pDone    = netmd_htons(read);

        netmd_log(NETMD_LOG_DEBUG, "Title request:\n");
        netmd_log_hex(NETMD_LOG_DEBUG, title_request, 19);

        ret = netmd_exch_message_ex(dev, title_request, 0x13, &pResp);

        if(ret < 0)
        {
            netmd_log(NETMD_LOG_ERROR, "request_disc_title: bad ret code, returning early\n");
            if (*buffer != NULL)
            {
                free(*buffer);
                *buffer = NULL;
            }
            return 0;
        }

        if (pResp == NULL)
        {
            netmd_log(NETMD_LOG_ERROR, "No usable response from device!\n");
            return 0;
        }

        if (remaining == 0)
        {
            /* first answer */
            total            = netmd_ntohs(*(uint16_t*)&pResp[23]);
            *buffer          = (char*)malloc(total + 1); /* one more for terminating zero */
            (*buffer)[total] = '\0';
            chunkSz          = netmd_ntohs(*(uint16_t*)&pResp[15]) - 6;
            memcpy((*buffer) + read, &pResp[25], chunkSz);
        }
        else
        {
            chunkSz = netmd_ntohs(*(uint16_t*)&pResp[15]);
            memcpy((*buffer) + read, &pResp[19], chunkSz);
        }

        free(pResp);
        pResp = NULL;

        read += chunkSz;
        remaining = total - read;
    }

    return read;
}

int netmd_request_track_time(netmd_dev_handle* dev, const uint16_t track, struct netmd_track* buffer)
{
    int ret = 0;
    unsigned char hs[] = {0x00, 0x18, 0x08, 0x10, 0x10, 0x01, 0x01, 0x00};
    unsigned char request[] = {0x00, 0x18, 0x06, 0x02, 0x20, 0x10,
                               0x01, 0x00, 0x01, 0x30, 0x00, 0x01,
                               0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
                               0x00};
    unsigned char time_request[255];
    unsigned char *buf;

    buf = request + 7;
    netmd_copy_word_to_buffer(&buf, track, 0);
    netmd_exch_message(dev, hs, 8, time_request);
    ret = netmd_exch_message(dev, request, 0x13, time_request);
    if(ret < 0)
    {
        fprintf(stderr, "bad ret code, returning early\n");
        return 0;
    }

    buffer->minute = bcd_to_proper(time_request + 28, 1) & 0xff;
    buffer->second = bcd_to_proper(time_request + 29, 1) & 0xff;
    buffer->tenth = bcd_to_proper(time_request + 30, 1) & 0xff;
    buffer->track = track;

    return 1;
}

int netmd_set_title(netmd_dev_handle* dev, const uint16_t track, const char* const buffer)
{
    int ret = 1;
    unsigned char *title_request = NULL;
    /* handshakes for 780/980/etc */
    unsigned char hs2[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x00, 0x00};
    unsigned char hs3[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x03, 0x00};

    unsigned char title_header[] = {0x00, 0x18, 0x07, 0x02, 0x20, 0x18,
                                    0x02, 0x00, 0x00, 0x30, 0x00, 0x0a,
                                    0x00, 0x50, 0x00, 0x00, 0x0a, 0x00,
                                    0x00, 0x00, 0x0d};
    unsigned char reply[255];
    unsigned char *buf;
    size_t size;
    int oldsize;

    /* the title update command wants to now how many bytes to replace */
    oldsize = netmd_request_title(dev, track, (char *)reply, sizeof(reply));
    if(oldsize == -1)
        oldsize = 0; /* Reading failed -> no title at all, replace 0 bytes */

    
    size = strlen(buffer);
    title_request = malloc(sizeof(char) * (0x15 + size));
    memcpy(title_request, title_header, 0x15);
    memcpy((title_request + 0x15), buffer, size);

    buf = title_request + 7;
    netmd_copy_word_to_buffer(&buf, track, 0);
    title_request[16] = size & 0xff;
    title_request[20] = oldsize & 0xff;


    /* send handshakes */
    netmd_exch_message(dev, hs2, 8, reply);
    netmd_exch_message(dev, hs3, 8, reply);

    ret = netmd_exch_message(dev, title_request, 0x15 + size, reply);
    free(title_request);

    if(ret < 0)
    {
        netmd_log(NETMD_LOG_WARNING, "netmd_set_title: exchange failed, ret=%d\n", ret);
        return 0;
    }

    return 1;
}

int netmd_move_track(netmd_dev_handle* dev, const uint16_t start, const uint16_t finish)
{
    int ret = 0;
    unsigned char hs[] = {0x00, 0x18, 0x08, 0x10, 0x10, 0x01, 0x00, 0x00};
    unsigned char request[] = {0x00, 0x18, 0x43, 0xff, 0x00, 0x00,
                               0x20, 0x10, 0x01, 0x00, 0x04, 0x20,
                               0x10, 0x01, 0x00, 0x03};
    unsigned char reply[255];
    unsigned char *buf;

    buf = request + 9;
    netmd_copy_word_to_buffer(&buf, start, 0);

    buf = request + 14;
    netmd_copy_word_to_buffer(&buf, finish, 0);

    netmd_exch_message(dev, hs, 8, reply);
    netmd_exch_message(dev, request, 16, reply);
    ret = netmd_exch_message(dev, request, 16, reply);

    if(ret < 0)
    {
        fprintf(stderr, "bad ret code, returning early\n");
        return 0;
    }

    return 1;
}

int netmd_set_group_title(netmd_dev_handle* dev, HndMdHdr md, unsigned int group, char* title)
{
    if (md_header_rename_group(md, group, title) == 0)
    {
        netmd_write_disc_header(dev, md);
        return 1;
    }

    return 0;
}

/* Sonys code is utter bile. So far we've encountered the following first segments in the disc title:
 *
 * 0[-n];<title>// - titled disc.
 * <title>// - titled disc
 * 0[-n];// - untitled disc
 * n{n>0};<group>// - untitled disc, group with one track
 * n{n>0}-n2{n2>n>0};group// - untitled disc, group with multiple tracks
 * ;group// - untitled disc, group with no tracks
 *
 */
int netmd_initialize_disc_info(netmd_dev_handle* devh, HndMdHdr* md)
{
    if (*md != NULL)
    {
        free_md_header(md);
        *md = NULL;
    }

    char *discHeader = NULL;

    request_disc_title_ex(devh, &discHeader);

    if (discHeader != NULL)
    {
        *md = create_md_header(discHeader);
        free(discHeader);
    }
    return strlen(md_header_to_string(*md));
}

void print_groups(HndMdHdr md)
{
    md_header_list_groups(md);
}

int netmd_create_group(netmd_dev_handle* dev, HndMdHdr md, char* name, int first, int last)
{
    if (md_header_add_group(md, name, first, last) > -1)
    {
        netmd_log(NETMD_LOG_VERBOSE, "New group %s (%d ... %d) added!\n", name, first, last);
        netmd_write_disc_header(dev, md);
    }
    else
    {
        netmd_log(NETMD_LOG_ERROR, "Error: Can't add new group %s (%d ... %d)!\n", name, first, last);
    }
    return 0;
}

int netmd_set_disc_title(netmd_dev_handle* dev, char* title, size_t title_length)
{
    unsigned char *request, *p;
    unsigned char write_req[] = {0x00, 0x18, 0x07, 0x02, 0x20, 0x18,
                                 0x01, 0x00, 0x00, 0x30, 0x00, 0x0a,
                                 0x00, 0x50, 0x00, 0x00};
    unsigned char hs1[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x01, 0x00};
    unsigned char hs2[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x00, 0x00};
    unsigned char hs3[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x03, 0x00};
    unsigned char hs4[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x00, 0x00};
    unsigned char reply[256];
    int result;
    int oldsize;

    /* the title update command wants to now how many bytes to replace */
    oldsize = request_disc_title(dev, (char *)reply, sizeof(reply));
    if(oldsize == -1)
        oldsize = 0; /* Reading failed -> no title at all, replace 0 bytes */

    request = malloc(21 + title_length);
    memset(request, 0, 21 + title_length);

    memcpy(request, write_req, 16);
    request[16] = title_length & 0xff;
    request[20] = oldsize & 0xff;

    p = request + 21;
    memcpy(p, title, title_length);

    /* send handshakes */
    netmd_exch_message(dev, hs1, sizeof(hs1), reply);
    netmd_exch_message(dev, hs2, sizeof(hs2), reply);
    netmd_exch_message(dev, hs3, sizeof(hs3), reply);
    result = netmd_exch_message(dev, request, 0x15 + title_length, reply);
    /* send handshake to write */
    netmd_exch_message(dev, hs4, sizeof(hs4), reply);
    return result;
}

/* move track, then manipulate title string */
int netmd_put_track_in_group(netmd_dev_handle* dev, HndMdHdr md, const uint16_t track, const unsigned int group)
{
    /* this might fail if track is ungrouped */
    md_header_del_track_from_group(md, group, track);

    if (md_header_add_track_to_group(md, group, track) == 0)
    {
        return netmd_write_disc_header(dev, md);
    }
    return -1;
}

int netmd_pull_track_from_group(netmd_dev_handle* dev, HndMdHdr md, const uint16_t track, const unsigned int group)
{
    /* this might fail if track is ungrouped */
    if (md_header_del_track_from_group(md, group, track) == 0)
    {
        return netmd_write_disc_header(dev, md);
    }
    return -1;
}

int netmd_delete_group(netmd_dev_handle* dev, HndMdHdr md, const unsigned int group)
{
    if (md_header_del_group(md, group) == 0)
    {
        return netmd_write_disc_header(dev, md);
    }
    return -1;
}

static int request_disc_header_size(netmd_dev_handle* devh)
{
    int ret = 0;
    char *buff = NULL;
    request_disc_title_ex(devh, &buff);

    if (buff != NULL)
    {
        ret = strlen(buff);
        free(buff);
    }

    return ret;
}

int netmd_write_disc_header(netmd_dev_handle* devh, HndMdHdr md)
{
    size_t header_size     = 0;
    size_t old_header_size = request_disc_header_size(devh);
    size_t request_size    = 0;
    
    /* new header */
    const char* header = md_header_to_string(md);
    unsigned char* request = 0;

    unsigned char hs[]  = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x01, 0x00};
    unsigned char hs2[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x00, 0x00};
    unsigned char hs3[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x03, 0x00};
    unsigned char hs4[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x00, 0x00};



    unsigned char write_req[] = {0x00, 0x18, 0x07, 0x02, 0x20, 0x18,
                                 0x01, 0x00, 0x00, 0x30, 0x00, 0x0a,
                                 0x00, 0x50, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00};
    unsigned char reply[255];
    int ret;
    printf("sending write disc header handshake");
    netmd_exch_message(devh, hs, 8, reply);
    netmd_exch_message(devh, hs2, 8, reply);
    netmd_exch_message(devh, hs3, 8, reply);

    printf("...OK\n");
    header_size = strlen(header);
    printf("Header size: %llu\n", header_size);

    request_size = header_size + sizeof(write_req);
    request = malloc(request_size);
    memset(request, 0, request_size);

    memcpy(request, write_req, sizeof(write_req));
    *(uint16_t*)&request[15] = netmd_htons(header_size);
    *(uint16_t*)&request[19] = netmd_htons(old_header_size);

    memcpy(request + sizeof(write_req), header, header_size);
    ret = netmd_exch_message(devh, request, request_size, reply);
    netmd_exch_message(devh, hs4, 8, reply);
    free(request);

    return ret;
}

/* Warning: for most purposes you will probably want to use the send
 * functions in secure.c instead. */
int netmd_write_track(netmd_dev_handle* devh, char* szFile)
{
    int ret = 0;
    int transferred = 0;
    int fd = open(szFile, O_RDONLY); /* File descriptor to omg file */
    unsigned char *data = malloc(4096); /* Buffer for reading the omg file */
    unsigned char *p = NULL; /* Pointer to index into data */
    uint16_t track_number='\0'; /* Will store the track number of the recorded song */

    /* Some unknown command being send before titling */
    unsigned char begintitle[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02,
                                  0x03, 0x00};

    /* Some unknown command being send after titling */
    unsigned char endrecord[] =  {0x00, 0x18, 0x08, 0x10, 0x18, 0x02,
                                  0x00, 0x00};

    /* Command to finish toc flashing */
    unsigned char fintoc[] = {0x00, 0x18, 0x00, 0x08, 0x00, 0x46,
                              0xf0, 0x03, 0x01, 0x03, 0x48, 0xff,
                              0x00, 0x10, 0x01, 0x00, 0x25, 0x8f,
                              0xbf, 0x09, 0xa2, 0x2f, 0x35, 0xa3,
                              0xdd};

    /* Record command */
    unsigned char movetoendstartrecord[] = {0x00, 0x18, 0x00, 0x08, 0x00, 0x46,
                                            0xf0, 0x03, 0x01, 0x03, 0x28, 0xff,
                                            0x00, 0x01, 0x00, 0x10, 0x01, 0xff,
                                            0xff, 0x00, 0x94, 0x02, 0x00, 0x00,
                                            0x00, 0x06, 0x00, 0x00, 0x04, 0x98};

    /* The expected response from the record command. */
    unsigned char movetoendresp[] = {0x0f, 0x18, 0x00, 0x08, 0x00, 0x46,
                                     0xf0, 0x03, 0x01, 0x03, 0x28, 0x00,
                                     0x00, 0x01, 0x00, 0x10, 0x01, 0x00,
                                     0x11, 0x00, 0x94, 0x02, 0x00, 0x00,
                                     0x43, 0x8c, 0x00, 0x32, 0xbc, 0x50};

    /* Header to be inserted at every 0x3F10 bytes */
    unsigned char header[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x3f, 0x00, 0xd4, 0x4b, 0xdc, 0xaa,
                              0xef, 0x68, 0x22, 0xe2};

    /*	char debug[]  =      {0x4c, 0x63, 0xa0, 0x20, 0x82, 0xae, 0xab, 0xa1}; */
    unsigned char size_request[4];
    size_t data_size_i; /* the size of the data part, later it will be used to point out the last byte in file */
    unsigned int size;
    unsigned char* buf=NULL; /* A buffer for recieving file info */
    libusb_device_handle *dev;

    dev = (libusb_device_handle *)devh;

    if(fd < 0)
        return fd;

    if(!data)
        return ENOMEM;


    /********** Get the size of file ***********/
    lseek(fd, 0x56, SEEK_SET); /* Read to data size */
    read(fd,data,4);
    data_size_i = (size_t)(data[3] + (data[2] << 8) + (data[1] << 16) + (data[0] << 24));

    fprintf(stderr,"Size of data: %lu\n", (unsigned long)data_size_i);
    size = (data_size_i/0x3f18)*8+data_size_i+8;           /* plus number of data headers */
    fprintf(stderr,"Size of data w/ headers: %d\n",size);


    /********** Fill in information in start record command and send ***********/
    /* not sure if size is 3 of 4 bytes in rec command... */
    movetoendstartrecord[27]=(size >> 16) & 255;
    movetoendstartrecord[28]=(size >> 8) & 255;
    movetoendstartrecord[29]=size & 255;

    buf = (unsigned char*)sendcommand(devh, movetoendstartrecord, 30, movetoendresp, 0x1e);
    track_number = buf[0x12] & 0xff;


    /********** Prepare to send data ***********/
    lseek(fd, 90, SEEK_SET);  /* seek to 8 bytes before leading 8 00's */
    data_size_i += 90;        /* data_size_i will now contain position of last byte to be send */

    netmd_wait_for_sync(devh);   /* Wait for 00 00 00 00 from unit.. */


    /********** Send data ***********/
    while(ret >= 0)
    {
        size_t file_pos=0;	/* Position in file */
        size_t bytes_to_send;    /* The number of bytes that needs to be send in this round */

        size_t __bytes_left;     /* How many bytes are left in the file */
        size_t __chunk_size;     /* How many bytes are left in the 0x1000 chunk to send */
        size_t __distance_to_header; /* How far till the next header insert */

        file_pos = (size_t)lseek(fd,0,SEEK_CUR); /* Gets the position in file, might be a nicer way of doing this */

        fprintf(stderr,"pos: %lu/%lu; remain data: %ld\n",
                (unsigned long)file_pos, (unsigned long)data_size_i,
                (signed long)(data_size_i - file_pos));
        if (file_pos >= data_size_i) {
            fprintf(stderr,"Done transferring.\n");
            free(data);
            break;
        }

        __bytes_left = data_size_i - file_pos;
        __chunk_size = 0x1000;
        if (__bytes_left < 0x1000) {
            __chunk_size = __bytes_left;
        }

        __distance_to_header = (file_pos-0x5a) % 0x3f10;
        if (__distance_to_header !=0) __distance_to_header = 0x3f10 - __distance_to_header;
        bytes_to_send = __chunk_size;

        fprintf(stderr,"Chunksize: %lu\n", (unsigned long)__chunk_size);
        fprintf(stderr,"distance_to_header: %lu\n", (unsigned long)__distance_to_header);
        fprintf(stderr,"Bytes left: %lu\n", (unsigned long)__bytes_left);

        if (__distance_to_header <= 0x1000) {   	     /* every 0x3f10 the header should be inserted, with an appropiate key.*/
            fprintf(stderr,"Inserting header\n");

            if (__chunk_size<0x1000) {                 /* if there is room for header then make room for it.. */
                __chunk_size += 0x10;              /* Expand __chunk_size */
                bytes_to_send = __chunk_size-0x08; /* Expand bytes_to_send */
            }

            read(fd,data, __distance_to_header ); /* Errors checking should be added for read function */
            __chunk_size -= __distance_to_header; /* Update chunk size */

            p = data+__distance_to_header;        /* Change p to point at the position header should be inserted */
            memcpy(p,header,16);
            if (__bytes_left-__distance_to_header-0x10 < 0x3f00) {
                __bytes_left -= (__distance_to_header + 0x10);
            }
            else {
                __bytes_left = 0x3f00;
            }

            fprintf (stderr, "bytes left in chunk: %lu\n", (unsigned long)__bytes_left);
            p[6] = (__bytes_left >> 8) & 0xff;      /* Inserts the higher 8 bytes of the length */
            p[7] = __bytes_left & 0xff;     /* Inserts the lower 8 bytes of the length */
            __chunk_size -= 0x10;          /* Update chunk size (for inserted header */

            p += 0x10;                     /* p should now point at the beginning of the next data segment */
            lseek(fd,8,SEEK_CUR);          /* Skip the key value in omg file.. Should probably be used for generating the header */
            read(fd,p, __chunk_size);      /* Error checking should be added for read function */

        } else {
            if(0 == read(fd, data, __chunk_size)) { /* read in next chunk */
                free(data);
                break;
            }
        }

        netmd_log(NETMD_LOG_DEBUG, "Sending %d bytes to md\n", bytes_to_send);
        netmd_log_hex(NETMD_LOG_DEBUG, data, bytes_to_send);
        ret = libusb_bulk_transfer(dev, 0x02, data, (int)bytes_to_send, &transferred, 5000);
    } /* End while */

    if (ret<0) {
        free(data);
        return ret;
    }


    /******** End transfer wait for unit ready ********/
    fprintf(stderr,"Waiting for Done:\n");
    do {
        libusb_control_transfer(dev, 0xc1, 0x01, 0, 0, size_request, 0x04, 5000);
    } while  (memcmp(size_request,"\0\0\0\0",4)==0);

    netmd_log(NETMD_LOG_DEBUG, "Recieving response: \n");
    netmd_log_hex(NETMD_LOG_DEBUG, size_request, 4);
    size = size_request[2];
    if (size<1) {
        fprintf(stderr, "Invalid size\n");
        return -1;
    }
    buf = malloc(size);
    libusb_control_transfer(dev, 0xc1, 0x81, 0, 0, buf, (int)size, 500);
    netmd_log_hex(NETMD_LOG_DEBUG, buf, size);
    free(buf);

    /******** Title the transfered song *******/
    buf = (unsigned char*)sendcommand(devh, begintitle, 8, NULL, 0);

    fprintf(stderr,"Renaming track %d to test\n",track_number);
    netmd_set_title(devh, track_number, "test");

    buf = (unsigned char*)sendcommand(devh, endrecord, 8, NULL, 0);


    /********* End TOC Edit **********/
    ret = libusb_control_transfer(dev, 0x41, 0x80, 0, 0, fintoc, 0x19, 800);

    fprintf(stderr,"Waiting for Done: \n");
    do {
        libusb_control_transfer(dev, 0xc1, 0x01, 0, 0, size_request, 0x04, 5000);
    } while  (memcmp(size_request,"\0\0\0\0",4)==0);

    return ret;
}

/* AV/C Disc Subunit Specification ERASE (0x40),
 * subfunction "specific_object" (0x01) */
int netmd_delete_track(netmd_dev_handle* dev, const uint16_t track)
{
    int ret = 0;
    unsigned char request[] = {0x00, 0x18, 0x40, 0xff, 0x01, 0x00,
                               0x20, 0x10, 0x01, 0x00, 0x00};
    unsigned char reply[255];
    unsigned char *buf;

    buf = request + 9;
    netmd_copy_word_to_buffer(&buf, track, 0);
    ret = netmd_exch_message(dev, request, 11, reply);

    return ret;
}

/* AV/C Disc Subunit Specification ERASE (0x40),
 * subfunction "complete" (0x00) */
int netmd_erase_disc(netmd_dev_handle* dev)
{
    int ret = 0;
    unsigned char request[] = {0x00, 0x18, 0x40, 0xff, 0x00, 0x00};
    unsigned char reply[255];

    ret = netmd_exch_message(dev, request, 6, reply);

    return ret;
}

/* AV/C Description Spefication OPEN DESCRIPTOR (0x08),
 * subfunction "open for write" (0x03) */
int netmd_cache_toc(netmd_dev_handle* dev)
{
    int ret = 0;
    unsigned char request[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x03, 0x00};
    unsigned char reply[255];

    ret = netmd_exch_message(dev, request, sizeof(request), reply);
    return ret;
}

/* AV/C Description Spefication OPEN DESCRIPTOR (0x08),
 * subfunction "close" (0x00) */
int netmd_sync_toc(netmd_dev_handle* dev)
{
    int ret = 0;
    unsigned char request[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x00, 0x00};
    unsigned char reply[255];

    ret = netmd_exch_message(dev, request, sizeof(request), reply);
    return ret;
}

/* Calls need for Sharp devices */
int netmd_acquire_dev(netmd_dev_handle* dev)
{
    unsigned char request[] = {0x00, 0xff, 0x01, 0x0c, 0xff, 0xff, 0xff, 0xff,
                               0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    unsigned char reply[255];

    netmd_exch_message(dev, request, sizeof(request), reply);
    if (reply[0] == NETMD_STATUS_ACCEPTED){
      return NETMD_NO_ERROR;
    } else {
      return NETMD_COMMAND_FAILED_UNKNOWN_ERROR;
    }
}

int netmd_release_dev(netmd_dev_handle* dev)
{
    int ret = 0;
    unsigned char request[] = {0x00, 0xff, 0x01, 0x00, 0xff, 0xff, 0xff, 0xff,
                               0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    unsigned char reply[255];

    ret = netmd_exch_message(dev, request, sizeof(request), reply);
    return ret;
}

/*! @brief      request disc title (raw header)

    @param      dev     The dev
    @param      buffer  The buffer
    @param[in]  size    The size

    @return     title size
 */
int netmd_request_raw_header(netmd_dev_handle* dev, char* buffer, size_t size)
{
    return request_disc_title(dev, buffer, size);
}

int netmd_request_raw_header_ex(netmd_dev_handle* dev, char** buffer)
{
    return request_disc_title_ex(dev, buffer);
}

//------------------------------------------------------------------------------
//! @brief      get track count from MD device
//!
//! @param[in]  dev     The dev handle
//! @param[out] tcount  The tcount buffer
//!
//! @return     0 -> ok; -1 -> error
//------------------------------------------------------------------------------
int netmd_request_track_count(netmd_dev_handle* dev, uint16_t* tcount)
{
    unsigned char req[] = {0x00, 0x18, 0x06, 0x02, 0x10, 0x10, 
                           0x01, 0x30, 0x00, 0x10, 0x00, 0xff, 
                           0x00, 0x00, 0x00, 0x00, 0x00};
    
    unsigned char reply[255];

    int ret = netmd_exch_message(dev, req, 17, reply);

    if (ret > 0)
    {
        // last byte contains the track count
        *tcount = reply[ret - 1];
        ret = 0;
    }
    else
    {
        ret = -1;
    }

    return ret;
}
