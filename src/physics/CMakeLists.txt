# ============================================================================
#  src/physics/ — núcleo físico relativístico (Marco 3).
#
#  Biblioteca C++ pura, SEM dependências externas (nem GPU, nem rede): toda a
#  física é testável de forma isolada por testes unitários (ótimo p/ CI/headless).
# ============================================================================

add_library(starlag_physics STATIC
    Relativity.cpp
    FlightProfile.cpp
    Calendar.cpp
    Simulation.cpp
)

# Permite #include "physics/Relativity.h" a partir da raiz de src/.
target_include_directories(starlag_physics PUBLIC ${CMAKE_SOURCE_DIR}/src)
