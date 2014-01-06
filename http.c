#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include "types.h"
#include "http.h"
#include "sound_file.h"

struct sound_file_info *http_file_open(const char *URL)
{
	// fixme!

	return 0;
}

int http_file_close(struct sound_file_info *file)
{
	// fixme!
	return 0;
}

int get_http_head(int fd, char *buff, size_t buff_size)
{
	int ret;
	int size;

	size = 4;
	ret = recv(fd, buff, size, 0);
	if (ret < 0) {
		perror("recv");
		return -1;
	}

	while (memcmp(buff + size - 4, "\r\n\r\n", 4) && size < buff_size)
	{
		ret = recv(fd, buff + size, 1, 0);
		size++;
	}

	buff[size] = '\0';

	return size;
}

int read_timeouts(int fd, int sec)
{
	fd_set rfds;
	struct timeval timeout;

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	timeout.tv_sec = sec;
	timeout.tv_usec = 0;

	return select(fd + 1, &rfds, NULL, NULL, &timeout);
}

void *download(void *arg)
{
	struct download_arg *p;
	long used = 0;
	int len;
	int ret;
	int reconn = 0;
	char httpHeadBuff[HTTP_HEAD_LEN];
	char request[RQU_LEN];
	int down_fd;
	struct sockaddr_in server;

	down_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (down_fd < 0) {
		perror("socket");
		goto L1;
	}

	p = arg;

	server.sin_family = AF_INET;
	server.sin_port = htons(SERVER_PORT);
	inet_aton(p->ip, &server.sin_addr);
	ret = connect(down_fd, (struct sockaddr *)&server, sizeof(server));
	if (ret < 0) {
		perror("connect()");
		goto L1;
	}

	len = sprintf(request,
				"GET %s HTTP/1.0\r\nRange:bytes=%ld-%ld\r\n\r\n",
				p->file->url,
				(long)p->offset,
				(long)p->offset + p->size - 1);

	ret = send(down_fd, request, len, 0);
	if (ret < 0) {
		perror("send");
		goto L1;
	}

	ret = get_http_head(down_fd, httpHeadBuff, HTTP_HEAD_LEN);
	if (ret < 0) {
		printf("get_http_head()");
		goto L1;
	}

	while (p->size > 0) {
		ret = read_timeouts(down_fd, SEC);
		if (ret < 0) {
			perror("select");
			goto L1;
		}

		if (ret == 0) {
			server.sin_family = AF_INET;
			server.sin_port = htons(SERVER_PORT);
			inet_aton(p->ip, &server.sin_addr);
			ret = connect(down_fd, (struct sockaddr *)&server,
							sizeof(server));
			if (ret < 0) {
				perror("connect");
				continue;
			}

			len = sprintf(request,
						"GET %s HTTP/1.0\r\nrange: bytes=%ld-%ld\r\n\r\n",
						p->file->url,
						(long)used + p->offset + 1,
						(long)p->offset + p->size - 1);

			ret = send(down_fd, request, len, 0);
			if (ret < 0) {
				perror("send");
				goto L1;
			}

			reconn = 1;
			continue;
		}

		if (reconn == 1) {
			get_http_head(down_fd, httpHeadBuff, HTTP_HEAD_LEN);
			reconn = 0;
			continue;
		}

		ret = recv(down_fd, p->buff + used, p->size, 0);
		if (ret < 0) {
			perror("recv_http_data()");
			goto L1;
		}

		p->size -= ret;
		used += ret;

		if (ret == 0)
			break;
	}

	p->ret_size = used;

L1:
	close(down_fd);

	return 0;
}

int http_file_load(struct sound_file_info *file, u8 *buff, size_t size)
{
	int i;
	int ret;
	int block_size;
	int sum;
	pthread_t pth_id[THREAD_NUM];
	char ip[INET_ADDRSTRLEN];
	const char *p;
	int total_ret_size;
	struct download_arg arg[THREAD_NUM];

	i = 0;
	p = file->url + 7;
	while (*p != '/')
		ip[i++] = *p++;
	ip[i] = '\0';

	sum = 0;
	block_size = size / THREAD_NUM;
	for (i = 0; i < THREAD_NUM; i++) {
		if (i == THREAD_NUM - 1)
			arg[i].size = size - sum;
		else
			arg[i].size = block_size;

		arg[i].ip = ip;
		arg[i].file = file;
		arg[i].buff = buff + block_size * i;
		arg[i].offset = i * block_size + file->offset;

		sum += block_size;
		ret = pthread_create(&pth_id[i], NULL, download, &arg[i]);
		if (ret < 0) {
			perror("pthread_create()");
			return ret;
		}
	}

	for (i = 0; i < THREAD_NUM; i++) {
		ret = pthread_join(pth_id[i], NULL);
		if (ret < 0) {
			perror("pthread_join()");
			return ret;
		}
		total_ret_size += arg[i].ret_size;
	}

	return total_ret_size;
}
