/*******************************************************************************
*   (c) 2018 ZondaX GmbH
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if defined(LEDGER_SPECIFIC)
#include "bolos_target.h"
#endif

#if defined(TARGET_NANOX)
#define NV_CONST const
#define NV_VOL volatile
#else
#define NV_CONST
#define NV_VOL
#endif

#define NV_ALIGN __attribute__ ((aligned(64)))

#if defined (TARGET_NANOS) || defined(TARGET_NANOX)

#include "bolos_target.h"
#include "os.h"
#include "cx.h"

#if defined(TARGET_NANOX)
#include "ux.h"
#else
#include "os_io_seproxyhal.h"
#endif

#define WAIT_EVENT() io_seproxyhal_spi_recv(G_io_seproxyhal_spi_buffer, sizeof(G_io_seproxyhal_spi_buffer), 0)

#define UX_WAIT()  \
    while (!UX_DISPLAYED()) {  WAIT_EVENT();  UX_DISPLAY_NEXT_ELEMENT(); } \
    WAIT_EVENT(); \
    io_seproxyhal_general_status(); \
    WAIT_EVENT()


#define MEMMOVE os_memmove
#define MEMSET os_memset
#define MEMCPY os_memcpy
#define MEMCPY_NV nvm_write

void debug_printf(void* buffer);

#undef LOG
#undef LOGSTACK
#define LOG(str) debug_printf(str)
extern unsigned int app_stack_canary;
void __logstack();
#define LOGSTACK() __logstack()

#else
#include <string.h>
#define MEMMOVE memmove
#define MEMSET memset
#define MEMCPY memcpy
#define MEMCPY_NV memcpy
#define LOG(str)
#define LOGSTACK()
#endif

#include <inttypes.h>
#include <stdint.h>
#include <memory.h>

#define __Z_INLINE inline __attribute__((always_inline)) static

#define SET_NV(DST, TYPE, VAL) { \
    TYPE nvset_tmp=(VAL); \
    MEMCPY_NV((void*) PIC(DST), (void *) PIC(&nvset_tmp), sizeof(TYPE)); \
}

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define __SWAP(v) (((v) & 0x000000FFu) << 24u | ((v) & 0x0000FF00u) << 8u | ((v) & 0x00FF0000u) >> 8u | ((v) & 0xFF000000u) >> 24u)
#define HtoNL(v) __SWAP( v )
#define NtoHL(v) __SWAP( v )
#else
#define HtoNL(x) (x)
#define NtoHL(x) (x)
#endif

__Z_INLINE void array_to_hexstr(char *dst, const uint8_t *src, uint8_t count) {
    const char hexchars[] = "0123456789ABCDEF";
    for (uint8_t i = 0; i < count; i++, src++) {
        *dst++ = hexchars[*src >> 4];
        *dst++ = hexchars[*src & 0x0F];
    }
    *dst = 0; // terminate string
}

__Z_INLINE const char *int64_to_str(char *data, int size, int64_t number) {
    char temp[] = "-9223372036854775808";

    char *ptr = temp;
    int64_t num = number;
    int sign = 1;
    if (number < 0) {
        sign = -1;
    }
    while (num != 0) {
        *ptr++ = '0' + (num % 10) * sign;
        num /= 10;
    }
    if (number < 0) {
        *ptr++ = '-';
    } else if (number == 0) {
        *ptr++ = '0';
    }
    int distance = (ptr - temp) + 1;
    if (size < distance) {
        return "Size too small";
    }
    int index = 0;
    while (--ptr >= temp) {
        data[index++] = *ptr;
    }
    data[index] = '\0';
    return NULL;
}

__Z_INLINE int8_t str_to_int8(const char *start, const char *end, char *error) {

    int sign = 1;
    if (*start == '-') {
        sign = -1;
        start++;
    }

    int64_t value = 0;
    int multiplier = 1;
    for (const char *s = end - 1; s >= start; s--) {
        int delta = (*s - '0');
        if (delta >= 0 && delta <= 9) {
            value += (delta * multiplier);
            multiplier *= 10;
        } else {
            if (error != NULL) {
                *error = 1;
                return 0;
            }
        }
    }

    value *= sign;
    if (value >= INT8_MIN && value <= INT8_MAX) {
        return (int8_t) value;
    }
    if (error != NULL) {
        *error = 1;
    }
    return 0;
}

__Z_INLINE int64_t str_to_int64(const char *start, const char *end, char *error) {

    int sign = 1;
    if (*start == '-') {
        sign = -1;
        start++;
    }

    int64_t value = 0;
    uint64_t multiplier = 1;
    for (const char *s = end - 1; s >= start; s--) {
        int delta = (*s - '0');
        if (delta >= 0 && delta <= 9) {
            value += (delta * multiplier);
            multiplier *= 10;
        } else {
            if (error != NULL) {
                *error = 1;
                return 0;
            }
        }
    }

    return value * sign;
}

__Z_INLINE void fpuint64_to_str(char *dst, const uint64_t value, uint8_t decimals) {
    char buffer[30];

    int64_to_str(buffer, 30, value);
    size_t digits = strlen(buffer);

    if (digits <= decimals) {
        *dst++ = '0';
        *dst++ = '.';
        for (uint16_t i = 0; i < decimals - digits; i++, dst++)
            *dst = '0';
        strcpy(dst, buffer);
    } else {
        strcpy(dst, buffer);
        const size_t shift = digits - decimals;
        dst = dst + shift;
        *dst++ = '.';

        char *p = buffer + shift;
        strcpy(dst, p);
    }
}

__Z_INLINE uint64_t uint64_from_BEarray(const uint8_t data[8]) {
    uint64_t result = 0;
    for (int i = 0; i < 8; i++) {
        result <<= 8;
        result += data[i];
    }
    return result;
}

size_t asciify(char *utf8_in);

size_t asciify_ext(const char *utf8_in, char *ascii_only_out);

#ifndef PIC
#define PIC(x) (x)
#endif

#ifdef __cplusplus
}
#endif
