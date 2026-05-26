cmake_minimum_required(VERSION 3.20)

project(gameengine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 REQUIRED CONFIG)

add_executable(gameengine
    src/main.cpp

    src/window/Window.cpp

    src/rendering/Renderer.cpp
    src/rendering/RendererBackendFactory.cpp
)

target_link_libraries(gameengine PRIVATE
    SDL3::SDL3
)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_sources(gameengine PRIVATE
        src/rendering/backends/metal/MetalRendererBackend.cpp
        src/rendering/backends/metal/MetalCppImplementation.cpp
    )

    target_include_directories(gameengine PRIVATE
        ${CMAKE_SOURCE_DIR}/external/metal-cpp
    )

    set(METAL_SHADER_SOURCE
        ${CMAKE_SOURCE_DIR}/src/rendering/backends/metal/shaders/Triangle.metal
    )

    set(METAL_SHADER_AIR
        ${CMAKE_BINARY_DIR}/shaders/Triangle.air
    )

    set(METAL_SHADER_LIB
        ${CMAKE_BINARY_DIR}/shaders/default.metallib
    )

    add_custom_command(
        OUTPUT ${METAL_SHADER_AIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/shaders
        COMMAND xcrun -sdk macosx metal
            -c ${METAL_SHADER_SOURCE}
            -o ${METAL_SHADER_AIR}
        DEPENDS ${METAL_SHADER_SOURCE}
        COMMENT "Compiling Metal shader to AIR"
    )

    add_custom_command(
        OUTPUT ${METAL_SHADER_LIB}
        COMMAND xcrun -sdk macosx metallib
            ${METAL_SHADER_AIR}
            -o ${METAL_SHADER_LIB}
        DEPENDS ${METAL_SHADER_AIR}
        COMMENT "Linking Metal shader library"
    )

    add_custom_target(gameengine_metal_shaders
        DEPENDS ${METAL_SHADER_LIB}
    )

    add_dependencies(gameengine gameengine_metal_shaders)

    target_compile_definitions(gameengine PRIVATE
        ANJEAN_PLATFORM_MACOS=1
        ANJEAN_RENDERER_METAL=1
        "ANJEAN_METAL_SHADER_LIBRARY=\"${METAL_SHADER_LIB}\""
    )

    target_link_libraries(gameengine PRIVATE
        "-framework Metal"
        "-framework QuartzCore"
        "-framework Foundation"
    )
else()
    target_sources(gameengine PRIVATE
        src/rendering/backends/vulkan/VulkanRendererBackend.cpp
    )

    target_compile_definitions(gameengine PRIVATE
        ANJEAN_RENDERER_VULKAN=1
    )

    find_package(Vulkan REQUIRED)

    target_link_libraries(gameengine PRIVATE
        Vulkan::Vulkan
    )
endif()