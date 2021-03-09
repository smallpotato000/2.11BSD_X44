/*	$NetBSD: mulaw.c,v 1.12 1998/08/09 21:41:45 mycroft Exp $	*/

/*
 * Copyright (c) 1991-1993 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/types.h>
#include <sys/audioio.h>
#include <machine/endian_machdep.h>
#include <dev/audio/mulaw/mulaw.h>

#if BYTE_ORDER == LITTLE_ENDIAN
#define LO 0
#define HI 1
#else
#define LO 1
#define HI 0
#endif

/*
 * This table converts a (8 bit) mulaw value two a 16 bit value.
 * The 16 bits are represented as an array of two butes for easier access
 * to the individual bytes.
 */
static u_char mulawtolin16[256][2] = {
	{0x02,0x84}, {0x06,0x84}, {0x0a,0x84}, {0x0e,0x84},
	{0x12,0x84}, {0x16,0x84}, {0x1a,0x84}, {0x1e,0x84},
	{0x22,0x84}, {0x26,0x84}, {0x2a,0x84}, {0x2e,0x84},
	{0x32,0x84}, {0x36,0x84}, {0x3a,0x84}, {0x3e,0x84},
	{0x41,0x84}, {0x43,0x84}, {0x45,0x84}, {0x47,0x84},
	{0x49,0x84}, {0x4b,0x84}, {0x4d,0x84}, {0x4f,0x84},
	{0x51,0x84}, {0x53,0x84}, {0x55,0x84}, {0x57,0x84},
	{0x59,0x84}, {0x5b,0x84}, {0x5d,0x84}, {0x5f,0x84},
	{0x61,0x04}, {0x62,0x04}, {0x63,0x04}, {0x64,0x04},
	{0x65,0x04}, {0x66,0x04}, {0x67,0x04}, {0x68,0x04},
	{0x69,0x04}, {0x6a,0x04}, {0x6b,0x04}, {0x6c,0x04},
	{0x6d,0x04}, {0x6e,0x04}, {0x6f,0x04}, {0x70,0x04},
	{0x70,0xc4}, {0x71,0x44}, {0x71,0xc4}, {0x72,0x44},
	{0x72,0xc4}, {0x73,0x44}, {0x73,0xc4}, {0x74,0x44},
	{0x74,0xc4}, {0x75,0x44}, {0x75,0xc4}, {0x76,0x44},
	{0x76,0xc4}, {0x77,0x44}, {0x77,0xc4}, {0x78,0x44},
	{0x78,0xa4}, {0x78,0xe4}, {0x79,0x24}, {0x79,0x64},
	{0x79,0xa4}, {0x79,0xe4}, {0x7a,0x24}, {0x7a,0x64},
	{0x7a,0xa4}, {0x7a,0xe4}, {0x7b,0x24}, {0x7b,0x64},
	{0x7b,0xa4}, {0x7b,0xe4}, {0x7c,0x24}, {0x7c,0x64},
	{0x7c,0x94}, {0x7c,0xb4}, {0x7c,0xd4}, {0x7c,0xf4},
	{0x7d,0x14}, {0x7d,0x34}, {0x7d,0x54}, {0x7d,0x74},
	{0x7d,0x94}, {0x7d,0xb4}, {0x7d,0xd4}, {0x7d,0xf4},
	{0x7e,0x14}, {0x7e,0x34}, {0x7e,0x54}, {0x7e,0x74},
	{0x7e,0x8c}, {0x7e,0x9c}, {0x7e,0xac}, {0x7e,0xbc},
	{0x7e,0xcc}, {0x7e,0xdc}, {0x7e,0xec}, {0x7e,0xfc},
	{0x7f,0x0c}, {0x7f,0x1c}, {0x7f,0x2c}, {0x7f,0x3c},
	{0x7f,0x4c}, {0x7f,0x5c}, {0x7f,0x6c}, {0x7f,0x7c},
	{0x7f,0x88}, {0x7f,0x90}, {0x7f,0x98}, {0x7f,0xa0},
	{0x7f,0xa8}, {0x7f,0xb0}, {0x7f,0xb8}, {0x7f,0xc0},
	{0x7f,0xc8}, {0x7f,0xd0}, {0x7f,0xd8}, {0x7f,0xe0},
	{0x7f,0xe8}, {0x7f,0xf0}, {0x7f,0xf8}, {0x80,0x00},
	{0xfd,0x7c}, {0xf9,0x7c}, {0xf5,0x7c}, {0xf1,0x7c},
	{0xed,0x7c}, {0xe9,0x7c}, {0xe5,0x7c}, {0xe1,0x7c},
	{0xdd,0x7c}, {0xd9,0x7c}, {0xd5,0x7c}, {0xd1,0x7c},
	{0xcd,0x7c}, {0xc9,0x7c}, {0xc5,0x7c}, {0xc1,0x7c},
	{0xbe,0x7c}, {0xbc,0x7c}, {0xba,0x7c}, {0xb8,0x7c},
	{0xb6,0x7c}, {0xb4,0x7c}, {0xb2,0x7c}, {0xb0,0x7c},
	{0xae,0x7c}, {0xac,0x7c}, {0xaa,0x7c}, {0xa8,0x7c},
	{0xa6,0x7c}, {0xa4,0x7c}, {0xa2,0x7c}, {0xa0,0x7c},
	{0x9e,0xfc}, {0x9d,0xfc}, {0x9c,0xfc}, {0x9b,0xfc},
	{0x9a,0xfc}, {0x99,0xfc}, {0x98,0xfc}, {0x97,0xfc},
	{0x96,0xfc}, {0x95,0xfc}, {0x94,0xfc}, {0x93,0xfc},
	{0x92,0xfc}, {0x91,0xfc}, {0x90,0xfc}, {0x8f,0xfc},
	{0x8f,0x3c}, {0x8e,0xbc}, {0x8e,0x3c}, {0x8d,0xbc},
	{0x8d,0x3c}, {0x8c,0xbc}, {0x8c,0x3c}, {0x8b,0xbc},
	{0x8b,0x3c}, {0x8a,0xbc}, {0x8a,0x3c}, {0x89,0xbc},
	{0x89,0x3c}, {0x88,0xbc}, {0x88,0x3c}, {0x87,0xbc},
	{0x87,0x5c}, {0x87,0x1c}, {0x86,0xdc}, {0x86,0x9c},
	{0x86,0x5c}, {0x86,0x1c}, {0x85,0xdc}, {0x85,0x9c},
	{0x85,0x5c}, {0x85,0x1c}, {0x84,0xdc}, {0x84,0x9c},
	{0x84,0x5c}, {0x84,0x1c}, {0x83,0xdc}, {0x83,0x9c},
	{0x83,0x6c}, {0x83,0x4c}, {0x83,0x2c}, {0x83,0x0c},
	{0x82,0xec}, {0x82,0xcc}, {0x82,0xac}, {0x82,0x8c},
	{0x82,0x6c}, {0x82,0x4c}, {0x82,0x2c}, {0x82,0x0c},
	{0x81,0xec}, {0x81,0xcc}, {0x81,0xac}, {0x81,0x8c},
	{0x81,0x74}, {0x81,0x64}, {0x81,0x54}, {0x81,0x44},
	{0x81,0x34}, {0x81,0x24}, {0x81,0x14}, {0x81,0x04},
	{0x80,0xf4}, {0x80,0xe4}, {0x80,0xd4}, {0x80,0xc4},
	{0x80,0xb4}, {0x80,0xa4}, {0x80,0x94}, {0x80,0x84},
	{0x80,0x78}, {0x80,0x70}, {0x80,0x68}, {0x80,0x60},
	{0x80,0x58}, {0x80,0x50}, {0x80,0x48}, {0x80,0x40},
	{0x80,0x38}, {0x80,0x30}, {0x80,0x28}, {0x80,0x20},
	{0x80,0x18}, {0x80,0x10}, {0x80,0x08}, {0x80,0x00},
};

