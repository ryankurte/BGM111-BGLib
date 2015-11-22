/*
 * apitypes.h
 *
 *  Created on: Nov 27, 2009
 *      Author: jere
 */

#ifndef APITYPES_H_
#define APITYPES_H_

#include <stdint.h>

typedef uint8_t  uint8;
typedef int8_t   int8;
typedef uint16_t uint16;
typedef int16_t   int16;
typedef uint32_t  uint32;
typedef int32_t    int32;

typedef struct bd_addr_t
{
    unsigned char addr[6];

}bd_addr;

typedef bd_addr hw_addr;
typedef struct
{
    unsigned char len;
    unsigned char data[];
}uint8array;
typedef struct
{
    unsigned short len;
    unsigned char data[];
}uint16array;

typedef struct
{
    unsigned char len;
    signed char data[];
}string;

typedef union
{
        uint32_t u;
        uint8_t  a[4];
}ipv4;
#endif /* APITYPES_H_ */
