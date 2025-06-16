include(FetchContent)

find_package(OpenGL QUIET)
find_package(Vulkan QUIET)

#=================== ImGui ===================
set(imgui_fixes_and_config_patch_file ${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies/patches/imgui-fixes-and-config.patch)
set(imgui_apply_patch_command ${CMAKE_COMMAND} -Dpatch_file=${imgui_fixes_and_config_patch_file} -Dwith_reset=TRUE -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies/git-patch.cmake)

FetchContent_Declare(
    ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.9b-docking
    PATCH_COMMAND ${imgui_apply_patch_command}
)
FetchContent_MakeAvailable(ImGui)
list(APPEND ADDITIONAL_LIB_INCLUDES ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)

add_library(ImGui STATIC)
set_property(TARGET ImGui PROPERTY CXX_STANDARD 20)

target_sources(ImGui
    PRIVATE
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
)

target_sources(ImGui
    PRIVATE
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
)

if(Vulkan_FOUND)
    message("Vulkan found")
    target_sources(ImGui
            PRIVATE
            ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    )
    link_libraries(Vulkan::Vulkan)
endif()

target_include_directories(ImGui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends PRIVATE ${SDL2_INCLUDE_DIRS})

# ========= StormLib =============
if(NOT EXCLUDE_MPQ_SUPPORT)
    set(stormlib_patch_file ${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies/patches/stormlib-optimizations.patch)
    set(stormlib_apply_patch_command ${CMAKE_COMMAND} -Dpatch_file=${stormlib_patch_file} -Dwith_reset=TRUE -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies/git-patch.cmake)

    FetchContent_Declare(
        StormLib
        GIT_REPOSITORY https://github.com/ladislav-zezula/StormLib.git
        GIT_TAG v9.25
        PATCH_COMMAND ${stormlib_apply_patch_command}
    )
    FetchContent_MakeAvailable(StormLib)
    list(APPEND ADDITIONAL_LIB_INCLUDES ${stormlib_SOURCE_DIR}/src)
endif()

#=================== STB ===================
set(STB_DIR ${CMAKE_BINARY_DIR}/_deps/stb)
file(DOWNLOAD "https://github.com/nothings/stb/raw/0bc88af4de5fb022db643c2d8e549a0927749354/stb_image.h" "${STB_DIR}/stb_image.h")
file(WRITE "${STB_DIR}/stb_impl.c" "#define STB_IMAGE_IMPLEMENTATION\n#include \"stb_image.h\"")

add_library(stb STATIC)

target_sources(stb PRIVATE
    ${STB_DIR}/stb_image.h
    ${STB_DIR}/stb_impl.c
)

target_include_directories(stb PUBLIC ${STB_DIR})
list(APPEND ADDITIONAL_LIB_INCLUDES ${STB_DIR})

#=================== libgfxd ===================
if (GFX_DEBUG_DISASSEMBLER)
    FetchContent_Declare(
        libgfxd
        GIT_REPOSITORY https://github.com/glankk/libgfxd.git
        GIT_TAG 008f73dca8ebc9151b205959b17773a19c5bd0da
    )
    FetchContent_MakeAvailable(libgfxd)

    add_library(libgfxd STATIC)
    set_property(TARGET libgfxd PROPERTY C_STANDARD 11)

    target_sources(libgfxd PRIVATE
        ${libgfxd_SOURCE_DIR}/gbi.h
        ${libgfxd_SOURCE_DIR}/gfxd.h
        ${libgfxd_SOURCE_DIR}/priv.h
        ${libgfxd_SOURCE_DIR}/gfxd.c
        ${libgfxd_SOURCE_DIR}/uc.c
        ${libgfxd_SOURCE_DIR}/uc_f3d.c
        ${libgfxd_SOURCE_DIR}/uc_f3db.c
        ${libgfxd_SOURCE_DIR}/uc_f3dex.c
        ${libgfxd_SOURCE_DIR}/uc_f3dex2.c
        ${libgfxd_SOURCE_DIR}/uc_f3dexb.c
    )

    target_include_directories(libgfxd PUBLIC ${libgfxd_SOURCE_DIR})
endif()

#======== thread-pool ========
FetchContent_Declare(
    ThreadPool
    GIT_REPOSITORY https://github.com/bshoshany/thread-pool.git
    GIT_TAG v4.1.0
)
FetchContent_MakeAvailable(ThreadPool)

list(APPEND ADDITIONAL_LIB_INCLUDES ${threadpool_SOURCE_DIR}/include)
#=========== prism ===========
option(PRISM_STANDALONE "Build prism as a standalone library" OFF)
FetchContent_Declare(
    prism
    GIT_REPOSITORY https://github.com/KiritoDv/prism-processor.git
    GIT_TAG 0746117bfa3889469eb3a20f6cba7366ee856549
)
FetchContent_MakeAvailable(prism)

set(LLGL_BUILD_EXAMPLES OFF CACHE BOOL "Disable LLGL examples")
set(LLGL_BUILD_RENDERER_NULL OFF CACHE BOOL "Disable LLGL Null renderer")
set(LLGL_BUILD_RENDERER_OPENGL ON CACHE BOOL "Enable LLGL OpenGL renderer")
set(LLGL_GL_ENABLE_DSA_EXT ON CACHE BOOL "Enable OpenGL DSA extension")
set(LLGL_GL_ENABLE_VENDOR_EXT ON CACHE BOOL "Enable OpenGL vendor extensions")
set(LLGL_GL_INCLUDE_EXTERNAL ON CACHE BOOL "Include external OpenGL headers")
if (Vulkan_FOUND AND NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
set(LLGL_BUILD_RENDERER_VULKAN ON CACHE BOOL "Enable LLGL Vulkan renderer")
else()
set(LLGL_BUILD_RENDERER_VULKAN OFF CACHE BOOL "Disable LLGL Vulkan renderer")
endif()
set(LLGL_BUILD_STATIC_LIB ON CACHE BOOL "Build LLGL as a static library")

set(LLGL_OUTPUT_DIR ${CMAKE_BINARY_DIR} CACHE INTERNAL "LLGL output directory")
# Add the LLGL library
set(llgl_patch_file ${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies/patches/llgl.patch)

# Applies the patch or checks if it has already been applied successfully previously. Will error otherwise.
set(llgl_apply_patch_command ${CMAKE_COMMAND} -Dpatch_file=${llgl_patch_file} -Dwith_reset=TRUE -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies/git-patch.cmake)

FetchContent_Declare(
    llgl
    GIT_REPOSITORY https://github.com/coco875/LLGL.git
    GIT_TAG 0f23f55d9c0917bb0291cc748c72fa8185da97bc
    PATCH_COMMAND ${llgl_apply_patch_if_needed}
)
FetchContent_MakeAvailable(llgl)

if(LLGL_BUILD_RENDERER_NULL)
add_definitions(-DLLGL_BUILD_RENDERER_NULL=1)
target_link_libraries(LLGL LLGL_Null)
endif()

if(LLGL_BUILD_RENDERER_VULKAN)
add_definitions(-DLLGL_BUILD_RENDERER_VULKAN=1)
target_link_libraries(LLGL LLGL_Vulkan)
endif()

if(LLGL_BUILD_RENDERER_OPENGL)
add_definitions(-DLLGL_BUILD_RENDERER_OPENGL=1)
target_link_libraries(LLGL LLGL_OpenGL)
endif()

if(LLGL_BUILD_RENDERER_DIRECT3D11)
add_definitions(-DLLGL_BUILD_RENDERER_DIRECT3D11=1)
target_link_libraries(LLGL LLGL_Direct3D11)
endif()

if(LLGL_BUILD_RENDERER_DIRECT3D12)
add_definitions(-DLLGL_BUILD_RENDERER_DIRECT3D12=1)
target_link_libraries(LLGL LLGL_Direct3D12)
endif()

if(LLGL_BUILD_RENDERER_METAL)
add_definitions(-DLLGL_BUILD_RENDERER_METAL=1)
target_link_libraries(LLGL LLGL_Metal)
endif()

link_libraries(LLGL)

# include(cmake/FindVulkan.cmake)
set(SPIRV-Cross_DIR cmake/FIndPkgs)
# find_package(Vulkan REQUIRED SPIRV-Tools)
# find_package(Vulkan REQUIRED glslang)

find_package(glslang REQUIRED)

# link_libraries(Vulkan::SPIRV-Tools)
# link_libraries(Vulkan::glslang)
link_libraries(glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV glslang::SPVRemapper)

find_package(spirv_cross_core REQUIRED)
# if(LLGL_BUILD_RENDERER_OPENGL)
    find_package(spirv_cross_glsl REQUIRED)
    include_directories(${spirv_cross_glsl_SOURCE_DIR}/include)
    link_libraries(spirv-cross-glsl)
# endif()
# if(LLGL_BUILD_RENDERER_VULKAN OR LLGL_BUILD_RENDERER_DIRECT3D11 OR LLGL_BUILD_RENDERER_DIRECT3D12)
    find_package(spirv_cross_hlsl REQUIRED)
    include_directories(${spirv_cross_hlsl_SOURCE_DIR}/include)
    link_libraries(spirv-cross-hlsl)
# endif()
# if(LLGL_BUILD_RENDERER_METAL)
    find_package(spirv_cross_msl REQUIRED)
    include_directories(${spirv_cross_msl_SOURCE_DIR}/include)
    link_libraries(spirv-cross-msl)
# endif()
find_package(spirv_cross_cpp REQUIRED)

include_directories(${spirv_cross_core_SOURCE_DIR}/include)
include_directories(${spirv_cross_cpp_SOURCE_DIR}/include)

link_libraries(spirv-cross-core)
link_libraries(spirv-cross-cpp)

if(WIN32)
    set(EXTERNAL_INCLUDE_DIR "${llgl_SOURCE_DIR}/external")
    include_directories("${EXTERNAL_INCLUDE_DIR}/OpenGL/include")
endif()