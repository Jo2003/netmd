/*
 * utils.c
 *
 * This file is part of libnetmd, a library for accessing Sony NetMD devices.
 *
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

#include <string.h>
#include <math.h>
#include "utils.h"
#include "log.h"

inline unsigned char proper_to_bcd_single(unsigned char value)
{
    unsigned char high, low;

    low = (value % 10) & 0xf;
    high = (((value / 10) % 10) * 0x10U) & 0xf0;

    return high | low;
}

inline unsigned char* proper_to_bcd(unsigned int value, unsigned char* target, size_t len)
{
    while (value > 0 && len > 0) {
        target[len - 1] = proper_to_bcd_single(value & 0xff);
        value /= 100;
        len--;
    }

    return target;
}

inline unsigned char bcd_to_proper_single(unsigned char value)
{
    unsigned char high, low;

    high = (value & 0xf0) >> 4;
    low = (value & 0xf);

    return ((high * 10U) + low) & 0xff;
}

inline unsigned int bcd_to_proper(unsigned char* value, size_t len)
{
    unsigned int result = 0;
    unsigned int nibble_value = 1;

    for (; len > 0; len--) {
        result += nibble_value * bcd_to_proper_single(value[len - 1]);

        nibble_value *= 100;
    }

    return result;
}

void netmd_check_response_bulk(netmd_response *response, const unsigned char* const expected,
                               const size_t expected_length, netmd_error *error)
{
    unsigned char *current;

    /* only check if there was no error before */
    if (*error == NETMD_NO_ERROR) {

        if ((response->length - response->position) < expected_length) {
            *error = NETMD_RESPONSE_TO_SHORT;
        }
        else {
            current = response->content + response->position;

            if (memcmp(current, expected, expected_length) == 0) {
                response->position += expected_length;
            }
            else {
                netmd_log_hex(0, current, expected_length);
                netmd_log_hex(0, expected, expected_length);
                *error = NETMD_RESPONSE_NOT_EXPECTED;
            }
        }
    }
}

void netmd_check_response_word(netmd_response *response, const uint16_t expected,
                               netmd_error *error)
{
    unsigned char buf[2];
    unsigned char *tmp = buf;

    /* only check if there was no error before */
    if (*error == NETMD_NO_ERROR) {
        if ((response->length - response->position) < 2) {
            *error = NETMD_RESPONSE_TO_SHORT;
        }
        else {
            netmd_copy_word_to_buffer(&tmp, expected, 0);
            netmd_check_response_bulk(response, buf, 2, error);
        }
    }
}

void netmd_check_response_doubleword(netmd_response *response, const uint32_t expected,
                                     netmd_error *error)
{
    unsigned char buf[4];
    unsigned char *tmp = buf;

    /* only check if there was no error before */
    if (*error == NETMD_NO_ERROR) {
        if ((response->length - response->position) < 4) {
            *error = NETMD_RESPONSE_TO_SHORT;
        }
        else {
            netmd_copy_doubleword_to_buffer(&tmp, expected, 0);
            netmd_check_response_bulk(response, buf, 4, error);
        }
    }
}


void netmd_check_response(netmd_response *response, const unsigned char expected,
                          netmd_error *error)
{
    /* only check if there was no error before */
    if (*error == NETMD_NO_ERROR) {

        if ((response->length - response->position) < 1) {
            *error = NETMD_RESPONSE_TO_SHORT;
        }
        else {
            if (response->content[response->position] == expected) {
                response->position++;
            }
            else {
                netmd_log_hex(0, response->content + response->position, 1);
                netmd_log_hex(0, &expected, 1);
                *error = NETMD_RESPONSE_NOT_EXPECTED;
            }
        }
    }
}

void netmd_read_response_bulk(netmd_response *response, unsigned char* target,
                              const size_t length, netmd_error *error)
{
    /* only copy if there was no error before */
    if (*error == NETMD_NO_ERROR) {

        if ((response->length - response->position) < length) {
            *error = NETMD_RESPONSE_TO_SHORT;
        }
        else {
            if (target) {
                memcpy(target, response->content + response->position, length);
            }

            response->position += length;
        }
    }
}

