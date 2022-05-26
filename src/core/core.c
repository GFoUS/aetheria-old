#include "core.h"

void clear_memory(void* object, u64 size) {
	memset(object, 0, size);
}