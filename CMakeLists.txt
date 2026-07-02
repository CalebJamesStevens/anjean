cmake_minimum_required(VERSION 3.20)

project(anjean LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------
# Renderer options
# -----------------------------

option(ANJEAN_USE_VULKAN "Build Anjean with the Vulkan renderer backend" ON)
option(ANJEAN_USE_METAL "Build Anjean with the Metal renderer backend" OFF)

if(NOT ANJEAN_USE_VULKAN AND NOT ANJEAN_USE_METAL)
    message(FATAL_ERROR "At least one renderer backend must be enabled.")
endif()

if(ANJEAN_USE_METAL AND NOT APPLE)
    message(FATAL_ERROR "ANJEAN_USE_METAL is only supported on Apple platforms.")
endif()

if(ANJEAN_USE_VULKAN AND ANJEAN_USE_METAL)
    message(WARNING "Both Vulkan and Metal are enabled. Make sure RendererBackendFactory supports selecting between both.")
endif()

# -----------------------------
# Dependencies
# -----------------------------

find_package(SDL3 REQUIRED CONFIG)
find_package(Stb REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)

find_path(TINYGLTF_INCLUDE_DIR tiny_gltf.h REQUIRED)

if(ANJEAN_USE_VULKAN)
    find_package(Vulkan REQUIRED)
    find_package(Ktx CONFIG REQUIRED)
endif()

# -----------------------------
# Helper for debug compile settings
# -----------------------------

function(anjean_apply_debug_settings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE
            $<$<CONFIG:Debug>:/Zi>
        )
    else()
        target_compile_options(${target_name} PRIVATE
            $<$<CONFIG:Debug>:-g>
            $<$<CONFIG:Debug>:-O0>
            $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
            # $<$<CONFIG:Debug>:-fsanitize=address>
        )

        target_link_options(${target_name} PRIVATE
            # $<$<CONFIG:Debug>:-fsanitize=address>
        )
    endif()
endfunction()

# -----------------------------
# Shared engine/runtime code
# -----------------------------

add_library(anjean_core STATIC
    src/runtime/ModelLoader.cpp
    src/runtime/RenderSystem.cpp
    src/runtime/Runtime.cpp
    src/runtime/objects/NativeMeshes/Mesh.cpp
    src/runtime/input/InputManager.cpp
    src/runtime/scripting/ScriptingEngine.cpp
)

set_target_properties(anjean_core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(anjean_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${TINYGLTF_INCLUDE_DIR}
)

target_link_libraries(anjean_core PUBLIC
    SDL3::SDL3
    glm::glm
)

target_link_libraries(anjean_core PRIVATE
    tinyobjloader::tinyobjloader
)

anjean_apply_debug_settings(anjean_core)

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

anjean_apply_debug_settings(anjean_native)

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
    src/rendering/RendererSelection.cpp
    src/rendering/Utilities.cpp

    src/vendor/stb_image.cpp
)

target_include_directories(anjean PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${TINYGLTF_INCLUDE_DIR}
    ${Stb_INCLUDE_DIR}
)

target_link_libraries(anjean PRIVATE
    anjean_core
    anjean_native
    glm::glm
)

anjean_apply_debug_settings(anjean)

# -----------------------------
# macOS / .NET host support
# -----------------------------

if(APPLE)
    set(ANJEAN_DOTNET_HOST_ARM64
        /usr/local/share/dotnet/packs/Microsoft.NETCore.App.Host.osx-arm64/8.0.1/runtimes/osx-arm64/native
    )

    set(ANJEAN_DOTNET_HOST_X64
        /usr/local/share/dotnet/x64/packs/Microsoft.NETCore.App.Host.osx-x64/8.0.1/runtimes/osx-x64/native
    )

    target_include_directories(anjean_core PUBLIC
        ${ANJEAN_DOTNET_HOST_ARM64}
        ${ANJEAN_DOTNET_HOST_X64}
    )

    target_link_libraries(anjean_core PUBLIC
        ${ANJEAN_DOTNET_HOST_ARM64}/libnethost.a
    )
endif()

# -----------------------------
# Metal backend
# -----------------------------

if(ANJEAN_USE_METAL)
    target_sources(anjean PRIVATE
        src/rendering/backends/metal/MetalRendererBackend.cpp
        src/rendering/backends/metal/MetalCppImplementation.cpp
        src/vendor/stb_image.cpp
    )

    target_include_directories(anjean PRIVATE
        ${CMAKE_SOURCE_DIR}/external/metal-cpp
        ${Stb_INCLUDE_DIR}
    )

    target_link_libraries(anjean PRIVATE
        "-framework Metal"
        "-framework QuartzCore"
        "-framework Foundation"
    )

    target_compile_definitions(anjean PRIVATE
        ANJEAN_PLATFORM_MACOS=1
        ANJEAN_RENDERER_METAL=1
    )

    set(METAL_SHADER_DIR ${CMAKE_BINARY_DIR}/shaders/metal)

    set(METAL_SHADER_SOURCE
        ${CMAKE_SOURCE_DIR}/src/rendering/backends/metal/shaders/Triangle.metal
        ${CMAKE_SOURCE_DIR}/src/rendering/backends/metal/shaders/Sprite2D.metal
        ${CMAKE_SOURCE_DIR}/src/rendering/backends/metal/shaders/SOTA.metal
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
        "ANJEAN_METAL_SHADER_LIBRARY=\"${METAL_SHADER_LIB}\""
    )
endif()

# -----------------------------
# Vulkan backend
# -----------------------------

if(ANJEAN_USE_VULKAN)
    target_sources(anjean PRIVATE
        src/rendering/backends/vulkan/VulkanRendererBackend.cpp
    )

    target_link_libraries(anjean PRIVATE
        Vulkan::Vulkan
        KTX::ktx
    )

    target_compile_definitions(anjean PRIVATE
        ANJEAN_RENDERER_VULKAN=1
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    )

    if(APPLE)
        set_target_properties(anjean PROPERTIES
            BUILD_RPATH "$ENV{VULKAN_SDK}/lib"
        )
    endif()

    # -----------------------------
    # Compile Slang shader to SPIR-V
    # -----------------------------

    find_program(SLANGC_EXECUTABLE slangc
        HINTS "$ENV{VULKAN_SDK}/bin"
        REQUIRED
    )

    set(VULKAN_SHADER_DIR ${CMAKE_BINARY_DIR}/shaders/vulkan)

    set(VULKAN_SHADER_SOURCE
        ${CMAKE_SOURCE_DIR}/src/rendering/backends/vulkan/shaders/shader.slang
    )

    set(VULKAN_SHADER_OUTPUT
        ${VULKAN_SHADER_DIR}/shader.spv
    )

    add_custom_command(
        OUTPUT ${VULKAN_SHADER_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${VULKAN_SHADER_DIR}
        COMMAND ${SLANGC_EXECUTABLE}
            ${VULKAN_SHADER_SOURCE}
            -target spirv
            -profile spirv_1_4
            -emit-spirv-directly
            -fvk-use-entrypoint-name
            -entry vertMain
            -entry fragMain
            -o ${VULKAN_SHADER_OUTPUT}
        DEPENDS ${VULKAN_SHADER_SOURCE}
        COMMENT "Compiling Vulkan Slang shader to SPIR-V"
        VERBATIM
    )

    add_custom_target(anjean_vulkan_shaders
        DEPENDS ${VULKAN_SHADER_OUTPUT}
    )

    add_dependencies(anjean anjean_vulkan_shaders)

    target_compile_definitions(anjean PRIVATE
        "ANJEAN_VULKAN_SHADER_PATH=\"${VULKAN_SHADER_OUTPUT}\""
    )
endif()
