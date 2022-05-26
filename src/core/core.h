#pragma once

#include "log.h"

#include <stdlib.h>
#include <string.h>

typedef char               i8;
typedef short              i16;
typedef int                i32;
typedef long long          i64;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;


void clear_memory(void* object, u64 size);
#define CLEAR_MEMORY(object) clear_memory(object, sizeof(*object))