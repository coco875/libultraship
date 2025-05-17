include(FetchContent)

find_package(OpenGL QUIET)
find_package(Vulkan QUIET)

#=================== ImGui ===================
set(imgui_fixes_and_config_patch_file ${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies/patches/imgui-fixes-and-config.patch)
set(imgui_apply_patch_command ${CMAKE_COMMAND} -Dpatch_file=${imgui_fixes_and_config_patch_file} -Dwith_reset=TRUE -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies/git-patch.cmake)

FetchContent_Declare(
    ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.6-docking
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
    GIT_TAG 493974843e910d0fac4e3bb1ec52656728b875b4
)
FetchContent_MakeAvailable(prism)

# Add the LLGL library
set(LLGL_BUILD_EXAMPLES OFF)
set(LLGL_BUILD_RENDERER_NULL OFF)
set(LLGL_BUILD_RENDERER_OPENGL ON)
set(LLGL_GL_ENABLE_DSA_EXT ON)
set(LLGL_GL_ENABLE_VENDOR_EXT ON)
set(LLGL_GL_INCLUDE_EXTERNAL ON)
if (Vulkan_FOUND)
    set(LLGL_BUILD_RENDERER_VULKAN ON)
else()
    set(LLGL_BUILD_RENDERER_VULKAN OFF)
endif()
set(LLGL_BUILD_STATIC_LIB ON)

set(LLGL_OUTPUT_DIR ${CMAKE_BINARY_DIR})

set(llgl_patch_file ${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies/patches/llgl.patch)

# Applies the patch or checks if it has already been applied successfully previously. Will error otherwise.
set(llgl_apply_patch_if_needed git apply ${llgl_patch_file} ${git_hide_output} || git apply --reverse --check ${llgl_patch_file})

FetchContent_Declare(
        llgl
        GIT_REPOSITORY https://github.com/LukasBanana/LLGL.git
        GIT_TAG 310e6b9f6c173ac2dac11b765e0fd0a7f66f1286
        PATCH_COMMAND ${llgl_apply_patch_if_needed}
)
FetchContent_MakeAvailable(llgl)

if(LLGL_BUILD_RENDERER_VULKAN)
    link_libraries(LLGL_Vulkan)
endif()

if(LLGL_BUILD_RENDERER_OPENGL)
    link_libraries(LLGL_OpenGL)
endif()

if(LLGL_BUILD_RENDERER_DIRECT3D11)
    link_libraries(LLGL_Direct3D11)
endif()

if(LLGL_BUILD_RENDERER_DIRECT3D12)
    link_libraries(LLGL_Direct3D12)
endif()

if(LLGL_BUILD_RENDERER_METAL)
    link_libraries(LLGL_Metal)
endif()

link_libraries(LLGL)

include_directories(${llgl_SOURCE_DIR})


include(cmake/FindVulkan.cmake)
set(SPIRV-Cross_DIR cmake/FIndPkgs)
find_package(Vulkan REQUIRED SPIRV-Tools)
find_package(Vulkan REQUIRED glslang)

link_libraries(Vulkan::SPIRV-Tools)
link_libraries(Vulkan::glslang)

find_package(SPIRV-Cross REQUIRED)
link_libraries(
        SPIRV-Cross::spirv-cross-core
        SPIRV-Cross::spirv-cross-glsl # or others as needed
        SPIRV-Cross::spirv-cross-msl # or others as needed
        SPIRV-Cross::spirv-cross-hlsl
)

if(WIN32)
    set(EXTERNAL_INCLUDE_DIR "${llgl_SOURCE_DIR}/external")
    include_directories("${EXTERNAL_INCLUDE_DIR}/OpenGL/include")
endif()