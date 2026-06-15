# ============================================================================
#  tests/ — suíte de testes unitários do `starlag`.
#
#  A física do Marco 3 é testável de forma pura (sem GUI/Metal), então os
#  testes mais importantes virão dela. Por ora, um teste-sentinela valida que
#  a infraestrutura de testes (CTest) está funcionando desde a fundação.
#
#  Decisão: usamos asserções simples de C++ (<cassert>/exit codes) sem framework
#  externo no início, para não acoplar uma dependência antes do necessário.
#  Se a suíte crescer, avaliamos Catch2/doctest como third_party (header-only).
# ============================================================================

add_executable(test_sentinel test_sentinel.cpp)
target_include_directories(test_sentinel PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME sentinel COMMAND test_sentinel)

# T3.1 — testes do núcleo de dilatação temporal (linkam a lib de física pura).
add_executable(test_relativity test_relativity.cpp)
target_link_libraries(test_relativity PRIVATE starlag_physics)
add_test(NAME relativity COMMAND test_relativity)

# T3.2 — testes do perfil de voo acelerado (hiperbólico).
add_executable(test_flight_profile test_flight_profile.cpp)
target_link_libraries(test_flight_profile PRIVATE starlag_physics)
add_test(NAME flight_profile COMMAND test_flight_profile)

# T3.3 — testes do calendário gregoriano.
add_executable(test_calendar test_calendar.cpp)
target_link_libraries(test_calendar PRIVATE starlag_physics)
add_test(NAME calendar COMMAND test_calendar)

# T3.4 — testes da fachada de simulação (métricas agregadas).
add_executable(test_simulation test_simulation.cpp)
target_link_libraries(test_simulation PRIVATE starlag_physics)
add_test(NAME simulation COMMAND test_simulation)

# T1.1 — testes do cache local do catálogo HYG.
# Injeta o caminho absoluto do CSV real (em data/) via macro, pois o CTest roda
# de um diretório de build distinto da raiz do projeto.
add_executable(test_catalog_cache test_catalog_cache.cpp)
target_link_libraries(test_catalog_cache PRIVATE starlag_data)
target_compile_definitions(test_catalog_cache PRIVATE
    STARLAG_TEST_CATALOG="${CMAKE_SOURCE_DIR}/data/hygdata_v42.csv")
add_test(NAME catalog_cache COMMAND test_catalog_cache)

# T1.2 — testes do parser CSV + struct Star (sintéticos + catálogo real).
add_executable(test_catalog_parser test_catalog_parser.cpp)
target_link_libraries(test_catalog_parser PRIVATE starlag_data)
target_compile_definitions(test_catalog_parser PRIVATE
    STARLAG_TEST_CATALOG="${CMAKE_SOURCE_DIR}/data/hygdata_v42.csv")
add_test(NAME catalog_parser COMMAND test_catalog_parser)

# T1.3 — testes do índice de busca/seleção (sintéticos + catálogo real).
add_executable(test_star_index test_star_index.cpp)
target_link_libraries(test_star_index PRIVATE starlag_data)
target_compile_definitions(test_star_index PRIVATE
    STARLAG_TEST_CATALOG="${CMAKE_SOURCE_DIR}/data/hygdata_v42.csv")
add_test(NAME star_index COMMAND test_star_index)

# T2.1 — testes da câmera 3D / matrizes view-projection (GLM puro, headless).
add_executable(test_camera test_camera.cpp)
target_link_libraries(test_camera PRIVATE starlag_render)
add_test(NAME camera COMMAND test_camera)

# T2.2 — testes da cor de estrelas (B−V/spect → RGB).
add_executable(test_star_color test_star_color.cpp)
target_link_libraries(test_star_color PRIVATE starlag_render)
add_test(NAME star_color COMMAND test_star_color)

# T2.2 — testes da montagem do campo de estrelas (instâncias + sizing).
add_executable(test_star_field test_star_field.cpp)
target_link_libraries(test_star_field PRIVATE starlag_render)
target_compile_definitions(test_star_field PRIVATE
    STARLAG_TEST_CATALOG="${CMAKE_SOURCE_DIR}/data/hygdata_v42.csv")
add_test(NAME star_field COMMAND test_star_field)