static u_char lintomulaw[256] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01,
	0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03,
	0x03, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05,
	0x05, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07,
	0x07, 0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09,
	0x09, 0x0a, 0x0a, 0x0a, 0x0a, 0x0b, 0x0b, 0x0b,
	0x0b, 0x0c, 0x0c, 0x0c, 0x0c, 0x0d, 0x0d, 0x0d,
	0x0d, 0x0e, 0x0e, 0x0e, 0x0e, 0x0f, 0x0f, 0x0f,
	0x0f, 0x10, 0x10, 0x11, 0x11, 0x12, 0x12, 0x13,
	0x13, 0x14, 0x14, 0x15, 0x15, 0x16, 0x16, 0x17,
	0x17, 0x18, 0x18, 0x19, 0x19, 0x1a, 0x1a, 0x1b,
	0x1b, 0x1c, 0x1c, 0x1d, 0x1d, 0x1e, 0x1e, 0x1f,
	0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e,
	0x2f, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c,
	0x3e, 0x41, 0x45, 0x49, 0x4d, 0x53, 0x5b, 0x67,
	0xff, 0xe7, 0xdb, 0xd3, 0xcd, 0xc9, 0xc5, 0xc1,
	0xbe, 0xbc, 0xba, 0xb8, 0xb6, 0xb4, 0xb2, 0xb0,
	0xaf, 0xae, 0xad, 0xac, 0xab, 0xaa, 0xa9, 0xa8,
	0xa7, 0xa6, 0xa5, 0xa4, 0xa3, 0xa2, 0xa1, 0xa0,
	0x9f, 0x9f, 0x9e, 0x9e, 0x9d, 0x9d, 0x9c, 0x9c,
	0x9b, 0x9b, 0x9a, 0x9a, 0x99, 0x99, 0x98, 0x98,
	0x97, 0x97, 0x96, 0x96, 0x95, 0x95, 0x94, 0x94,
	0x93, 0x93, 0x92, 0x92, 0x91, 0x91, 0x90, 0x90,
	0x8f, 0x8f, 0x8f, 0x8f, 0x8e, 0x8e, 0x8e, 0x8e,
	0x8d, 0x8d, 0x8d, 0x8d, 0x8c, 0x8c, 0x8c, 0x8c,
	0x8b, 0x8b, 0x8b, 0x8b, 0x8a, 0x8a, 0x8a, 0x8a,
	0x89, 0x89, 0x89, 0x89, 0x88, 0x88, 0x88, 0x88,
	0x87, 0x87, 0x87, 0x87, 0x86, 0x86, 0x86, 0x86,
	0x85, 0x85, 0x85, 0x85, 0x84, 0x84, 0x84, 0x84,
	0x83, 0x83, 0x83, 0x83, 0x82, 0x82, 0x82, 0x82,
	0x81, 0x81, 0x81, 0x81, 0x80, 0x80, 0x80, 0x80,
};

