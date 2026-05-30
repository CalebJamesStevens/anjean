cmake_minimum_required(VERSION 3.20)

project(anjean LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 REQUIRED CONFIG)

add_executable(anjean
    src/main.cpp

    src/window/Window.cpp

    src/rendering/Renderer.cpp
    src/rendering/RendererBackendFactory.cpp
)

target_link_libraries(anjean PRIVATE
    SDL3::SDL3
)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_sources(anjean PRIVATE
        src/rendering/backends/metal/MetalRendererBackend.cpp
        src/rendering/backends/metal/MetalCppImplementation.cpp
        src/vendor/stb_image.cpp
    )

    target_include_directories(anjean PRIVATE
        ${CMAKE_SOURCE_DIR}/external/metal-cpp
        ${CMAKE_SOURCE_DIR}/external/stb
    )

    set(METAL_SHADER_DIR ${CMAKE_BINARY_DIR}/shaders)

    set(METAL_SHADER_SOURCE
        ${CMAKE_SOURCE_DIR}/src/rendering/backends/metal/shaders/Triangle.metal
        ${CMAKE_SOURCE_DIR}/src/rendering/backends/metal/shaders/Sprite2D.metal
    )

    set(METAL_SHADER_AIR)

    foreach(METAL_FILE ${METAL_SHADER_SOURCE})
        get_filename_component(METAL_NAME ${METAL_FILE} NAME_WE)

        set(AIR_FILE ${METAL_SHADER_DIR}/${METAL_NAME}.air)

        add_custom_command(
            OUTPUT ${AIR_FILE}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${METAL_SHADER_DIR}
            COMMAND xcrun -sdk macosx metal
                -c ${METAL_FILE}
                -o ${AIR_FILE}
            DEPENDS ${METAL_FILE}
            COMMENT "Compiling ${METAL_NAME}.metal to AIR"
            VERBATIM
        )

        list(APPEND METAL_SHADER_AIR ${AIR_FILE})
    endforeach()

    set(METAL_SHADER_LIB ${METAL_SHADER_DIR}/default.metallib)

    add_custom_command(
        OUTPUT ${METAL_SHADER_LIB}
        COMMAND xcrun -sdk macosx metallib
            ${METAL_SHADER_AIR}
            -o ${METAL_SHADER_LIB}
        DEPENDS ${METAL_SHADER_AIR}
        COMMENT "Linking Metal shader library"
        VERBATIM
    )

    add_custom_target(anjean_metal_shaders
        DEPENDS ${METAL_SHADER_LIB}
    )

    add_dependencies(anjean anjean_metal_shaders)

    target_compile_definitions(anjean PRIVATE
        ANJEAN_PLATFORM_MACOS=1
        ANJEAN_RENDERER_METAL=1
        "ANJEAN_METAL_SHADER_LIBRARY=\"${METAL_SHADER_LIB}\""
    )

    target_link_libraries(anjean PRIVATE
        "-framework Metal"
        "-framework QuartzCore"
        "-framework Foundation"
    )
else()
    target_sources(anjean PRIVATE
        src/rendering/backends/vulkan/VulkanRendererBackend.cpp
    )

    target_compile_definitions(anjean PRIVATE
        ANJEAN_RENDERER_VULKAN=1
    )

    find_package(Vulkan REQUIRED)

    target_link_libraries(anjean PRIVATE
        Vulkan::Vulkan
    )
endif()