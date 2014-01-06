#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lrc.h"
#include "list.h"

static int head_parse(const u8 *buff, int *offset)
{
	char *c;
	const char *p = (const char *)buff;
	char lyric_buff[DATALEN];

	c = strchr(p, ']');
	*c = '\0';

	c = strchr(p, ':');
	*c = '\0';

	if (!(strncmp(p, "ti", 2)))
		sprintf(lyric_buff, "song:%s", p + 3);

	if (!(strncmp(p, "ar", 2)))
		sprintf(lyric_buff, "singer:%s", p + 3);

	if (!(strncmp(p, "al", 2)))
		sprintf(lyric_buff, "album:%s", p + 3);

	if (!(strncmp(p, "by", 2)))
		sprintf(lyric_buff, "Manufacture:%s", p + 3);

	if (!(strncmp(p, "offset", 6))) {
		sprintf(lyric_buff, "%s", p + 7);
		*offset = atoi(lyric_buff);
		memset(lyric_buff, 0, DATALEN);
	}

	//show_text(char *text);  //fixme

	return 0;
}

static struct _ListNode *Transfer_lyric(const u8 *lrc_buff, size_t size, Node *first)
{
	LrcData data[DATALEN];
	LrcData *p_data = data;
	char lyric_buff[DATALEN];
	char *q = lyric_buff;
	int offset;
	float time[TIMECOUNT][TIME];
	const u8 *p = lrc_buff;

	while (*p) {
		int flag = 0;
		memset(time, 0, TIMECOUNT * TIME);
		q = lyric_buff;

		while (*p != '\n' && *p) {
			if (*p == '[') {
				p++;
				if (NISNUM(*p)) {
					head_parse(p, &offset);
					continue;
				} else {
					flag += 1;
					while (*p != ':')
						time[flag - 1][0] = time[flag - 1][0] * 10  + (*p++ - '0');

					while (*++p != '.')
						time[flag - 1][1] = time[flag - 1][1] * 10 + (*p - '0');

					while (*++p != ']')
						time[flag - 1][2] = time[flag - 1][2] * 10 + (*p - '0');
				}
			}

			if (*++p != '[' && *p != ' ') {
				while (*p) {
					if (*p == '\n')
						break;
					*q++ = *p++;
				}
				*q = '\0';

				while (flag--) {
					strcpy(p_data->str, lyric_buff);
					p_data->time = time[flag][0] * 60 + time[flag][1] + (time[flag][2] + offset) / 1000 ;
					first = InsertList(first, *p_data);
				}
			}
		}
		p++;
	}

	return first;
}

int show_lyric(const u8 *lrc_buff, size_t size, struct timeval *total, struct timeval *curr)
{
	Node *first = NULL;
	float timeData;
	char buff[512];
	static int flag = 0;

	timeData = curr->tv_sec + curr->tv_usec / 1000000.0;

	if (lrc_buff == NULL)
		return -1;

	if (flag == 0) {
		first = InitList(first);
		first = Transfer_lyric(lrc_buff, size, first);
		flag = 1;
	}

	if (flag == 1) {
		first = DeleteNode(first, timeData, buff);
		//show_text(char *text);  //fixme
	}

	if (fabs(timeData - (total->tv_sec + total->tv_usec / 1000000.0)) < 0.001)
		DestroyNode(first);

	return 0;
}