static u_char alawtolin16[256][2] = {
	{0x6a,0x80}, {0x6b,0x80}, {0x68,0x80}, {0x69,0x80},
	{0x6e,0x80}, {0x6f,0x80}, {0x6c,0x80}, {0x6d,0x80},
	{0x62,0x80}, {0x63,0x80}, {0x60,0x80}, {0x61,0x80},
	{0x66,0x80}, {0x67,0x80}, {0x64,0x80}, {0x65,0x80},
	{0x75,0x40}, {0x75,0xc0}, {0x74,0x40}, {0x74,0xc0},
	{0x77,0x40}, {0x77,0xc0}, {0x76,0x40}, {0x76,0xc0},
	{0x71,0x40}, {0x71,0xc0}, {0x70,0x40}, {0x70,0xc0},
	{0x73,0x40}, {0x73,0xc0}, {0x72,0x40}, {0x72,0xc0},
	{0x2a,0x00}, {0x2e,0x00}, {0x22,0x00}, {0x26,0x00},
	{0x3a,0x00}, {0x3e,0x00}, {0x32,0x00}, {0x36,0x00},
	{0x0a,0x00}, {0x0e,0x00}, {0x02,0x00}, {0x06,0x00},
	{0x1a,0x00}, {0x1e,0x00}, {0x12,0x00}, {0x16,0x00},
	{0x55,0x00}, {0x57,0x00}, {0x51,0x00}, {0x53,0x00},
	{0x5d,0x00}, {0x5f,0x00}, {0x59,0x00}, {0x5b,0x00},
	{0x45,0x00}, {0x47,0x00}, {0x41,0x00}, {0x43,0x00},
	{0x4d,0x00}, {0x4f,0x00}, {0x49,0x00}, {0x4b,0x00},
	{0x7e,0xa8}, {0x7e,0xb8}, {0x7e,0x88}, {0x7e,0x98},
	{0x7e,0xe8}, {0x7e,0xf8}, {0x7e,0xc8}, {0x7e,0xd8},
	{0x7e,0x28}, {0x7e,0x38}, {0x7e,0x08}, {0x7e,0x18},
	{0x7e,0x68}, {0x7e,0x78}, {0x7e,0x48}, {0x7e,0x58},
	{0x7f,0xa8}, {0x7f,0xb8}, {0x7f,0x88}, {0x7f,0x98},
	{0x7f,0xe8}, {0x7f,0xf8}, {0x7f,0xc8}, {0x7f,0xd8},
	{0x7f,0x28}, {0x7f,0x38}, {0x7f,0x08}, {0x7f,0x18},
	{0x7f,0x68}, {0x7f,0x78}, {0x7f,0x48}, {0x7f,0x58},
	{0x7a,0xa0}, {0x7a,0xe0}, {0x7a,0x20}, {0x7a,0x60},
	{0x7b,0xa0}, {0x7b,0xe0}, {0x7b,0x20}, {0x7b,0x60},
	{0x78,0xa0}, {0x78,0xe0}, {0x78,0x20}, {0x78,0x60},
	{0x79,0xa0}, {0x79,0xe0}, {0x79,0x20}, {0x79,0x60},
	{0x7d,0x50}, {0x7d,0x70}, {0x7d,0x10}, {0x7d,0x30},
	{0x7d,0xd0}, {0x7d,0xf0}, {0x7d,0x90}, {0x7d,0xb0},
	{0x7c,0x50}, {0x7c,0x70}, {0x7c,0x10}, {0x7c,0x30},
	{0x7c,0xd0}, {0x7c,0xf0}, {0x7c,0x90}, {0x7c,0xb0},
	{0x95,0x80}, {0x94,0x80}, {0x97,0x80}, {0x96,0x80},
	{0x91,0x80}, {0x90,0x80}, {0x93,0x80}, {0x92,0x80},
	{0x9d,0x80}, {0x9c,0x80}, {0x9f,0x80}, {0x9e,0x80},
	{0x99,0x80}, {0x98,0x80}, {0x9b,0x80}, {0x9a,0x80},
	{0x8a,0xc0}, {0x8a,0x40}, {0x8b,0xc0}, {0x8b,0x40},
	{0x88,0xc0}, {0x88,0x40}, {0x89,0xc0}, {0x89,0x40},
	{0x8e,0xc0}, {0x8e,0x40}, {0x8f,0xc0}, {0x8f,0x40},
	{0x8c,0xc0}, {0x8c,0x40}, {0x8d,0xc0}, {0x8d,0x40},
	{0xd6,0x00}, {0xd2,0x00}, {0xde,0x00}, {0xda,0x00},
	{0xc6,0x00}, {0xc2,0x00}, {0xce,0x00}, {0xca,0x00},
	{0xf6,0x00}, {0xf2,0x00}, {0xfe,0x00}, {0xfa,0x00},
	{0xe6,0x00}, {0xe2,0x00}, {0xee,0x00}, {0xea,0x00},
	{0xab,0x00}, {0xa9,0x00}, {0xaf,0x00}, {0xad,0x00},
	{0xa3,0x00}, {0xa1,0x00}, {0xa7,0x00}, {0xa5,0x00},
	{0xbb,0x00}, {0xb9,0x00}, {0xbf,0x00}, {0xbd,0x00},
	{0xb3,0x00}, {0xb1,0x00}, {0xb7,0x00}, {0xb5,0x00},
	{0x81,0x58}, {0x81,0x48}, {0x81,0x78}, {0x81,0x68},
	{0x81,0x18}, {0x81,0x08}, {0x81,0x38}, {0x81,0x28},
	{0x81,0xd8}, {0x81,0xc8}, {0x81,0xf8}, {0x81,0xe8},
	{0x81,0x98}, {0x81,0x88}, {0x81,0xb8}, {0x81,0xa8},
	{0x80,0x58}, {0x80,0x48}, {0x80,0x78}, {0x80,0x68},
	{0x80,0x18}, {0x80,0x08}, {0x80,0x38}, {0x80,0x28},
	{0x80,0xd8}, {0x80,0xc8}, {0x80,0xf8}, {0x80,0xe8},
	{0x80,0x98}, {0x80,0x88}, {0x80,0xb8}, {0x80,0xa8},
	{0x85,0x60}, {0x85,0x20}, {0x85,0xe0}, {0x85,0xa0},
	{0x84,0x60}, {0x84,0x20}, {0x84,0xe0}, {0x84,0xa0},
	{0x87,0x60}, {0x87,0x20}, {0x87,0xe0}, {0x87,0xa0},
	{0x86,0x60}, {0x86,0x20}, {0x86,0xe0}, {0x86,0xa0},
	{0x82,0xb0}, {0x82,0x90}, {0x82,0xf0}, {0x82,0xd0},
	{0x82,0x30}, {0x82,0x10}, {0x82,0x70}, {0x82,0x50},
	{0x83,0xb0}, {0x83,0x90}, {0x83,0xf0}, {0x83,0xd0},
	{0x83,0x30}, {0x83,0x10}, {0x83,0x70}, {0x83,0x50},
};

