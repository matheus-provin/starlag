# ============================================================================
#  src/render/ — camada de renderização (Metal).
#
#  Contém código Objective-C++ (.mm) que faz a ponte com Metal/Cocoa. É a única
#  camada que toca APIs Apple; o resto do app vê apenas interfaces C++.
# ============================================================================

add_library(starlag_render STATIC
    MetalWindow.mm
    Camera.cpp
    StarColor.cpp
    StarField.cpp
)

# Permite #include "render/MetalWindow.h" a partir da raiz de src/.
target_include_directories(starlag_render PUBLIC ${CMAKE_SOURCE_DIR}/src)

# StarColor/StarField usam o struct Star da camada de dados.
target_link_libraries(starlag_render PUBLIC starlag_data)

# Diretório dos shaders .metal, lido em runtime por MetalWindow.mm (T2.1).
# Quando o app for empacotado, migrar para um .metallib no bundle.
target_compile_definitions(starlag_render PRIVATE
    STARLAG_SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders")

# GLFW (instalado via Homebrew: `brew install glfw`).
find_package(glfw3 REQUIRED)
target_link_libraries(starlag_render PUBLIC glfw)

# GLM (header-only) para a câmera e matrizes view/projection.
find_package(glm REQUIRED)
target_link_libraries(starlag_render PUBLIC glm::glm)

# Frameworks Apple necessários para Metal + janela + input.
target_link_libraries(starlag_render PUBLIC
    "-framework Metal"
    "-framework QuartzCore"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreFoundation"
)

# ARC (Automatic Reference Counting) para o código Objective-C++.
set_source_files_properties(MetalWindow.mm PROPERTIES
    COMPILE_FLAGS "-fobjc-arc")
