#include "lock.h"
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

// Linked list node
typedef struct Chunk {
	size_t size;
	struct Chunk *next; // pointer to next node
	bool freed; // keeps track of freed chunks
	int temp_debug; // TODO remove after debugging
} Chunk;

#define CHUNK_SIZE sizeof(Chunk)

void *head = NULL; // Link list head

// Find available memory in the heap
Chunk *find_free_chunk(Chunk **last, size_t size)
{
	Chunk *current = head;
	while (current && !(current->freed && current->size >= size)) {
		*last = current;
		current = current->next;
	}

	return current;
}

// Allocate size bytes of uninitialized storage (melloc)
void *lock(size_t size)
{
	//TODO
	return NULL;
}

// Deallocates the space previously allocated (free)
void za_hando(void *ptr)
{
	//TODO
}

// Allocates memory for an array of num objects of size and initializes all
// bytes in the allocated storage to zero.
void *clock(size_t count, size_t size)
{
	//TODO
	return NULL;
}