static u_char lintoalaw[256] = {
	0x2a, 0x2a, 0x2a, 0x2a, 0x2b, 0x2b, 0x2b, 0x2b,
	0x28, 0x28, 0x28, 0x28, 0x29, 0x29, 0x29, 0x29,
	0x2e, 0x2e, 0x2e, 0x2e, 0x2f, 0x2f, 0x2f, 0x2f,
	0x2c, 0x2c, 0x2c, 0x2c, 0x2d, 0x2d, 0x2d, 0x2d,
	0x22, 0x22, 0x22, 0x22, 0x23, 0x23, 0x23, 0x23,
	0x20, 0x20, 0x20, 0x20, 0x21, 0x21, 0x21, 0x21,
	0x26, 0x26, 0x26, 0x26, 0x27, 0x27, 0x27, 0x27,
	0x24, 0x24, 0x24, 0x24, 0x25, 0x25, 0x25, 0x25,
	0x3a, 0x3a, 0x3b, 0x3b, 0x38, 0x38, 0x39, 0x39,
	0x3e, 0x3e, 0x3f, 0x3f, 0x3c, 0x3c, 0x3d, 0x3d,
	0x32, 0x32, 0x33, 0x33, 0x30, 0x30, 0x31, 0x31,
	0x36, 0x36, 0x37, 0x37, 0x34, 0x34, 0x35, 0x35,
	0x0a, 0x0b, 0x08, 0x09, 0x0e, 0x0f, 0x0c, 0x0d,
	0x02, 0x03, 0x00, 0x01, 0x06, 0x07, 0x04, 0x05,
	0x1a, 0x18, 0x1e, 0x1c, 0x12, 0x10, 0x16, 0x14,
	0x6a, 0x6e, 0x62, 0x66, 0x7a, 0x72, 0x4a, 0x5a,
	0xd5, 0xc5, 0xf5, 0xfd, 0xe5, 0xe1, 0xed, 0xe9,
	0x95, 0x97, 0x91, 0x93, 0x9d, 0x9f, 0x99, 0x9b,
	0x85, 0x84, 0x87, 0x86, 0x81, 0x80, 0x83, 0x82,
	0x8d, 0x8c, 0x8f, 0x8e, 0x89, 0x88, 0x8b, 0x8a,
	0xb5, 0xb5, 0xb4, 0xb4, 0xb7, 0xb7, 0xb6, 0xb6,
	0xb1, 0xb1, 0xb0, 0xb0, 0xb3, 0xb3, 0xb2, 0xb2,
	0xbd, 0xbd, 0xbc, 0xbc, 0xbf, 0xbf, 0xbe, 0xbe,
	0xb9, 0xb9, 0xb8, 0xb8, 0xbb, 0xbb, 0xba, 0xba,
	0xa5, 0xa5, 0xa5, 0xa5, 0xa4, 0xa4, 0xa4, 0xa4,
	0xa7, 0xa7, 0xa7, 0xa7, 0xa6, 0xa6, 0xa6, 0xa6,
	0xa1, 0xa1, 0xa1, 0xa1, 0xa0, 0xa0, 0xa0, 0xa0,
	0xa3, 0xa3, 0xa3, 0xa3, 0xa2, 0xa2, 0xa2, 0xa2,
	0xad, 0xad, 0xad, 0xad, 0xac, 0xac, 0xac, 0xac,
	0xaf, 0xaf, 0xaf, 0xaf, 0xae, 0xae, 0xae, 0xae,
	0xa9, 0xa9, 0xa9, 0xa9, 0xa8, 0xa8, 0xa8, 0xa8,
	0xab, 0xab, 0xab, 0xab, 0xaa, 0xaa, 0xaa, 0xaa,
};

