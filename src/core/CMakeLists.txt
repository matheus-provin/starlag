# ============================================================================
#  src/core/ — utilidades base e harness de validação.
#
#  Por ora contém o smoke test de dependências (T0.3): um executável headless
#  que prova GLM + nlohmann/json + libcurl + Dear ImGui integradas e funcionais.
#  Tipos base do app (App loop, etc.) também viverão aqui em marcos futuros.
# ============================================================================

# Dependências via Homebrew / SDK do sistema.
find_package(glm CONFIG REQUIRED)               # brew install glm
find_package(nlohmann_json CONFIG REQUIRED)     # brew install nlohmann-json
find_package(CURL REQUIRED)                      # libcurl do SDK do macOS

# O smoke test só é construído se o ImGui estiver presente (ver third_party/).
if(STARLAG_HAS_IMGUI)
    add_executable(deps_smoke deps_smoke.cpp)
    target_link_libraries(deps_smoke PRIVATE
        glm::glm
        nlohmann_json::nlohmann_json
        CURL::libcurl
        imgui
    )
    # Registra como teste: passa se exit code == 0 (todos os checks PASS).
    if(STARLAG_BUILD_TESTS)
        add_test(NAME deps_smoke COMMAND deps_smoke)
    endif()
else()
    message(STATUS "deps_smoke desabilitado: Dear ImGui ausente em third_party/imgui.")
endif()
