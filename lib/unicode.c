/* -*-  mode:c; tab-width:8; c-basic-offset:8; indent-tabs-mode:nil;  -*- */
/*
   Copyright (C) 2016 by Ronnie Sahlberg <ronniesahlberg@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2.1 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef STDC_HEADERS
#include <stddef.h>
#endif

#include <stdio.h>

#include "smb2.h"
#include "libsmb2.h"
#include "libsmb2-private.h"

/* Count number of leading 1 bits in the char */
static int l1(char c)
{
        int i = 0;
        while (c & 0x80) {
                i++;
                c <<= 1;
        }
        return i;
}

/* Validates that utf8 points to a valid utf8 codepoint.
 * Will update **utf8 to point at the next character in the string.
 * return 0 if the encoding is valid and
 * -1 if not.
 * If the encoding is valid the codepoint will be returned in *cp.
 */
static int validate_utf8_cp(char **utf8, uint16_t *cp)
{
        int c = *(*utf8)++;
        int l = l1(c);

        switch (l) {
        case 0:
                /* 7-bit ascii is always ok */
                *cp = c & 0x7f;
                return 0;
        case 1:
                /* 10.. .... can never start a new codepoint */
                return -1;
        case 2:
        case 3:
                *cp = c & 0x1f;
                /* 2 and 3 byte sequences must always be followed by exactly
                 * 1 or 2 chars matching 10.. .... 
                 */
                while(--l) {
                        c = *(*utf8)++;
                        if (l1(c) != 1) {
                                return -1;
                        }
                        *cp <<= 6;
                        *cp |= (c & 0x3f);
                }
                return 0;
        }
        return -1;
}

/* Validate that the given string is properly formated UTF8.
 * Returns >=0 if valid UTF8 and -1 if not.
 */
static int validate_utf8_str(char *utf8)
{
        char *u = utf8;
        int i = 0;
        uint16_t cp;
        
        while (*u) {
                if (validate_utf8_cp(&u, &cp) < 0) {
                        return -1;
                }
                i++;
        }
        return i;
}

struct ucs2 *utf8_to_ucs2(char *utf8)
{
        struct ucs2 *ucs2;
        int i, len;

        len = validate_utf8_str(utf8);
        if (len < 0) {
                return NULL;
        }

        ucs2 = malloc(offsetof(struct ucs2, val) + 2 * len);
        if (ucs2 == NULL) {
                return NULL;
        }

        ucs2->len = len;
        for (i = 0; i < len; i++) {
                validate_utf8_cp(&utf8, &ucs2->val[i]);
        }
        
        return ucs2;
}

