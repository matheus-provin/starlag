# ============================================================================
#  src/ — alvo principal do aplicativo `starlag`.
#
#  Estratégia de modularização: cada camada (core, physics, data, render, ui)
#  é compilada como uma biblioteca estática própria e linkada ao executável,
#  à medida que os marcos são implementados.
# ============================================================================

# Camadas já implementadas:
add_subdirectory(core)     # T0.3: utilidades + smoke test de dependências
add_subdirectory(physics)  # T3.1-3.4: núcleo relativístico
add_subdirectory(data)     # T1.1: cache local do catálogo HYG
add_subdirectory(render)   # T0.2: janela Metal (ObjC++)

# Camadas futuras (ativar conforme os marcos):
# add_subdirectory(ui)

add_executable(starlag
    main.cpp
)

# Diretório de includes raiz de src/ para permitir #include "render/...", etc.
target_include_directories(starlag PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(starlag PRIVATE starlag_render starlag_physics)
