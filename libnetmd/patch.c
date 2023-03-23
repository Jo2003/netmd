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
#include <stdio.h>
#include <assert.h>
#include "patch.h"
#include "utils.h"
#include "log.h"
#include "libnetmd_intern.h"

// defines
#define PERIPHERAL_BASE 0x03802000ul
#define MAX_PATCH 8

// types

//! @brief supported firmware on Sony devices
typedef enum
{
    SDI_S1200   = (1ul <<  0),    //!< S1.200 version
    SDI_S1300   = (1ul <<  1),    //!< S1.300 version
    SDI_S1400   = (1ul <<  2),    //!< S1.400 version
    SDI_S1500   = (1ul <<  3),    //!< S1.500 version
    SDI_S1600   = (1ul <<  4),    //!< S1.600 version
    SDI_UNKNOWN = (1ul << 31),    //!< unsupported or unknown
} sony_dev_info_t;

//! @brief patch id
typedef enum
{
    PID_UNUSED,
    PID_DEVTYPE,
    PID_PATCH_0_A,
    PID_PATCH_0_B,
    PID_PATCH_0,
    PID_PREP_PATCH,
    PID_PATCH_CMN_1,
    PID_PATCH_CMN_2,
    PID_TRACK_TYPE,
    PID_SAFETY,
} patch_id_t;

//! @brief memory device open types
typedef enum
{
    NETMD_MEM_CLOSE      = 0x0,
    NETMD_MEM_READ       = 0x1,
    NETMD_MEM_WRITE      = 0x2,
    NETMD_MEM_READ_WRITE = 0x3,
} netmd_memory_open_t;

//! @brief patch address for one device
typedef struct
{
    sony_dev_info_t devinfo;
    uint32_t addr;
} patch_addr_t;

//! @brief patch address table entry (used in array / table)
typedef struct
{
    patch_id_t pid;
    int addr_count;
    patch_addr_t addrs[5];
} patch_addr_entry_t;

//! @brief patch payload table entry (used in array / table)
typedef struct
{
    patch_id_t pid;
    uint32_t devices;
    uint8_t payload[4];
} patch_payload_entry_t;

//!< @brief structure to hold all information of a patch
typedef struct
{
    uint32_t addr;
    uint8_t data[4];
} patch_data_t;

// static values

//! @brief patch address table
static patch_addr_entry_t patch_addr_tab[] = {
    {PID_DEVTYPE    , 4, {{SDI_S1600, 0x02003fcf},{SDI_S1500, 0x02003fc7},{SDI_S1400, 0x03000220},{SDI_S1300, 0x02003e97},{SDI_S1200, 0x00      }}},
    {PID_PATCH_0_A  , 4, {{SDI_S1600, 0x0007f408},{SDI_S1500, 0x0007e988},{SDI_S1400, 0x0007e2c8},{SDI_S1300, 0x0007aa00},{SDI_S1200, 0x00      }}},
    {PID_PATCH_0_B  , 5, {{SDI_S1600, 0x0007efec},{SDI_S1500, 0x0007e56c},{SDI_S1400, 0x0007deac},{SDI_S1300, 0x0007a5e4},{SDI_S1200, 0x00078dcc}}},
    {PID_PREP_PATCH , 5, {{SDI_S1600, 0x00077c04},{SDI_S1500, 0x0007720c},{SDI_S1400, 0x00076b38},{SDI_S1300, 0x00073488},{SDI_S1200, 0x00071e5c}}},
    {PID_PATCH_CMN_1, 5, {{SDI_S1600, 0x0007f4e8},{SDI_S1500, 0x0007ea68},{SDI_S1400, 0x0007e3a8},{SDI_S1300, 0x0007aae0},{SDI_S1200, 0x00078eac}}},
    {PID_PATCH_CMN_2, 5, {{SDI_S1600, 0x0007f4ec},{SDI_S1500, 0x0007ea6c},{SDI_S1400, 0x0007e3ac},{SDI_S1300, 0x0007aae4},{SDI_S1200, 0x00078eb0}}},
    {PID_TRACK_TYPE , 5, {{SDI_S1600, 0x000852b0},{SDI_S1500, 0x00084820},{SDI_S1400, 0x00084160},{SDI_S1300, 0x00080798},{SDI_S1200, 0x0007ea9c}}},
    {PID_SAFETY     , 4, {{SDI_S1600, 0x000000c4},{SDI_S1500, 0x000000c4},{SDI_S1400, 0x000000c4},{SDI_S1300, 0x000000c4},{SDI_S1200, 0x00      }}}, //< anti brick patch
};
//! @brief patch address table size
static const size_t patch_addr_tab_size = sizeof(patch_addr_tab) / sizeof(patch_addr_tab[0]);

