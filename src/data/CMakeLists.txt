# ============================================================================
#  src/data/ — camada de dados estelares (catálogo HYG, parsing, busca).
#
#  Marco 1. Começa com o acesso ao cache local (T1.1); parser + modelo Star
#  (T1.2), índice de busca (T1.3) e enriquecimento via API (T1.4) entram depois.
# ============================================================================

add_library(starlag_data STATIC
    CatalogCache.cpp
    CatalogParser.cpp
    StarIndex.cpp
)

target_include_directories(starlag_data PUBLIC ${CMAKE_SOURCE_DIR}/src)

# Depende da física para constantes de conversão (parsec → ano-luz).
target_link_libraries(starlag_data PUBLIC starlag_physics)

# libcurl será usada quando o download remoto (fetchRemote) for implementado.
# Já deixamos o link preparado para não re-mexer no CMake depois.
find_package(CURL REQUIRED)
target_link_libraries(starlag_data PUBLIC CURL::libcurl)
