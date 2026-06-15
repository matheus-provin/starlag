# ============================================================================
#  third_party/ — dependências vendorizadas (não geridas por brew).
#
#  Dear ImGui é distribuído como arquivos-fonte (não tem pacote CMake oficial),
#  então o compilamos como uma biblioteca estática própria. Espera-se que o
#  repositório esteja em `third_party/imgui/` (clone do GitHub — ver README).
#
#  Core do ImGui + backends de plataforma (GLFW) e render (Metal), integrados
#  ao loop real na T4.1.
# ============================================================================

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")

if(EXISTS "${IMGUI_DIR}/imgui.cpp")
    add_library(imgui STATIC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        # Backends de plataforma (GLFW) e render (Metal), ativados na T4.1.
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_metal.mm
    )
    # Headers do ImGui (core) e dos backends disponíveis para include.
    target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends)

    # O backend Metal (.mm) usa ARC; ambos os backends precisam de GLFW/Metal.
    set_source_files_properties(${IMGUI_DIR}/backends/imgui_impl_metal.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc")
    find_package(glfw3 REQUIRED)
    target_link_libraries(imgui PUBLIC
        glfw
        "-framework Metal"
        "-framework QuartzCore")

    set(STARLAG_HAS_IMGUI TRUE PARENT_SCOPE)
else()
    message(WARNING
        "Dear ImGui não encontrado em ${IMGUI_DIR}. "
        "Clone com:\n"
        "  git clone --depth 1 --branch v1.91.5 "
        "https://github.com/ocornut/imgui.git third_party/imgui\n"
        "O alvo de smoke test de dependências (deps_smoke) será desabilitado até lá.")
    set(STARLAG_HAS_IMGUI FALSE PARENT_SCOPE)
endif()