void
mulaw_to_ulinear8(v, p, cc)
	void *v;
	u_char *p;
	int cc;
{
	/* Use the 16 bit table for 8 bits too. */
	while (--cc >= 0) {
		*p = mulawtolin16[*p][0];
		++p;
	}
}

void
mulaw_to_slinear8(v, p, cc)
	void *v;
	u_char *p;
	int cc;
{
	/* Use the 16 bit table for 8 bits too. */
	while (--cc >= 0) {
		*p = mulawtolin16[*p][0] ^ 0x80;
		++p;
	}
}

void
mulaw_to_ulinear16(v, p, cc)
	void *v;
	u_char *p;
	int cc;
{
	u_char *q = p;

	p += cc;
	q += cc << 1;
	while (--cc >= 0) {
		--p;
		q -= 2;
		q[HI] = mulawtolin16[*p][0];
		q[LO] = mulawtolin16[*p][1];
	}
}

void
mulaw_to_slinear16(v, p, cc)
	void *v;
	u_char *p;
	int cc;
{
	u_char *q = p;

	p += cc;
	q += cc << 1;
	while (--cc >= 0) {
		--p;
		q -= 2;
		q[HI] = mulawtolin16[*p][0] ^ 0x80;
		q[LO] = mulawtolin16[*p][1];
	}
}

