cmake_minimum_required(VERSION 3.22.1)
project(SplitSecondVelocityNative LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(SSV_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)

include_directories(
    "${SSV_ROOT}/include"
    "${SSV_ROOT}/src/core"
    "${SSV_ROOT}/src/assets"
    "${SSV_ROOT}/src/render"
    "${SSV_ROOT}/src/render/gles3"
    "${ANDROID_NDK}/sources/android/native_app_glue"
)

set(SSV_SOURCES
    "android_main.cpp"
    "${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c"
    "${SSV_ROOT}/src/platform/platform_android.cpp"
    "${SSV_ROOT}/src/render/gles3/gles3_backend.cpp"
    "${SSV_ROOT}/src/render/vehicle_renderer.cpp"
    "${SSV_ROOT}/src/render/gl_platform.cpp"
    "${SSV_ROOT}/src/core/vehicle_physics.cpp"
    "${SSV_ROOT}/src/core/vehicle_database.cpp"
    "${SSV_ROOT}/src/core/camera_rig.cpp"
    "${SSV_ROOT}/src/assets/ark_parser.cpp"
    "${SSV_ROOT}/src/assets/lzss_decomp.cpp"
    "${SSV_ROOT}/src/core/game_engine.cpp"
    "${SSV_ROOT}/src/core/state_machine.cpp"
)

add_library(ssv SHARED ${SSV_SOURCES})

target_compile_definitions(ssv PRIVATE
    __ANDROID__
    GL_GLEXT_PROTOTYPES
)

target_compile_options(ssv PRIVATE
    -O3
    -flto
    -ffast-math
    -funsafe-math-optimizations
    -fomit-frame-pointer
    -fvisibility=hidden
)

target_link_libraries(ssv PRIVATE
    android
    log
    EGL
    GLESv3
    m
)
