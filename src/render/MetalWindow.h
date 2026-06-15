// ============================================================================
//  MetalWindow — janela + camada Metal, com interface C++ limpa.
//
//  Esta é a fronteira entre o mundo C++ puro (resto do app) e o mundo
//  Objective-C/Metal/Cocoa (implementação em MetalWindow.mm). Nenhum tipo
//  Objective-C aparece aqui: usamos um Impl opaco (PIMPL) para que este header
//  possa ser incluído por qualquer .cpp do projeto sem exigir compilação ObjC++.
//
//  Marco 0 (T0.2): abrir uma janela com uma CAMetalLayer e rodar um render loop
//  que apenas limpa a tela (clear color) por frame, a ~60 FPS (vsync).
// ============================================================================

#pragma once

#include <memory>
#include <string>

namespace starlag::render {

// Cor de limpeza RGBA (componentes 0..1). double conforme convenção do projeto.
struct ClearColor {
    double r = 0.02;
    double g = 0.02;
    double b = 0.06;  // azul-noite, evocando o céu profundo.
    double a = 1.0;
};

// Janela com uma CAMetalLayer anexada. Não-copiável (possui recursos de GPU/SO).
class MetalWindow {
public:
    // Cria a janela (largura x altura, em pontos) com um título. Lança
    // std::runtime_error em falha de inicialização (GLFW/Metal indisponível).
    MetalWindow(int width, int height, const std::string& title);
    ~MetalWindow();

    MetalWindow(const MetalWindow&) = delete;
    MetalWindow& operator=(const MetalWindow&) = delete;

    // true enquanto o usuário não pediu para fechar a janela.
    bool isOpen() const;

    // Processa eventos do SO (teclado/mouse/fechar). Chame uma vez por frame.
    void pollEvents();

    // Tamanho do framebuffer em pixels (resolução nativa, já com Retina). Útil
    // para atualizar o aspect da câmera ao redimensionar a janela.
    void framebufferSize(int* width, int* height) const;

    // Renderiza um frame: adquire o drawable, limpa com `color` e apresenta.
    // O present ocorre no vsync, fixando o ritmo em ~60 FPS na maioria dos Macs.
    void renderClearFrame(const ClearColor& color);

    // Renderiza a cena de teste 3D (T2.1): grade no plano XZ + eixos coloridos,
    // vistos pela matriz `viewProj` (16 floats column-major = projection*view,
    // p.ex. de Camera::viewProjection()). Limpa com `color`, usa depth buffer e
    // apresenta no vsync. Compila o pipeline na primeira chamada (lazy).
    //
    // Recebe a matriz como ponteiro cru para não vazar GLM/ObjC pelo header
    // (GLM e Metal compartilham o layout column-major, então é cópia direta).
    void renderTestScene(const float* viewProj, const ClearColor& color);

    // Renderiza o campo de estrelas (T2.2): `instanceData` aponta para `count`
    // instâncias de 7 floats cada (posição XYZ + cor RGB + tamanho), conforme
    // StarInstance/StarField. Visto pela matriz `viewProj`. Limpa com `color`,
    // usa depth buffer e blending alpha (discos macios). Compila o pipeline de
    // pontos na primeira chamada (lazy). Se `drawGrid` for true, desenha também
    // a grade/eixos de referência (T2.1) por baixo das estrelas.
    void renderStars(const float* viewProj, const float* instanceData,
                     size_t count, const ClearColor& color, bool drawGrid = true);

private:
    struct Impl;                 // definido em MetalWindow.mm (ObjC++).
    std::unique_ptr<Impl> impl_;
};

}  // namespace starlag::render
