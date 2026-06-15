# ============================================================================
#  third_party/ — dependências vendorizadas (não geridas por brew).
#
#  Dear ImGui é distribuído como arquivos-fonte (não tem pacote CMake oficial),
#  então o compilamos como uma biblioteca estática própria. Espera-se que o
#  repositório esteja em `third_party/imgui/` (clone do GitHub — ver README).
#
#  Aqui usamos APENAS o core do ImGui (sem backends). O backend Metal+plataforma
#  entra na T4.1, quando integrarmos a UI ao loop de render real.
# ============================================================================

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")

if(EXISTS "${IMGUI_DIR}/imgui.cpp")
    add_library(imgui STATIC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
    )
    # Headers do ImGui disponíveis como #include "imgui.h".
    target_include_directories(imgui PUBLIC ${IMGUI_DIR})
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
