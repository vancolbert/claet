/****************************************************************************
*            memory.c
*
* Author: 2011  Daniel Jungmann <el.3d.source@googlemail.com>
* Copyright: See COPYING file that comes with this distribution
****************************************************************************/
#include "memory.h"
#include "elloggingwrapper.h"
void *malloc_aligned(const Uint64 size, const Uint64 alignment) {
	void *result;
	result = malloc(size);
	LOG_DEBUG_VERBOSE("size: %d, alignment: %d, memory: %p", size, alignment, result);
	return result;
}
void free_aligned(void *memory) {
	LOG_DEBUG_VERBOSE("memory: %p", memory);
	free(memory);
}