unsigned char *netmd_copy_word_to_buffer(unsigned char **buf, uint16_t value, int little_endian)
{
    if (little_endian == 0) {
        **buf = (unsigned char)((value >> 8) & 0xff);
        (*buf)++;
    }

    **buf = (unsigned char)((value >> 0) & 0xff);
    (*buf)++;

    if (little_endian == 1) {
        **buf = (unsigned char)((value >> 8) & 0xff);
        (*buf)++;
    }

    return *buf;
}

unsigned char *netmd_copy_doubleword_to_buffer(unsigned char **buf, uint32_t value, int little_endian)
{
    int8_t diff = 8;
    int bit = 24;
    int i;

    if (little_endian == 1) {
        diff = -8;
        bit = 0;
    }

    for (i = 0; i < 4; i++, bit = (bit - diff) & 0xff) {
        **buf = (unsigned char)(value >> bit) & 0xff;
        (*buf)++;
    }

    return *buf;
}

unsigned char *netmd_copy_quadword_to_buffer(unsigned char **buf, uint64_t value)
{
    **buf = (value >> 56) & 0xff;
    (*buf)++;

    **buf = (value >> 48) & 0xff;
    (*buf)++;

    **buf = (value >> 40) & 0xff;
    (*buf)++;

    **buf = (value >> 32) & 0xff;
    (*buf)++;

    **buf = (value >> 24) & 0xff;
    (*buf)++;

    **buf = (value >> 16) & 0xff;
    (*buf)++;

    **buf = (value >> 8) & 0xff;
    (*buf)++;

    **buf = (value >> 0) & 0xff;
    (*buf)++;

    return *buf;
}

/* TODO: add error */

unsigned char netmd_read(netmd_response *response)
{
    return response->content[response->position++];
}

uint16_t netmd_read_word(netmd_response *response)
{
    int i;
    uint16_t value;

    value = 0;
    for (i = 0; i < 2; i++) {
        value = (((unsigned int)value << 8U) | ((unsigned int)response->content[response->position] & 0xff)) & 0xffff;
        response->position++;
    }

    return value;
}

uint32_t netmd_read_doubleword(netmd_response *response)
{
    int i;
    uint32_t value;

    value = 0;
    for (i = 0; i < 4; i++) {
        value <<= 8;
        value += (response->content[response->position] & 0xff);
        response->position++;
    }

    return value;
}

uint64_t netmd_read_quadword(netmd_response *response)
{
    int i;
    uint64_t value;

    value = 0;
    for (i = 0; i < 8; i++) {
        value <<= 8;
        value += (response->content[response->position] & 0xff);
        response->position++;
    }

    return value;
}

static int bigEndian()
{
    unsigned char buf[2];
    uint16_t *pU16 = (uint16_t*)buf;

    *pU16 = 0x8811;

    return (buf[0] == 0x88) ? 1 : 0;
}

static uint16_t swop16(uint16_t in)
{
    return ((in & 0xff) << 8) | ((in >> 8) & 0xff);
}

static uint32_t swop32(uint32_t in)
{
    return (( in        & 0xff) << 24) | (((in >>  8) & 0xff) << 16)
         | (((in >> 16) & 0xff) <<  8) | (( in >> 24) & 0xff       );
}

static uint64_t swop64(uint64_t in)
{
    return (( in           & 0xff) << 56ull) | (((in >>  8ull) & 0xff) << 48ull)
         | (((in >> 16ull) & 0xff) << 40ull) | (((in >> 24ull) & 0xff) << 32ull)
         | (((in >> 32ull) & 0xff) << 24ull) | (((in >> 40ull) & 0xff) << 16ull)
         | (((in >> 48ull) & 0xff) <<  8ull) | (( in >> 56ull) & 0xff          );
}


//------------------------------------------------------------------------------
//! @brief      htons short and dirty
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint16_t netmd_htons(uint16_t in)
{
    if (bigEndian() == 0)
    {
        // little endian -> swop bytes
        return swop16(in);
    }
    return in;
}

