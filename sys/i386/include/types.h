/*
 * types.h
 *
 *  Created on: 17 Jan 2020
 *      Author: marti
 */

#ifndef	_MACHTYPES_H_
#define	_MACHTYPES_H_

typedef struct _physadr {
	int r[1];
} *physadr;

typedef struct label_t {
	int val[6];
} label_t;

typedef	unsigned long	vm_offset_t;
typedef	unsigned long	vm_size_t;

/*
 * Basic integral types.  Omit the typedef if
 * not possible for a machine/compiler combination.
 */
typedef	__signed char		int8_t;
typedef	unsigned char		u_int8_t;
typedef	short			  	int16_t;
typedef	unsigned short		u_int16_t;
typedef	int			  		int32_t;
typedef	unsigned int		u_int32_t;
typedef	long long		  	int64_t;
typedef	unsigned long long	u_int64_t;

#endif /* _MACHTYPES_H_ */
