#include "lock.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

// Linked list node
typedef struct Chunk {
	size_t size;
	struct Chunk *next; // pointer to next node
	struct Chunk *prev; // pointer to prev node
	bool freed; // keeps track of freed chunks
	int temp_debug; // TODO remove after debugging
} Chunk;

#define CHUNK_SIZE sizeof(Chunk)

void *head = NULL; // Link list head

// Find available memory in the heap
static Chunk *find_free_chunk(Chunk **last, size_t size)
{
	Chunk *current = head;
	while (current && !(current->freed && current->size >= size)) {
		*last = current;
		current = current->next;
	}

	return current;
}

static Chunk *request_space(Chunk *last, size_t size)
{
	Chunk *chunk;
	chunk = sbrk(0);
	void *request = sbrk(size + CHUNK_SIZE);

	assert((void *)chunk == request);
	if (request == (void *)-1) {
		return NULL; // sbrk failed.
	}

	if (last) { // NULL on first request.
		last->next = chunk;
		chunk->prev = last;
	}
	chunk->size = size;
	chunk->next = NULL;
	chunk->freed = false;
	chunk->temp_debug = 0x12345678;
	return chunk;
}

static Chunk *get_block_ptr(void *ptr)
{
	return (Chunk *)ptr - 1;
}

// Merge fragmented chunks
static void merge_freed_chunks(Chunk *chunk)
{
	assert(chunk->freed);

	if (chunk->prev != NULL && chunk->next != NULL) {
		if (chunk->prev->freed && chunk->next->freed) {
			chunk->prev->next = chunk->next;
			return;
		}
	} else if (chunk->prev != NULL && chunk->prev->freed) {
		chunk->prev->next = chunk;
		return;
	} else if (chunk->next != NULL && chunk->next->freed) {
		chunk->next = chunk->next->next;
		return;
	}
}

// Allocate size bytes of uninitialized storage (melloc)
void *lock(size_t size)
{
	Chunk *chunk;
	// TODO: align size

	if (size <= 0) {
		return NULL;
	}

	if (!head) { // First call.
		chunk = request_space(NULL, size);
		if (!chunk) {
			return NULL;
		}
		head = chunk;
	} else {
		Chunk *last = head;
		chunk = find_free_chunk(&last, size);
		if (!chunk) { // Failed to find free block.
			chunk = request_space(last, size);
			if (!chunk) {
				return NULL;
			}
		} else { // Found free block
			// TODO: split block here.
			chunk->freed = false;
			chunk->temp_debug = 0x77777777;
		}
	}

	return (chunk + 1);
}

// Deallocates the space previously allocated (free)
void za_hando(void *ptr)
{
	if (!ptr) {
		return;
	}

	Chunk *chunk_ptr = get_block_ptr(ptr);
	assert(chunk_ptr->freed == false);
	assert(chunk_ptr->temp_debug == 0x77777777 ||
	       chunk_ptr->temp_debug == 0x12345678);

	chunk_ptr->freed = true;
	chunk_ptr->temp_debug = 0x55555555;

	// Merge chunks freed chunks
	merge_freed_chunks(chunk_ptr);
}

// Reallocates the given area of memory.
void *relock(void *ptr, size_t size)
{
	if (!ptr) {
		// NULL ptr. relock should act like lock.
		return lock(size);
	}

	Chunk *chunk = get_block_ptr(ptr);
	if (chunk->size >= size) {
		// TODO free some space once split.
		return ptr;
	}

	// allocate new space and free old space.
	// Then copy old data to new space.
	void *new_ptr;
	new_ptr = lock(size);
	if (!new_ptr) {
		errno = ECANCELED;
		return NULL;
	}
	memcpy(new_ptr, ptr, chunk->size);
	za_hando(ptr);
	return new_ptr;
}

// Allocates memory for an array of num objects of size and initializes all
// bytes in the allocated storage to zero.
void *clock(size_t count, size_t size)
{
	size_t sum = count * size; // TODO check for overflow.
	void *ptr = lock(sum);
	memset(ptr, 0, sum);
	return ptr;
}
