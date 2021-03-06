﻿cmake_minimum_required (VERSION 3.8)

project ("aetheria")

find_package(Vulkan REQUIRED)

add_subdirectory(vendor/glfw)
add_subdirectory(vendor/cglm)

file(GLOB SRC
    "src/**/**/*.c"
    "src/**/*.c"
    "src/*.c"
)

include_directories(src)
include_directories(vendor/VulkanMemoryAllocator/include)
include_directories(vendor/stb)
include_directories(vendor/cJSON)

add_executable(aetheria 
                ${SRC}
                "src/vendor/vma.cpp"
                "vendor/cJSON/cJSON.c"
)
target_link_libraries(aetheria Vulkan::Vulkan glfw cglm)