//------------------------------------------------------------------------------
//! @brief      htonl short and dirty
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint32_t netmd_htonl(uint32_t in)
{
    if (bigEndian() == 0)
    {
        // little endian -> swop bytes
        return swop32(in);
    }
    return in;
}

//------------------------------------------------------------------------------
//! @brief      ntohs short and dirty
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint16_t netmd_ntohs(uint16_t in)
{
    // same same and not different ...
    return netmd_htons(in);
}

//------------------------------------------------------------------------------
//! @brief      ntohl short and dirty
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint32_t netmd_ntohl(uint32_t in)
{
    // same same and not different ...
    return netmd_htonl(in);
}

//------------------------------------------------------------------------------
//! @brief      host to big endian long long
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint64_t netmd_htonll(uint64_t in)
{
    if (bigEndian() == 0)
    {
        // little endian -> swop bytes
        return swop64(in);
    }
    return in;
}

//------------------------------------------------------------------------------
//! @brief      host to little endian short
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint16_t netmd_htoles(uint16_t in)
{
    if (bigEndian())
    {
        // little endian -> swop bytes
        return swop16(in);
    }
    return in;
}

//------------------------------------------------------------------------------
//! @brief      host to little endian long
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint32_t netmd_htolel(uint32_t in)
{
    if (bigEndian())
    {
        return swop32(in);
    }
    return in;
}

//------------------------------------------------------------------------------
//! @brief      host to little endian long long
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint64_t netmd_htolell(uint64_t in)
{
    if (bigEndian())
    {
        return swop64(in);
    }
    return in;
}

//------------------------------------------------------------------------------
//! @brief      little to host endian short
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint16_t netmd_letohs(uint16_t in)
{
    return netmd_htoles(in);
}

//------------------------------------------------------------------------------
//! @brief      little endian to host long
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint32_t netmd_letohl(uint32_t in)
{
    return netmd_htolel(in);
}

//------------------------------------------------------------------------------
//! @brief      little endian to host long long
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint64_t netmd_letohll(uint64_t in)
{
    return netmd_htolell(in);
}

//------------------------------------------------------------------------------
//! @brief     calculate NetMD checksum
//!
//! @param[in] data pointer to data
//! @param[in] size data size
//!
//! @return    checksum
//------------------------------------------------------------------------------
unsigned int netmd_calculate_checksum(unsigned char data[], size_t size)
{
    unsigned int crc  = 0;
    unsigned int temp = size;

    for (size_t i = 0; i < size; i++)
    {
        temp = (temp & 0xffff0000) | data[i];
        crc ^= temp;

        for (int j = 0; j < 16; j++)
        {
            unsigned int ts = crc & 0x8000;
            crc <<= 1;
            if (ts)
            {
                crc ^= 0x1021;
            }
        }
    }

    return (crc & 0xffff);
}

