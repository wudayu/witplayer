#pragma once
#include <sys/time.h>
#include "types.h"
#include "list.h"

#define LEN 1024
#define DATALEN 128
#define TIMECOUNT 20
#define TIME 3

#define NISNUM(a) (((a) >= '0' && (a) <= '9') ? 1 : 0)

int show_lyric(const u8 *lrc_buff, size_t size, struct timeval *total, struct timeval *curr);
