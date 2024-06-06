/*	$NetBSD: option-value-type.h,v 1.4 2016/01/08 21:35:41 christos Exp $	*/

/*   -*- buffer-read-only: t -*- vi: set ro:
 *
 *  DO NOT EDIT THIS FILE   (stdin.h)
 *
 *  It has been AutoGen-ed
 *  From the definitions    stdin
 *  and the template file   str2enum
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name ``Bruce Korb'' nor the name of any other
 *    contributor may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * str2enum IS PROVIDED BY Bruce Korb ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bruce Korb OR ANY OTHER CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Command/Keyword Dispatcher
 */
#ifndef STR2ENUM_OPTION_VALUE_TYPE_H_GUARD
#define STR2ENUM_OPTION_VALUE_TYPE_H_GUARD 1
#include <sys/types.h>
#ifndef MISSING_INTTYPES_H
# include <inttypes.h>
#endif

typedef enum {
    VTP_INVALID_CMD = 0,
    VTP_CMD_STRING         = 1,
    VTP_CMD_INTEGER        = 2,
    VTP_CMD_BOOL           = 3,
    VTP_CMD_BOOLEAN        = 4,
    VTP_CMD_KEYWORD        = 5,
    VTP_CMD_SET            = 6,
    VTP_CMD_SET_MEMBERSHIP = 7,
    VTP_CMD_NESTED         = 8,
    VTP_CMD_HIERARCHY      = 9,
    VTP_COUNT_CMD
} option_value_type_enum_t;

extern option_value_type_enum_t
find_option_value_type_cmd(char const * str, size_t len);

#endif /* STR2ENUM_OPTION_VALUE_TYPE_H_GUARD */
/* end of option-value-type.h */
