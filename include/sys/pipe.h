#ifndef __PIPE_H
#define __PIPE_H

#include "stdint.h"
#include "fs/vfs.h"

typedef struct pipe {
	uint8_t *buffer;
	uint32_t head;
	uint32_t tail;
	uint32_t max_length;
} pipe_t;

pipe_t *pipe_create(uint32_t length);
int pipe_push(pipe_t *pipe, uint32_t size, uint8_t *data);
int pipe_pop(pipe_t *pipe, uint32_t size, uint8_t *data);

vfs_node_t *pipe_device_create(uint32_t length);

int pipe_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *data);
int pipe_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *data);

#endif