void
ulinear8_to_mulaw(v, p, cc)
	void *v;
	u_char *p;
	int cc;
{
	while (--cc >= 0) {
		*p = lintomulaw[*p];
		++p;
	}
}

void
slinear8_to_mulaw(v, p, cc)
	void *v;
	u_char *p;
	int cc;
{
	while (--cc >= 0) {
		*p = lintomulaw[*p ^ 0x80];
		++p;
	}
}

void
slinear16_to_mulaw(v, p, cc)
	void *v;
	u_char *p;
	int cc;
{
	u_char *q = p + 1;	/* q points higher byte. */

	while (--cc >= 0) {
		p[HI] = lintomulaw[*q[0] ^ 0x80];
		p[LO] = lintomulaw[*q[1] ^ 0x80];
		++p;
		q +=2 ;
	}
}

void
alaw_to_ulinear8(v, p, cc)
	void *v;
	u_char *p;
	int cc;
{
	/* Use the 16 bit table for 8 bits too. */
	while (--cc >= 0) {
		*p = alawtolin16[*p][0];
		++p;
	}
}

void
alaw_to_slinear8(v, p, cc)
	void *v;
	u_char *p;
	int cc;
{
	/* Use the 16 bit table for 8 bits too. */
	while (--cc >= 0) {
		*p = alawtolin16[*p][0] ^ 0x80;
		++p;
	}
}

void
alaw_to_ulinear16(v, p, cc)
	void *v;
	u_char *p;
	int cc;
{
	u_char *q = p;

	p += cc;
	q += cc << 1;
	while (--cc >= 0) {
		--p;
		q -= 2;
		q[HI] = alawtolin16[*p][0];
		q[LO] = alawtolin16[*p][1];
	}
}

void
alaw_to_slinear16(v, p, cc)
	void *v;
	u_char *p;
	int cc;
{
	u_char *q = p;

	p += cc;
	q += cc << 1;
	while (--cc >= 0) {
		--p;
		q -= 2;
		q[HI] = alawtolin16[*p][0] ^ 0x80;
		q[LO] = alawtolin16[*p][1];
	}
}

void
ulinear8_to_alaw(v, p, cc)
	void *v;
	u_char *p;
	int cc;
{
	while (--cc >= 0) {
		*p = lintoalaw[*p];
		++p;
	}
}

void
slinear8_to_alaw(v, p, cc)
	void *v;
	u_char *p;
	int cc;
{
	while (--cc >= 0) {
		*p = lintoalaw[*p ^ 0x80];
		++p;
	}
}

void
slinear16_to_alaw(v, p, cc)
	void *v;
	u_char *p;
	int cc;
{
	u_char *q = p;

	while (--cc >= 0) {
		p[HI] = lintoalaw[q[0] ^ 0x80];
		p[LO] = lintoalaw[q[1] ^ 0x80];
		++p;
		q += 2;
	}
}
