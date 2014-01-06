#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fifo.h"

struct fifo *fifo_open()
{
	struct fifo *fifo;

	fifo = malloc(sizeof(*fifo));
	if (NULL == fifo) {
		perror("malloc");
		return NULL;
	}

	fifo->size = ARRAY_SIZE(fifo->data);
	fifo->read = 0;
	fifo->write = 0;
	fifo->used = 0;
	pthread_mutex_init(&fifo->mutex, NULL);

	return fifo;
}

int fifo_close(struct fifo *fifo)
{
	free(fifo);

	return 0;
}

int fifo_write(struct fifo *fifo, u8 *buff, size_t size)
{
	int i;
	size_t rest;

	while (size > 0) {
		pthread_mutex_lock(&fifo->mutex);
		rest = fifo->size - fifo->used;
		if (rest > size)
			rest = size;

		for (i = 0; i < rest; i++) {
			fifo->data[fifo->write] = buff[i];
			fifo->write++;
			fifo->write %= fifo->size;
		}

		fifo->used += rest;
		pthread_mutex_unlock(&fifo->mutex);

		size -= rest;
		buff += rest;

		sleep(1);
	}

	return size;
}

int fifo_read(struct fifo *fifo, u8 *buff, size_t size)
{
	int i;

	pthread_mutex_lock(&fifo->mutex);
	if (fifo->used < size)
		size = fifo->used;

	for (i = 0; i < size; i++) {
		buff[i] = fifo->data[fifo->read];
		fifo->read++;
		fifo->read %= fifo->size;
	}

	fifo->used -= size;
	pthread_mutex_unlock(&fifo->mutex);

	return size;
}
