#include "math.h"

u32 clamp(u32 value, u32 min, u32 max) {
    if (value < min) return min;
    if (value > max) return max;
    else return value;
}