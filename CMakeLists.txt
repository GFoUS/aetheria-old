cmake_minimum_required (VERSION 3.8)

project ("aetheria")

find_package(Vulkan REQUIRED)

add_subdirectory(vendor/glfw)

file(GLOB SRC
    "src/**/**/*.c"
    "src/**/*.c"
    "src/*.c"
)

include_directories(src)

add_executable(aetheria ${SRC})
target_link_libraries(aetheria Vulkan::Vulkan glfw)
