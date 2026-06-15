# ============================================================================
#  starlag — Simulador 3D de Viagens Interestelares Relativísticas
#  CMake raiz. Fonte única de build: gera tanto o app quanto a suíte de testes.
#
#  Build por linha de comando (Ninja/Make):
#     cmake -S . -B build
#     cmake --build build
#     ctest --test-dir build
#
#  Gerar projeto Xcode (macOS):
#     cmake -S . -B build-xcode -G Xcode
#     open build-xcode/starlag.xcodeproj
# ============================================================================

cmake_minimum_required(VERSION 3.20)

project(starlag
    VERSION 0.1.0
    DESCRIPTION "Simulador 3D de viagens interestelares relativísticas"
    LANGUAGES CXX OBJCXX)  # OBJCXX: ponte Metal/Cocoa em arquivos .mm (T0.2+).

# --- Padrão de linguagem -----------------------------------------------------
# C++20 conforme a stack decidida. Sem extensões de compilador (portabilidade).
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Exporta compile_commands.json (útil para clangd / análise estática).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Se nenhum tipo de build for especificado, usa Debug (projeto de estudo).
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Tipo de build" FORCE)
endif()

# --- Avisos de compilação ----------------------------------------------------
# Mantemos avisos ligados desde o início; correção numérica importa muito aqui.
add_compile_options(-Wall -Wextra -Wpedantic)

# --- Opções do projeto -------------------------------------------------------
option(STARLAG_BUILD_TESTS "Compilar a suíte de testes unitários" ON)

# --- Testes ------------------------------------------------------------------
# enable_testing() precisa vir ANTES dos subdiretórios que registram add_test
# (ex.: src/core/ com o smoke test de dependências).
if(STARLAG_BUILD_TESTS)
    enable_testing()
endif()

# --- Dependências de terceiros (vendorizadas) --------------------------------
# Adicionado antes de src/ para que alvos como `imgui` existam ao linkar.
add_subdirectory(third_party)

# --- Submódulos de código ----------------------------------------------------
# Cada camada (core/physics/data/render/ui) vira uma biblioteca conforme for
# implementada. Por ora core (T0.3) e render (T0.2); physics/data/ui entram nos
# respectivos marcos. Adicionamos os subdiretórios que já existem.
add_subdirectory(src)

# Suíte de testes dedicada (sentinela + futuros testes de física).
if(STARLAG_BUILD_TESTS)
    add_subdirectory(tests)
endif()