//------------------------------------------------------------------------------
//! @brief      format a netmd device query
//!
//! @param[in]  format    format string
//! @param[in]  argv      arguments array
//! @param[in]  argc      argument count
//! @param[in]  query_sz  buffer for query size
//!
//! @return     formatted byte array (you MUST call free() if not NULL);
//!             NULL on error
//------------------------------------------------------------------------------
uint8_t* netmd_format_query(const char* format, const netmd_query_data_t argv[], int argc, size_t* query_sz)
{
#define mBUFFSZ 2048
#define mSZ_CHECK                                                                                        \
    do                                                                                                   \
    {                                                                                                    \
        if ((argv[argno].size + byteIdx) > mBUFFSZ)                                                      \
        {                                                                                                \
            netmd_log(NETMD_LOG_ERROR, "Error: Data size exceeds prepared memory in %s!", __FUNCTION__); \
            return ret;                                                                                  \
        }                                                                                                \
    } while(0)

    uint8_t* ret = NULL;
    uint8_t out[mBUFFSZ];
    uint8_t b;
    size_t byteIdx     = 0;
    char   tok[3]     = {'\0',};
    size_t tokIdx     = 0;
    int    argno      = 0;
    int    esc        = 0;
    netmd_endianess_t endian = netmd_no_convert;
    *query_sz         = 0;

    // remove spaces
    while (*format != '\0')
    {
        // add some kind of sanity check
        if ((argno > argc) || (byteIdx > (mBUFFSZ - 1)))
        {
            netmd_log(NETMD_LOG_ERROR, "Error sanity check in %s!", __FUNCTION__);
            return ret;
        }

        if (!esc)
        {
            switch(*format)
            {
            case '\t':
            case ' ':
                // ignore
                break;
            case '%':
                esc = 1;
                break;
            default:
                tok[tokIdx++] = *format;
                if (tokIdx == 2)
                {
                    char *end;
                    b = strtoul(tok, &end, 16);

                    if (end != tok)
                    {
                        out[byteIdx++] = b;
                    }
                    else
                    {
                        // can't convert char* to number
                        netmd_log(NETMD_LOG_ERROR, "Can't convert token '%s' into hex number in %s!", tok, __FUNCTION__);
                        return ret;
                    }

                    tokIdx = 0;
                }
                break;
            }
        }
        else
        {
            int c;
            switch((c = tolower(*format)))
            {
            case netmd_fmt_byte:
                mSZ_CHECK;
                out[byteIdx++] = argv[argno++].data.u8;
                esc    = 0;
                endian = netmd_no_convert;
                break;

            case netmd_fmt_word:
                mSZ_CHECK;
                if (endian == netmd_hto_littleendian)
                {
                    *(uint16_t*)&out[byteIdx] = netmd_htoles(argv[argno].data.u16);
                }
                else if (endian == netmd_hto_bigendian)
                {
                    *(uint16_t*)&out[byteIdx] = netmd_htons(argv[argno].data.u16);
                }
                else
                {
                    *(uint16_t*)&out[byteIdx] = argv[tokIdx].data.u16;
                }
                byteIdx += argv[argno++].size;
                esc      = 0;
                endian   = netmd_no_convert;
                break;

            case netmd_fmt_dword:
                mSZ_CHECK;
                if (endian == netmd_hto_littleendian)
                {
                    *(uint32_t*)&out[byteIdx] = netmd_htolel(argv[argno].data.u32);
                }
                else if (endian == netmd_hto_bigendian)
                {
                    *(uint32_t*)&out[byteIdx] = netmd_htonl(argv[argno].data.u32);
                }
                else
                {
                    *(uint32_t*)&out[byteIdx] = argv[argno].data.u32;
                }
                byteIdx += argv[argno++].size;
                esc      = 0;
                endian   = netmd_no_convert;
                break;

            case netmd_fmt_qword:
                mSZ_CHECK;
                if (endian == netmd_hto_littleendian)
                {
                    *(uint64_t*)&out[byteIdx] = netmd_htolell(argv[argno].data.u64);
                }
                else if (endian == netmd_hto_bigendian)
                {
                    *(uint64_t*)&out[byteIdx] = netmd_htonll(argv[argno].data.u64);
                }
                else
                {
                    *(uint64_t*)&out[byteIdx] = argv[argno].data.u64;
                }
                byteIdx += argv[argno++].size;
                esc      = 0;
                endian   = netmd_no_convert;
                break;

            case netmd_fmt_barray:
                mSZ_CHECK;
                memcpy(&out[byteIdx], argv[argno].data.pu8, argv[argno].size);
                byteIdx += argv[argno++].size;
                esc      = 0;
                endian   = netmd_no_convert;
                break;

            case netmd_hto_littleendian:
            case netmd_hto_bigendian:
                endian = c;
                break;

            default:
                netmd_log(NETMD_LOG_ERROR, "Unsupported format option '%c' used in %s!", c, __FUNCTION__);
                return ret;
                break;
            }
        }
        format++;
    }

    if (byteIdx)
    {
        *query_sz = byteIdx;
        if ((ret = malloc(byteIdx)) != NULL)
        {
            memcpy(ret, out, byteIdx);
            netmd_log(NETMD_LOG_DEBUG, "Created query: ");
            netmd_log_hex(NETMD_LOG_DEBUG, ret, byteIdx);
        }
    }

    return ret;
#undef mSZ_CHECK
#undef mBUFFSZ
}

