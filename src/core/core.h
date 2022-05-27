#pragma once

#include "log.h"
#include "math.h"
#include "types.h"

#include <stdlib.h>
#include <string.h>

void clear_memory(void* object, u64 size);
#define CLEAR_MEMORY(object) clear_memory(object, sizeof(*object))
#define CLEAR_MEMORY_ARRAY(object, size) clear_memory(object, sizeof(*object) * size)
