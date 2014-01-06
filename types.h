#pragma once
#include <stdio.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define DPRINT(fmt, arg...)  \
	do {\
		printf("%s(), line  = %d: ", __func__, __LINE__);\
		printf(fmt, ##arg);\
	} while (0)
