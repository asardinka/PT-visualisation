cmake_minimum_required(VERSION 3.16)
project(PhasePortrait)

set(CMAKE_CXX_STANDARD 17)

# ON: один exe без sfml-*.dll (удобно переносить на другие ПК с Windows той же разрядности).
# Сборка тем же MinGW, что и для динамического SFML в libs/SFML.
option(PORTABLE_STATIC_SFML "Link SFML statically (no SFML DLLs)" OFF)
if(PORTABLE_STATIC_SFML)
    set(SFML_STATIC_LIBRARIES ON)
endif()

# SFML
set(SFML_DIR ${CMAKE_SOURCE_DIR}/libs/SFML/lib/cmake/SFML)
find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

# ImGui источники
set(IMGUI_SOURCES
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui-sfml/imgui-SFML.cpp
)

add_executable(PhasePortrait
    main.cpp
    src/odeSystem.cpp
    ${IMGUI_SOURCES}
)

target_include_directories(PhasePortrait PRIVATE
    libs/imgui
    libs/imgui-sfml
    libs/Eigen
    libs/
    libs/SFML/include
)

target_compile_definitions(PhasePortrait PRIVATE
    exprtk_disable_string_capabilities
    exprtk_disable_rtl_io_file
    exprtk_disable_rtl_vecops
    exprtk_disable_caseinsensitivity
)

target_compile_options(PhasePortrait PRIVATE -O2 -ffunction-sections -fdata-sections)
# Только odeSystem.cpp тянет exprtk (огромный .obj); big-obj не нужен для main/imgui
if(MINGW OR (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    set_source_files_properties(src/odeSystem.cpp PROPERTIES COMPILE_OPTIONS "-Wa,-mbig-obj")
endif()
target_link_options(PhasePortrait PRIVATE -Wl,--gc-sections)

target_link_libraries(PhasePortrait PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    opengl32
)

target_link_options(PhasePortrait PRIVATE
    -static-libgcc
    -static-libstdc++
)
# SFML DLL для динамической сборки копирует run-debug.ps1 в build\Debug (как раньше run.ps1).