#ifndef _YSD_RTDB_INCLUDE_H_
#define _YSD_RTDB_INCLUDE_H_

#ifdef WIN32
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //getcwd
#include <string>
#include <string.h>
#include <sys/time.h>
#include <memory.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/shm.h>
#endif
#include <map>
#include <list>
#include <time.h> 

#ifndef INT64
#	ifdef WIN32
#		define INT64 __int64
#		define UINT64 unsigned __int64
#	else
#		define INT64 long long
#		define UINT64 unsigned long long
#	endif
#endif

#ifndef BYTE
#define BYTE unsigned char
#endif

#ifndef WORD
#define WORD unsigned short
#endif

#ifndef UINT32
#define UINT32 unsigned int
#endif

#define LEN_8		8
#define LEN_16		16
#define LEN_32		32
#define LEN_64		64
#define LEN_128		128
#define LEN_256		256

#define LEN_VAL		32
#define MAX_SECTOR	33

typedef enum //实时库打开方式
{
	RTDB_OPEN_RO = 1,//只读方式
	RTDB_OPEN_RW = 2,//读写方式
}RtdbOpenType;

#endif