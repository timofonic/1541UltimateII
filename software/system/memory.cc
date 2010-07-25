#include <stdlib.h>
#include <string.h>
#include "small_printf.h"
#include "memory.h"
extern "C" {
    #include "dump_hex.h"
}
#define USE_MEM_TRACE 0
#define MEM_VERBOSE 1

MemManager :: MemManager()
{
	for(int i=0;i<NUM_MALLOCS;i++) {
		mem_locations[i] = 0;
		mem_lengths[i] = 0;
	}
	total_alloc = 0;
	last_alloc = 0;
}

MemManager :: ~MemManager()
{
	printf("\nTotal leak: %d bytes.", total_alloc);
	for(int i=0;i<NUM_MALLOCS;i++) {
		if(mem_lengths[i]) {
    		printf("NF %4d: Size: %7d  Loc: %p. Total = %d\n", i, mem_lengths[i], mem_locations[i], total_alloc);
    		dump_hex(mem_locations[i], mem_lengths[i]);
    		total_alloc -= mem_lengths[i];
		}
	}
}

void *MemManager :: qalloc(size_t size)
{
    void *p = malloc(size);
    if(!p) {
    	printf("alloc failed.\n");
    	while(1)
    		;
    }
    memset(p, 0xCC, size);

/*
    for(int i=0;i<NUM_MALLOCS;i++) {
    	if(!mem_lengths[i]) {
    		mem_lengths[i] = size;
    		mem_locations[i] = p;
    		total_alloc += size;
#if MEM_VERBOSE > 0
    		printf("++ %4d: Size: %7d  Loc: %p. Total = %d\n", i, size, p, total_alloc);
#endif
    		return p;
    	}
    }
    printf("No space in the alloc list..\n");
    while(1)
    	;
*/
    if(last_alloc < NUM_MALLOCS) {
    	mem_lengths[last_alloc] = size;
    	mem_locations[last_alloc] = p;
    	total_alloc += size;
#if MEM_VERBOSE > 0
		printf("++ %4d: Size: %7d  Loc: %p. Total = %d\n", last_alloc, size, p, total_alloc);
#endif
		last_alloc ++;
		return p;
    }

    printf("No space in the alloc list..\n");
	while(1);
}

void MemManager :: qfree(void *p)
{
	for(int i=0;i<NUM_MALLOCS;i++) {
		if(mem_locations[i] == p) {
			total_alloc -= mem_lengths[i];
#if MEM_VERBOSE > 0
    		printf("-- %4d: Size: %7d  Loc: %p. Total = %d\n", i, mem_lengths[i], p, total_alloc);
#endif
    		memset(p, 0xBB, mem_lengths[i]);
			mem_lengths[i] = 0;
			mem_locations[i] = 0;
			free(p);
			return;
		}
	}
	printf("Trying to free a location that is not in the alloc list: %p\n", p);
	while(1)
		;
}

#if USE_MEM_TRACE > 0
MemManager mem_manager;
#endif

void * operator new(size_t size)
{
#if USE_MEM_TRACE == 1
	return mem_manager.qalloc(size);
#else
	return malloc(size);
#endif
}

void operator delete(void *p)
{
#if USE_MEM_TRACE == 1
	mem_manager.qfree(p);
#else
	free(p);
#endif
}

void * operator new[](size_t size)
{
#if USE_MEM_TRACE == 1
	return mem_manager.qalloc(size);
#else
	return malloc(size);
#endif
}

void operator delete[](void *p)
{
#if USE_MEM_TRACE == 1
	mem_manager.qfree(p);
#else
	free(p);
#endif
}
