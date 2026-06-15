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

# Caminho do catálogo HYG para o app carregar (T2.2). TODO: migrar para
# ~/Library/Application Support/starlag/ quando o app for empacotado.
target_compile_definitions(starlag PRIVATE
    STARLAG_CATALOG_PATH="${CMAKE_SOURCE_DIR}/data/hygdata_v42.csv")

# starlag_render já traz starlag_data e starlag_physics transitivamente (PUBLIC),
# mas listamos os usados diretamente pelo main para deixar a dependência explícita.
target_link_libraries(starlag PRIVATE starlag_render starlag_data)
