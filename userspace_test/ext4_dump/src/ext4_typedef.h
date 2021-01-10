#ifndef __TYPEDEF_H__
#define __TYPEDEF_H__

#include <stdint.h>
#ifndef __bitwise
//#define __bitwise     __attribute__((bitwise))
#define __bitwise
#endif

typedef unsigned long __u64;
typedef unsigned int  __u32;
typedef unsigned short int __u16;
typedef short int __s16;
typedef unsigned char __u8;

typedef __u16   __bitwise   __le16;
typedef __u32   __bitwise   __le32;
typedef __u64   __bitwise   __le64;
typedef __u16   __bitwise   __be16;
typedef __u32   __bitwise   __be32;
typedef __u64   __bitwise   __be64;

#endif

