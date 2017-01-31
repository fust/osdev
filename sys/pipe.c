#include "sys/pipe.h"
#include "stdint.h"
#include "mem/kmalloc.h"
#include "string.h"
#include "debug.h"
#include "fs/vfs.h"
#include "spinlock.h"

spinlock_t pipe_slock;

pipe_t *pipe_create(uint32_t length)
{
	pipe_t *pipe = (pipe_t *)kmalloc(sizeof(pipe_t));
	pipe->buffer = (uint8_t*)kmalloc(length);
	memset(pipe->buffer, 0, sizeof(uint8_t) * length);
	pipe->max_length = length;

	debug("PIPE: Created new pipe. Location: 0x%x, length: %d\n", pipe, length);

	return pipe;
}

int pipe_push(pipe_t *pipe, uint32_t size, uint8_t *data)
{
	int next = pipe->head + 1;

	if (size > pipe->max_length - 1) { // Too large
		return -1;
	}
	// @todo: Add some more checks for overwrites, etc.

	size_t written = 0;
	spin_lock(&pipe_slock);
	while (written < size) {
		pipe->buffer[next] = data[written];
		next++;
		if (next >= pipe->max_length) {
			next = 0;
		}
		pipe->head = next;
		written++;
	}
	spin_unlock(&pipe_slock);

	return 0;
}

int pipe_pop(pipe_t *pipe, uint32_t size, uint8_t *data)
{
	if (pipe->head == pipe->tail) { // Buffer is empty
		return -1;
	}

	uint32_t next = pipe->tail + 1;

	size_t read = 0;
	spin_lock(&pipe_slock);
	while (read < size) {
		data[read] = pipe->buffer[pipe->tail];
		next++;
		if (next >= pipe->max_length) {
			next = 0;
		}
		pipe->tail = next;
		read++;
	}
	spin_unlock(&pipe_slock);

	return 0;
}

vfs_node_t *pipe_device_create(uint32_t length)
{
	vfs_node_t *node = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
	memset(node, 0, sizeof(vfs_node_t));

	node->device = NULL;
	node->mask = VFS_MASK_DEVICE;
	node->read = pipe_read;
	node->write = pipe_write;

	pipe_t *pipe = pipe_create(length);

	if (!pipe) {
		debug("PIPE: Couldn't create a new pipe!\n");
	}

	node->device = pipe;

	return node;
}

int pipe_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *data)
{
	if (!(node->mask & VFS_MASK_DEVICE)) {
		PANIC("Tried to read from a non-device node.\n");
	}

	return pipe_pop((pipe_t *)node->device, size, data);
}

int pipe_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *data)
{
	if (!(node->mask & VFS_MASK_DEVICE)) {
		PANIC("Tried to read from a non-device node.\n");
	}

	return pipe_push((pipe_t *)node->device, size, data);
}