//------------------------------------------------------------------------------
//! @brief      scan data for format options
//!
//! @param[in]  data      byte array to scan
//! @param[in]  size      data size
//! @param[in]  format    format string
//! @param[out] argv      buffer pointer for argument array
//!                       (call free() if not NULL!)
//!                       Note: If stored data is an byte array,
//!                       you have to free it using free() as well!
//! @param[out] argc      pointer to argument count
//!
//! @return     0 -> ok; -1 -> error
//------------------------------------------------------------------------------
int netmd_scan_query(const uint8_t data[], size_t size, const char* format, netmd_capture_data_t** argv, int* argc)
{
#define mBUFFSZ 10
    netmd_capture_data_t dataBuffer[mBUFFSZ] = {{netmd_fmt_barray, {.u8 = 0}, 0},};
    int     ret                 = -1;
    int     esc                 =  0;
    char    tok[3]              = {'\0',};
    size_t  tokIdx              =  0;
    size_t  dataIdx             = 0;
    uint8_t cmp                 = 0;
    netmd_capture_data_t* pArgv = dataBuffer;

    netmd_log(NETMD_LOG_DEBUG, "Scan reply: ");
    netmd_log_hex(NETMD_LOG_DEBUG, data, size);

    // remove spaces
    while (*format != '\0')
    {
        if ((*argc > mBUFFSZ) || (dataIdx >= size))
        {
            netmd_log(NETMD_LOG_ERROR, "Error sanity check in %s!", __FUNCTION__);

            for (int i = 0; i < (*argc - 1); i++)
            {
                if ((dataBuffer[i].tp == netmd_fmt_barray) && (dataBuffer[i].data.pu8 != NULL))
                {
                    free(dataBuffer[i].data.pu8);
                }
            }
            return ret;
        }

        if (!esc)
        {
            switch(*format)
            {
            case '\t':
            case ' ':
                // ignore
                break;
            case '%':
                esc = 1;
                break;
            default:
                tok[tokIdx++] = *format;
                if (tokIdx == 2)
                {
                    char *end;
                    cmp = strtoul(tok, &end, 16);

                    if (end != tok)
                    {
                        if (cmp != data[dataIdx++]) return ret;
                    }
                    else
                    {
                        // can't convert char* to number
                        netmd_log(NETMD_LOG_ERROR, "Can't convert token '%s' into hex number in %s!", tok, __FUNCTION__);
                        return ret;
                    }

                    tokIdx = 0;
                }
                break;
            }
        }
        else
        {
            int c;
            switch((c = tolower(*format)))
            {
            case '?':
                esc = 0;
                dataIdx ++;
                break;

            case netmd_fmt_byte:
                // capture byte
                pArgv->tp      = c;
                pArgv->size    = 1;
                pArgv->data.u8 = data[dataIdx++];
                pArgv++;
                (*argc)++;
                esc = 0;
                break;

            case netmd_fmt_word:
                // capture word
                pArgv->tp       = c;
                pArgv->size     = 2;
                pArgv->data.u16 = netmd_letohs(*(uint16_t*)&data[dataIdx]);

                (*argc)++;
                dataIdx += pArgv->size;
                pArgv++;
                esc = 0;
                break;

            case netmd_fmt_dword:
                // capture dword
                pArgv->tp       = c;
                pArgv->size     = 4;
                pArgv->data.u32 = netmd_letohl(*(uint32_t*)&data[dataIdx]);

                (*argc)++;
                dataIdx += pArgv->size;
                pArgv++;
                esc = 0;
                break;

            case netmd_fmt_qword:
                // capture qword
                pArgv->tp       = c;
                pArgv->size     = 8;
                pArgv->data.u64 = netmd_letohll(*(uint64_t*)&data[dataIdx]);

                (*argc)++;
                dataIdx += pArgv->size;
                pArgv++;
                esc = 0;
                break;

            case netmd_fmt_barray:
                pArgv->tp   = c;
                pArgv->size = size - dataIdx;
                if ((pArgv->data.pu8 = malloc(pArgv->size)) != NULL)
                {
                    memcpy(pArgv->data.pu8, &data[dataIdx], pArgv->size);
                }
                else
                {
                    netmd_log(NETMD_LOG_ERROR, "Error memory allocation error in %s!", __FUNCTION__);

                    for (int i = 0; i < *argc; i++)
                    {
                        if ((dataBuffer[i].tp == netmd_fmt_barray) && (dataBuffer[i].data.pu8 != NULL))
                        {
                            free(dataBuffer[i].data.pu8);
                        }
                    }
                    return ret;
                }

                (*argc)++;
                dataIdx += pArgv->size;
                pArgv++;
                esc = 0;
                break;

            case netmd_hto_littleendian:
            case netmd_hto_bigendian:
                // endian = c;
                // ignore
                break;

            default:
                netmd_log(NETMD_LOG_ERROR, "Unsupported format option '%c' used in %s!", c, __FUNCTION__);
                return ret;
                break;
            }
        }

        format ++;
    }

    ret = 0;

    if ((*argc) > 0)
    {
        /**
         *  \note We do not copy the byte array data, but the pointer to them.
         *        Memory was allocated above and has to be freed afterwards.
         */
        size_t cpsz = (*argc) * sizeof(netmd_capture_data_t);

        if ((*argv = malloc(cpsz)) != NULL)
        {
            memcpy(*argv, dataBuffer, cpsz);
        }
        else
        {
            netmd_log(NETMD_LOG_ERROR, "Error memory allocation error in %s!", __FUNCTION__);

            for (int i = 0; i < *argc; i++)
            {
                if ((dataBuffer[i].tp == netmd_fmt_barray) && (dataBuffer[i].data.pu8 != NULL))
                {
                    free(dataBuffer[i].data.pu8);
                }
            }
            ret = -1;
        }
    }

    return ret;

#undef mBUFFSZ
}

