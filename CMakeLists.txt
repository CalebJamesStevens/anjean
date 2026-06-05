cmake_minimum_required(VERSION 3.20)

project(anjean LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 REQUIRED CONFIG)

# -----------------------------
# Shared engine/runtime code
# -----------------------------

add_library(anjean_core STATIC
    src/runtime/Runtime.cpp
    src/runtime/input/InputManager.cpp
    src/runtime/scripting/ScriptingEngine.cpp
)

set_target_properties(anjean_core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(anjean_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(anjean_core PUBLIC
    SDL3::SDL3
)

# -----------------------------
# Native C API for C# P/Invoke
# Produces: libAnjean.Native.dylib
# -----------------------------

add_library(anjean_native SHARED
    src/runtime/scripting/NativeBindings.cpp
)

set_target_properties(anjean_native PROPERTIES
    OUTPUT_NAME "Anjean.Native"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_include_directories(anjean_native PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(anjean_native PRIVATE
    anjean_core
)

# -----------------------------
# Main executable
# -----------------------------

add_executable(anjean)

target_sources(anjean PRIVATE
    src/main.cpp
    src/init.cpp

    src/window/Window.cpp
    src/Orchestrator/Orchestrator.cpp

    src/rendering/Renderer.cpp
    src/rendering/RendererBackendFactory.cpp
    src/rendering/Utilities.cpp
)

target_include_directories(anjean PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(anjean PRIVATE
    anjean_core
    anjean_native
)

target_compile_options(anjean PRIVATE
    -g
    -O0
    -fsanitize=address
    -fno-omit-frame-pointer
)

target_link_options(anjean PRIVATE
    -fsanitize=address
)

target_compile_options(anjean_core PRIVATE
    -g
    -O0
    -fsanitize=address
    -fno-omit-frame-pointer
)

target_link_options(anjean_core PRIVATE
    -fsanitize=address
)

target_compile_options(anjean_native PRIVATE
    -g
    -O0
    -fsanitize=address
    -fno-omit-frame-pointer
)

target_link_options(anjean_native PRIVATE
    -fsanitize=address
)

# -----------------------------
# macOS / Metal backend
# -----------------------------

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_sources(anjean PRIVATE
        src/rendering/backends/metal/MetalRendererBackend.cpp
        src/rendering/backends/metal/MetalCppImplementation.cpp
        src/vendor/stb_image.cpp
    )

    target_include_directories(anjean PRIVATE
        /usr/local/share/dotnet/packs/Microsoft.NETCore.App.Host.osx-arm64/8.0.1/runtimes/osx-arm64/native
        /usr/local/share/dotnet/x64/packs/Microsoft.NETCore.App.Host.osx-x64/8.0.1/runtimes/osx-x64/native
        ${CMAKE_SOURCE_DIR}/external/metal-cpp
        ${CMAKE_SOURCE_DIR}/external/stb
    )

    target_include_directories(anjean_core PUBLIC
        /usr/local/share/dotnet/packs/Microsoft.NETCore.App.Host.osx-arm64/8.0.1/runtimes/osx-arm64/native
        /usr/local/share/dotnet/x64/packs/Microsoft.NETCore.App.Host.osx-x64/8.0.1/runtimes/osx-x64/native
    )

    target_link_libraries(anjean PRIVATE
        "-framework Metal"
        "-framework QuartzCore"
        "-framework Foundation"
    )

    target_link_libraries(anjean_core PUBLIC
        /usr/local/share/dotnet/packs/Microsoft.NETCore.App.Host.osx-arm64/8.0.1/runtimes/osx-arm64/native/libnethost.a
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