//! @brief patch payload table
static patch_payload_entry_t patch_payload_tab[] = {
    {PID_PATCH_0    , SDI_S1200 | SDI_S1300 | SDI_S1400 | SDI_S1500 | SDI_S1600, {0x00,0x00,0xa0,0xe1}},
    {PID_PREP_PATCH , SDI_S1200 | SDI_S1300 | SDI_S1400 | SDI_S1500 | SDI_S1600, {0x0D,0x31,0x01,0x60}},
    {PID_PATCH_CMN_1, SDI_S1200 | SDI_S1300 | SDI_S1400 | SDI_S1500 | SDI_S1600, {0x14,0x80,0x80,0x03}},
    {PID_PATCH_CMN_2, SDI_S1200 | SDI_S1300 | SDI_S1400 | SDI_S1500 | SDI_S1600, {0x14,0x90,0x80,0x03}},
    {PID_TRACK_TYPE , SDI_S1200 | SDI_S1300 | SDI_S1400 | SDI_S1500 | SDI_S1600, {0x06,0x02,0x00,0x04}},
    {PID_SAFETY     ,                         SDI_S1400 | SDI_S1500 | SDI_S1600, {0xdc,0xff,0xff,0xea}}, //< anti brick patch
};
//! @brief patch payload table size
static const size_t patch_payload_tab_size = sizeof(patch_payload_tab) / sizeof(patch_payload_tab[0]);

//! @brief patch areas used
static patch_id_t used_patches[MAX_PATCH] = {PID_UNUSED,};

//! @brief used for exchang
static uint8_t _s_buff[255];

// internal functions

//------------------------------------------------------------------------------
//! @brief      get next free patch area
//!
//! @return     -1 -> no more free | > -1 -> free patch index
//------------------------------------------------------------------------------
static int get_next_free_patch(patch_id_t pid)
{
    for (int i = 0; i < MAX_PATCH; i++)
    {
        if (used_patches[i] == PID_UNUSED)
        {
            used_patches[i] = pid;
            return i;
        }
    }
    return -1;
}