//------------------------------------------------------------------------------
//! @brief      prepare AUDIO for SP upload
//!
//! @param[in/out]  audio_data audio data (on out: must be freed afterwards)
//! @param[in/out]  data_size  size of audio data
//!
//! @return     netmd_error
//! @see        netmd_error
//------------------------------------------------------------------------------
netmd_error netmd_prepare_audio_sp_upload(uint8_t** audio_data, size_t* data_size)
{
#define PAD_SZ 100
    const uint8_t padding[PAD_SZ] = {0,};
    size_t   in_sz   = *data_size;
    uint8_t* in_data = *audio_data;
    size_t   out_idx = 0, in_idx = 0;
    uint8_t* out_data;
    uint8_t* sector;
    size_t sector_sz;

    // mind the header of 2048 bytes
    in_sz   -= 2048;
    in_data += 2048;

    // get final memory size incl. padding
    double temp_sz = in_sz;
    size_t new_sz = ceil(temp_sz / 2332) * 100 + temp_sz;
    if((out_data = malloc(new_sz)) != NULL)
    {
        for (in_idx = 0; in_idx < in_sz; in_idx += 2332)
        {
            // sector size might be less than 2332 bytes
            sector_sz = ((in_sz - in_idx) >= 2332) ? 2332 : in_sz - in_idx;
            sector = &out_data[out_idx];
            memcpy(sector, &in_data[in_idx], sector_sz);
            out_idx += sector_sz;

            // Rewrite Block Size Mode and the number of Block Floating Units
            // This mitigates an issue with atracdenc where it doesn't write
            // the bytes at the end of each frame.
            for(size_t j = 0; j < sector_sz; j += 212)
            {
                sector[j + 212 - 1] = sector[j + 0];
                sector[j + 212 - 2] = sector[j + 1];
            }

            memcpy(&out_data[out_idx], padding, PAD_SZ);
            out_idx += PAD_SZ;
        }

        free(*audio_data);
        *data_size  = out_idx;
        *audio_data = out_data;

        return NETMD_NO_ERROR;
    }

    return NETMD_ERROR;

#undef PAD_SZ
}
