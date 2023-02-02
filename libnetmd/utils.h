#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "error.h"

typedef struct {
        unsigned char content[255];
        size_t length;
        size_t position;
} netmd_response;

#ifndef netmd_min
    #define netmd_min(a,b) ((a)<(b)?(a):(b))
#endif


#ifdef WIN32
    #include <windows.h>
    #define msleep(x) Sleep(x)
#else
    #define msleep(x) usleep(1000*x)
#endif

/**
 * union to hold data used by netmd_format_query() function
 */
typedef union {
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    uint8_t  u8;
    uint8_t *pu8;
} netmd_query_data_store_t;

/**
 * structure to be used for data detail by netmd_format_query() function
 */
typedef struct {
    netmd_query_data_store_t data;
    size_t size;
} netmd_query_data_t;

/**
 * format place holder used in netmd_format_query()
 */
typedef enum {
    netmd_fmt_byte   = 'b',
    netmd_fmt_word   = 'w',
    netmd_fmt_dword  = 'd',
    netmd_fmt_qword  = 'q',
    netmd_fmt_barray = '*',
} netmd_format_items_t;

/**
 * help endianess helper used by netmd_format_query() function
 */
typedef enum {
    netmd_hto_littleendian = '<',
    netmd_hto_bigendian    = '>',
    netmd_no_convert
} netmd_endianess_t;

/**
 * structure to be used for capture data by netmd_scan_query() function
 */
typedef struct {
    netmd_format_items_t tp;
    netmd_query_data_store_t data;
    size_t size;
} netmd_capture_data_t;


unsigned char proper_to_bcd_single(unsigned char value);
unsigned char* proper_to_bcd(unsigned int value, unsigned char* target, size_t len);
unsigned char bcd_to_proper_single(unsigned char value);
unsigned int bcd_to_proper(unsigned char* value, size_t len);

void netmd_check_response_bulk(netmd_response *response, const unsigned char* const expected,
                               const size_t expected_length, netmd_error *error);

void netmd_check_response_word(netmd_response *response, const uint16_t expected,
                               netmd_error *error);

void netmd_check_response(netmd_response *response, const unsigned char expected,
                          netmd_error *error);

void netmd_read_response_bulk(netmd_response *response, unsigned char* target,
                              const size_t length, netmd_error *error);



unsigned char *netmd_copy_word_to_buffer(unsigned char **buf, uint16_t value, int little_endian);
unsigned char *netmd_copy_doubleword_to_buffer(unsigned char **buf, uint32_t value, int little_endian);
unsigned char *netmd_copy_quadword_to_buffer(unsigned char **buf, uint64_t value);

unsigned char netmd_read(netmd_response *response);
uint16_t netmd_read_word(netmd_response *response);
uint32_t netmd_read_doubleword(netmd_response *response);
uint64_t netmd_read_quadword(netmd_response *response);

//------------------------------------------------------------------------------
//! @brief      htons short and dirty
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint16_t netmd_htons(uint16_t in);

//------------------------------------------------------------------------------
//! @brief      htonl short and dirty
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint32_t netmd_htonl(uint32_t in);

//------------------------------------------------------------------------------
//! @brief      ntohs short and dirty
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint16_t netmd_ntohs(uint16_t in);

//------------------------------------------------------------------------------
//! @brief      ntohl short and dirty
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint32_t netmd_ntohl(uint32_t in);

//------------------------------------------------------------------------------
//! @brief      host to little endian short
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint16_t netmd_htoles(uint16_t in);

//------------------------------------------------------------------------------
//! @brief      host to little endian long
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint32_t netmd_htolel(uint32_t in);

//------------------------------------------------------------------------------
//! @brief      host to little endian long long
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint64_t netmd_htolell(uint64_t in);

//------------------------------------------------------------------------------
//! @brief      host to big endian long long
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint64_t netmd_htonll(uint64_t in);

//------------------------------------------------------------------------------
//! @brief      little to host endian short
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint16_t netmd_letohs(uint16_t in);

//------------------------------------------------------------------------------
//! @brief      little endian to host long
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint32_t netmd_letohl(uint32_t in);

//------------------------------------------------------------------------------
//! @brief      little endian to host long long
//!
//! @param[in]  in    value to convert
//!
//! @return     converted or original value
//------------------------------------------------------------------------------
uint64_t netmd_letohll(uint64_t in);

//------------------------------------------------------------------------------
//! @brief     calculate NetMD checksum
//!
//! @param[in] data pointer to data
//! @param[in] size data size
//!
//! @return    checksum
//------------------------------------------------------------------------------
unsigned int netmd_calculate_checksum(unsigned char data[], size_t size);

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
uint8_t* netmd_format_query(const char* format, const netmd_query_data_t argv[], int argc, size_t* query_sz);

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
int netmd_scan_query(const uint8_t data[], size_t size, const char* format, netmd_capture_data_t** argv, int* argc);

#endif