//------------------------------------------------------------------------------
//! @brief      get patch address by name and device info
//!
//! @param[in]  devinfo    device info
//! @param[in]  pid        patch id
//!
//! @return     0 -> error | > 0 -> address
//------------------------------------------------------------------------------
static uint32_t get_patch_address(sony_dev_info_t devinfo, patch_id_t pid)
{
    for(size_t i = 0; i < patch_addr_tab_size; i++)
    {
        if (patch_addr_tab[i].pid == pid)
        {
            for (int j = 0; j < patch_addr_tab[i].addr_count; j++)
            {
                if (patch_addr_tab[i].addrs[j].devinfo == devinfo)
                {
                    return patch_addr_tab[i].addrs[j].addr;
                }
            }
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
//! @brief      get patch payload by name and device info
//!
//! @param[in]  devinfo    device info
//! @param[in]  pid        patch id
//!
//! @return     NULL -> error; else -> patch content
//------------------------------------------------------------------------------
static uint8_t* get_patch_payload(sony_dev_info_t devinfo, patch_id_t pid)
{
    for(size_t i = 0; i < patch_payload_tab_size; i++)
    {
        if (patch_payload_tab[i].pid == pid)
        {
            if (devinfo & patch_payload_tab[i].devices)
            {
                return patch_payload_tab[i].payload;
            }
        }
    }
    return NULL;
}

//------------------------------------------------------------------------------
//! @brief      write patch data
//!
//! @param[in]  devh      device handle
//! @param[in]  addr      address
//! @param[in]  data      data to write
//! @param[in]  data_size size of data
//!
//! @return     netmd_error
//! @see        netmd_error
//------------------------------------------------------------------------------
static netmd_error patch_write(netmd_dev_handle *devh, uint32_t addr, uint8_t data[], size_t data_size)
{
    netmd_error ret = NETMD_ERROR;
    size_t query_sz = 0;
    netmd_query_data_t argv[] = {
        {{.u32 = addr                                     }, sizeof(uint32_t)},
        {{.u8  = data_size                                }, sizeof(uint8_t) },
        {{.pu8 = data                                     }, data_size       },
        {{.u16 = netmd_calculate_checksum(data, data_size)}, sizeof(uint16_t)},
    };

    int argc = sizeof(argv) / sizeof(argv[0]);

    uint8_t* query = netmd_format_query("00 1822 ff 00 %<d %b 0000 %* %<w", argv, argc, &query_sz);
    if (query != NULL)
    {
        // send ...
        ret = netmd_exch_message(devh, query, query_sz, _s_buff);

        // free memory
        free(query);
    }

    return ret;
}

//------------------------------------------------------------------------------
//! @brief      read patch data
//!
//! @param[in]  devh       device handle
//! @param[in]  addr       address
//! @param[in]  data_size  size of data to read
//! @param[out] reply_size buffer for reply size
//!
//! @return     NULL -> error; else -> reply
//------------------------------------------------------------------------------
static uint8_t* patch_read(netmd_dev_handle *devh, uint32_t addr, size_t data_size, size_t* reply_size)
{
    uint8_t *reply  = NULL;
    int reply_sz    = 0;
    size_t query_sz = 0;
    netmd_capture_data_t* cap_argv = NULL;
    int                   cap_argc = 0;

    netmd_query_data_t argv[] = {
        {{.u32 = addr     }, sizeof(uint32_t)},
        {{.u8  = data_size}, sizeof(uint8_t) },
    };

    int argc = sizeof(argv) / sizeof(argv[0]);

    uint8_t* query = netmd_format_query("00 1821 ff 00 %<d %b", argv, argc, &query_sz);

    if (query != NULL)
    {
        // send ...
        reply_sz = netmd_exch_message_ex(devh, query, query_sz, &reply);

        // free memory
        free(query);
    }

    if (reply_sz > 0)
    {
        if (reply != NULL)
        {
            netmd_scan_query(reply, reply_sz, "%? 1821 00 %? %?%?%?%? %? %?%? %*", &cap_argv, &cap_argc);
            free(reply);
        }
    }

    reply = NULL;

    if (cap_argc > 0)
    {
        if (cap_argv != NULL)
        {
            if (cap_argv[0].tp == netmd_fmt_barray)
            {
                // don't mind the checksum
                *reply_size = cap_argv[0].size - 2;
                if ((reply = malloc(*reply_size)) != NULL)
                {
                    memcpy(reply, cap_argv[0].data.pu8, *reply_size);
                }
                else
                {
                    *reply_size = 0;
                }
                free(cap_argv[0].data.pu8);
            }
            free(cap_argv);
        }
    }

    return reply;
}

//------------------------------------------------------------------------------
//! @brief      open / close device memory
//!
//! @param[in]  devh  device handle
//! @param[in]  addr  address
//! @param[in]  sz    size of memory to change state
//! @param[in]  state open state
//!
//! @return     netmd_error
//! @see        netmd_error
//------------------------------------------------------------------------------
static netmd_error netmd_change_memory_state(netmd_dev_handle *devh, uint32_t addr, size_t sz, netmd_memory_open_t state)
{
    netmd_error ret = NETMD_ERROR;
    size_t query_sz = 0;
    netmd_query_data_t argv[] = {
        {{.u32 = addr }, sizeof(uint32_t)},
        {{.u8  = sz   }, sizeof(uint8_t) },
        {{.u8  = state}, sizeof(uint8_t) },
    };

    int argc = sizeof(argv) / sizeof(argv[0]);

    uint8_t* query = netmd_format_query("00 1820 ff 00 %<d %b %b 00", argv, argc, &query_sz);
    if (query != NULL)
    {
        // send ...
        ret = netmd_exch_message(devh, query, query_sz, _s_buff);

        // free memory
        free(query);
    }

    return ret;
}

//------------------------------------------------------------------------------
//! @brief      open for read, read, close
//!
//! @param[in]  devh     device handle
//! @param[in]  addr     address
//! @param[in]  sz       size of data to read
//! @param[out] reply_sz size of data read
//!
//! @return     NULL -> error; else -> reply (must be freed afterwards)
//------------------------------------------------------------------------------
static uint8_t* netmd_clean_read(netmd_dev_handle *devh, uint32_t addr, size_t sz, size_t* reply_sz)
{
    uint8_t* reply = NULL;
    netmd_change_memory_state(devh, addr, sz, NETMD_MEM_READ);
    reply = patch_read(devh, addr, sz, reply_sz);
    netmd_change_memory_state(devh, addr, sz, NETMD_MEM_CLOSE);
    return reply;
}

//------------------------------------------------------------------------------
//! @brief      open for write, write, close
//!
//! @param[in]  devh      device handle
//! @param[in]  addr      address
//! @param[in]  data      data to write
//! @param[in]  data_size size of data
//!
//! @return     netmd_error
//! @see        netmd_error
//------------------------------------------------------------------------------
static netmd_error netmd_clean_write(netmd_dev_handle *devh, uint32_t addr, uint8_t data[], size_t data_size)
{
    netmd_error ret = NETMD_ERROR;
    netmd_change_memory_state(devh, addr, data_size, NETMD_MEM_WRITE);
    ret = patch_write(devh, addr, data, data_size);
    netmd_change_memory_state(devh, addr, data_size, NETMD_MEM_CLOSE);
    return ret;
}

//------------------------------------------------------------------------------
//! @brief      get device code / type
//!
//! @param[in]  devh      device handle
//!
//! @return     sony_dev_info_t
//! @see        sony_dev_info_t
//------------------------------------------------------------------------------
static sony_dev_info_t netmd_get_device_code_ex(netmd_dev_handle *devh)
{
    sony_dev_info_t ret = SDI_UNKNOWN;
    char code[32]       = {'\0',};
    uint8_t query[]     = {0x00, 0x18, 0x12, 0xff};
    int idx             = 0;
    uint8_t chip        = 255, hwid = 255, version = 255;

    memset(_s_buff, 0xff, 255);
    netmd_exch_message(devh, query, sizeof(query), _s_buff);

    chip    = _s_buff[4];
    hwid    = _s_buff[5];
    version = _s_buff[7];

    if ((chip != 255) || (hwid != 255) || (version != 255))
    {
        switch (chip)
        {
        case 0x20:
            code[idx++] = 'R';
            break;
        case 0x21:
            code[idx++] = 'S';
            break;
        case 0x24:
            code[idx++] = 'H';
            code[idx++] = 'i';
            break;
        default:
            idx = snprintf(code, 32, "0x%.02X", (int)chip);
            break;
        }

        snprintf(&code[idx], 32 - idx, "%d.%d00", (int)(version >> 4), (int)(version & 0x0f));
        netmd_log(NETMD_LOG_VERBOSE, "Found device info: '%s'!\n", code);

        if (!strncmp(code, "S1.600", 6))
        {
            ret = SDI_S1600;
        }
        else if (!strncmp(code, "S1.200", 6))
        {
            ret = SDI_S1200;
        }
        else if (!strncmp(code, "S1.300", 6))
        {
            ret = SDI_S1300;
        }
        else if (!strncmp(code, "S1.400", 6))
        {
            ret = SDI_S1400;
        }
        else if (!strncmp(code, "S1.500", 6))
        {
            ret = SDI_S1500;
        }
    }

    return ret;
}

//------------------------------------------------------------------------------
//! @brief      read patch data
//!
//! @param[in]  devh         device handle
//! @param[in]  patch_number number of patch
//! @param[out] patch buffer for patch data
//!
//! @return     netmd_error
//! @see        netmd_error
//------------------------------------------------------------------------------
static netmd_error netmd_read_patch(netmd_dev_handle *devh, int patch_number, patch_data_t* patch)
{
    int            ret   = 0;
    const uint32_t base  = 0x03802000 + patch_number * 0x10;
    size_t         rsz   = 0;
    uint8_t*       reply = netmd_clean_read(devh, base + 4, 4, &rsz);

    if (reply != NULL)
    {
        if (rsz >= 4)
        {
            patch->addr = netmd_letohl(*(uint32_t*)reply);
            ret ++;
        }
        free(reply);
    }

    if ((reply = netmd_clean_read(devh, base + 8, 4, &rsz)) != NULL)
    {
        if (rsz >= 4)
        {
            memcpy(patch->data, reply, 4);
            ret ++;
        }
        free(reply);
    }

    return (ret == 2) ? NETMD_NO_ERROR : NETMD_ERROR;
}

//------------------------------------------------------------------------------
//! @brief      appy one patch
//!
//! @param[in]  devh         device handle
//! @param[in]  address      address where to apply patch
//! @param[in]  data         patch data
//! @param[in]  data_size    patch data size
//! @param[in]  patch_number number of patch
//------------------------------------------------------------------------------
static void netmd_patch(netmd_dev_handle *devh, uint32_t address, uint8_t data[], size_t data_size, int patch_number)
{
    // Original method written by Sir68k.
    assert(data_size == 4);

    const uint32_t base    = PERIPHERAL_BASE + patch_number  * 0x10;
    const uint32_t control = PERIPHERAL_BASE + MAX_PATCH     * 0x10;

    uint8_t  tmpdata[4];
    uint8_t* reply = NULL;
    size_t   rsz   = 0;

    // Write 5, 12 to main control
    tmpdata[0] =  5;
    tmpdata[1] = 12;

    netmd_clean_write(devh, control, &tmpdata[0], 1);
    netmd_clean_write(devh, control, &tmpdata[1], 1);

    // AND 0xFE with patch control
    reply = netmd_clean_read(devh, base, 4, &rsz);

    if (rsz > 0)
    {
        if (reply != NULL)
        {
            reply[0] &= 0xfe;
            netmd_clean_write(devh, base, reply, rsz);
            free(reply);
        }
    }

    // AND 0xFD with patch control
    reply = NULL;
    rsz   = 0;
    reply = netmd_clean_read(devh, base, 4, &rsz);

    if (rsz > 0)
    {
        if (reply != NULL)
        {
            reply[0] &= 0xfd;
            netmd_clean_write(devh, base, reply, rsz);
            free(reply);
        }
    }

    // Write patch ADDRESS
    *(uint32_t*)tmpdata = netmd_htolel(address);
    netmd_clean_write(devh, base + 4, tmpdata, sizeof(address));

    // Write patch VALUE
    netmd_clean_write(devh, base + 8, data, data_size);

    // OR 1 with patch control
    reply = NULL;
    rsz   = 0;
    reply = netmd_clean_read(devh, base, 4, &rsz);

    if (rsz > 0)
    {
        if (reply != NULL)
        {
            reply[0] |= 1;
            netmd_clean_write(devh, base, reply, rsz);
            free(reply);
        }
    }

    // write 5, 9 to main control
    tmpdata[0] = 5;
    tmpdata[1] = 9;

    netmd_clean_write(devh, control, &tmpdata[0], 1);
    netmd_clean_write(devh, control, &tmpdata[1], 1);
}

//------------------------------------------------------------------------------
//! @brief      undo one patch
//!
//! @param[in]  devh  device handle
//! @param[in]  pid   patch id
//------------------------------------------------------------------------------
static void netmd_unpatch(netmd_dev_handle *devh, patch_id_t pid)
{
    int patch_number = -1;

    for (int i = 0; i < MAX_PATCH; i++)
    {
        if (used_patches[i] == pid)
        {
            used_patches[i] = PID_UNUSED;
            patch_number = i;
        }
    }

    if (patch_number != -1)
    {
        const uint32_t base    = PERIPHERAL_BASE + patch_number  * 0x10;
        const uint32_t control = PERIPHERAL_BASE + MAX_PATCH     * 0x10;

        uint8_t  tmpdata[2];
        uint8_t* reply = NULL;
        size_t   rsz   = 0;

        // Write 5, 12 to main control
        tmpdata[0] =  5;
        tmpdata[1] = 12;

        netmd_clean_write(devh, control, &tmpdata[0], 1);
        netmd_clean_write(devh, control, &tmpdata[1], 1);

        // AND 0xFE with patch control
        reply = netmd_clean_read(devh, base, 4, &rsz);

        if (rsz > 0)
        {
            if (reply != NULL)
            {
                reply[0] &= 0xfe;
                netmd_clean_write(devh, base, reply, rsz);
                free(reply);
            }
        }

        // write 5, 9 to main control
        tmpdata[0] = 5;
        tmpdata[1] = 9;

        netmd_clean_write(devh, control, &tmpdata[0], 1);
        netmd_clean_write(devh, control, &tmpdata[1], 1);
    }
}

//------------------------------------------------------------------------------
//! @brief      appy safety patch if needed
//!
//! @param[in]  devh         device handle
//------------------------------------------------------------------------------
static void netmd_safety_patch(netmd_dev_handle *devh)
{
    sony_dev_info_t devcode   = netmd_get_device_code_ex(devh);
    uint32_t        addr      = get_patch_address(devcode, PID_SAFETY);
    uint8_t*        patch_cnt = get_patch_payload(devcode, PID_SAFETY);
    patch_data_t    patch     = {0, {0,}};

    if ((addr != 0) && (patch_cnt != NULL))
    {
        int safety_loaded = 0;

        for (int i = 0; i < MAX_PATCH; i++)
        {
            if (netmd_read_patch(devh, i, &patch) == NETMD_NO_ERROR)
            {
                if ((patch.addr == addr) && !memcmp(patch.data, patch_cnt, 4))
                {
                    netmd_log(NETMD_LOG_DEBUG, "Safety patch found at patch slot #%d\n", i);
                    safety_loaded   = 1;
                    used_patches[i] = PID_SAFETY;
                }

                // developer device
                if ((patch.addr == 0xe6c0) || (patch.addr == 0xe69c))
                {
                    netmd_log(NETMD_LOG_DEBUG, "Dev patch found at patch slot #%d\n", i);
                    safety_loaded   = 1;
                    used_patches[i] = PID_SAFETY;
                }
            }
        }

        if (safety_loaded == 0)
        {
            netmd_patch(devh, addr, patch_cnt, 4,
                        get_next_free_patch(PID_SAFETY));
            netmd_log(NETMD_LOG_DEBUG, "Safety patch applied.\n");
        }
    }
}

//------------------------------------------------------------------------------
//! @brief      enable factory commands
//!
//! @param[in]  devh device handle
//!
//! @return     netmd_error
//! @see        netmd_error
//------------------------------------------------------------------------------
static netmd_error netmd_enable_factory(netmd_dev_handle *devh)
{
    netmd_error ret   = NETMD_NO_ERROR;
    uint8_t     p1[]  = {0x00, 0x18, 0x09, 0x00, 0xff, 0x00, 0x00, 0x00,
                         0x00, 0x00};
    size_t      qsz   = 0;
    uint8_t*    query = netmd_format_query("00 1801 ff0e 4e6574204d442057616c6b6d616e", NULL, 0, &qsz);

    if (netmd_change_descriptor_state(devh, discSubunitIndentifier, nda_openread))
    {
        ret = NETMD_ERROR;
    }

    if (netmd_exch_message(devh, p1, sizeof(p1), _s_buff) < 0)
    {
        ret = NETMD_ERROR;
    }
    
    netmd_set_factory_write(1);

    if (query != NULL)
    {
        if (netmd_exch_message(devh, query, qsz, _s_buff) < 0)
        {
            ret = NETMD_ERROR;
        }
        free(query);
    }

    return ret;
}

// exported functions

//------------------------------------------------------------------------------
//! @brief      appy SP upload patch
//!
//! @param[in]  devh         device handle
//! @param[in]  chan_no      number of audio channels (1: mono, 2: stereo)
//!
//! @return     netmd_error
//! @see        netmd_error
//------------------------------------------------------------------------------
netmd_error netmd_apply_sp_patch(netmd_dev_handle *devh, int chan_no)
{
    netmd_error ret    = NETMD_NO_ERROR;
    patch_id_t  patch0 = PID_UNUSED;
    uint32_t    addr   = 0;
    size_t      rsz    = 0;
    uint8_t*    reply  = NULL;
    sony_dev_info_t devcode;

    netmd_log(NETMD_LOG_DEBUG, "Enable factory ...\n");
    ret = netmd_enable_factory(devh);
    if(ret){
        netmd_set_factory_write(0);
        return ret;
    }

    netmd_log(NETMD_LOG_DEBUG, "Apply safety patch ...\n");
    netmd_safety_patch(devh);

    netmd_log(NETMD_LOG_DEBUG, "Try to get device code ...\n");
    if ((devcode = netmd_get_device_code_ex(devh)) == SDI_S1200)
    {
        patch0 = PID_PATCH_0_B;
    }
    else if (devcode != SDI_UNKNOWN)
    {
        if ((addr = get_patch_address(devcode, PID_DEVTYPE)) != 0)
        {
            if ((reply = netmd_clean_read(devh, addr, 1, &rsz)) != NULL)
            {
                if (rsz > 0)
                {
                    if (reply[0] == 1)
                    {
                        patch0 = PID_PATCH_0_B;
                    }
                    else
                    {
                        patch0 = PID_PATCH_0_A;
                    }
                }
                free(reply);
            }
        }
    }

    if (patch0 != PID_UNUSED)
    {
        netmd_log(NETMD_LOG_DEBUG, "=== Apply patch 0 ===\n");
        netmd_patch(devh, get_patch_address(devcode, patch0),
                    get_patch_payload(devcode, PID_PATCH_0), 4,
                    get_next_free_patch(PID_PATCH_0));

        netmd_log(NETMD_LOG_DEBUG, "=== Apply patch common 1 ===\n");
        netmd_patch(devh, get_patch_address(devcode, PID_PATCH_CMN_1),
                    get_patch_payload(devcode, PID_PATCH_CMN_1), 4,
                    get_next_free_patch(PID_PATCH_CMN_1));

        netmd_log(NETMD_LOG_DEBUG, "=== Apply patch common 2 ===\n");
        netmd_patch(devh, get_patch_address(devcode, PID_PATCH_CMN_2),
                    get_patch_payload(devcode, PID_PATCH_CMN_2), 4,
                    get_next_free_patch(PID_PATCH_CMN_2));

        netmd_log(NETMD_LOG_DEBUG, "=== Apply prep patch ===\n");
        netmd_patch(devh, get_patch_address(devcode, PID_PREP_PATCH),
                    get_patch_payload(devcode, PID_PREP_PATCH), 4,
                    get_next_free_patch(PID_PREP_PATCH));

        netmd_log(NETMD_LOG_DEBUG, "=== Apply track type patch ===\n");
        reply    = get_patch_payload(devcode, PID_TRACK_TYPE);
        reply[1] = (chan_no == 1) ? 4 : 6; // mono or stereo
        netmd_patch(devh, get_patch_address(devcode, PID_TRACK_TYPE),
                    reply, 4, get_next_free_patch(PID_TRACK_TYPE));
    }
    else
    {
        ret = NETMD_ERROR;
        netmd_log(NETMD_LOG_ERROR, "Can't figure out patch 0!\n");
    }

    netmd_set_factory_write(0);

    return ret;
}

//------------------------------------------------------------------------------
//! @brief      undo SP upload patch
//!
//! @param[in]  devh  device handle
//------------------------------------------------------------------------------
void netmd_undo_sp_patch(netmd_dev_handle *devh)
{
    netmd_set_factory_write(1);
    netmd_log(NETMD_LOG_DEBUG, "=== Undo patch 0 ===\n");
    netmd_unpatch(devh, PID_PATCH_0);

    netmd_log(NETMD_LOG_DEBUG, "=== Undo patch common 1 ===\n");
    netmd_unpatch(devh, PID_PATCH_CMN_1);

    netmd_log(NETMD_LOG_DEBUG, "=== Undo patch common 2 ===\n");
    netmd_unpatch(devh, PID_PATCH_CMN_2);

    netmd_log(NETMD_LOG_DEBUG, "=== Undo prep patch ===\n");
    netmd_unpatch(devh, PID_PREP_PATCH);

    netmd_log(NETMD_LOG_DEBUG, "=== Undo track type patch ===\n");
    netmd_unpatch(devh, PID_TRACK_TYPE);
    netmd_set_factory_write(0);
}

//------------------------------------------------------------------------------
//! @brief      check if device supports sp upload
//!
//! @param[in]  devh  device handle
//!
//! @return     0 -> no support; esle
//------------------------------------------------------------------------------
int netmd_dev_supports_sp_upload(netmd_dev_handle *devh)
{
    int ret = 0;
    netmd_log(NETMD_LOG_DEBUG, "Enable factory ...\n");
    if (netmd_enable_factory(devh) == NETMD_NO_ERROR)
    {
        netmd_log(NETMD_LOG_DEBUG, "Get extended device info!\n");
        if (netmd_get_device_code_ex(devh) != SDI_UNKNOWN)
        {
            netmd_log(NETMD_LOG_DEBUG, "Supported device!\n");
            ret = 1;
        }
    }
    netmd_set_factory_write(0);
    return ret;
}
