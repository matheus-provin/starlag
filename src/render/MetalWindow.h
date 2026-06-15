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

    // Renderiza um frame: adquire o drawable, limpa com `color` e apresenta.
    // O present ocorre no vsync, fixando o ritmo em ~60 FPS na maioria dos Macs.
    void renderClearFrame(const ClearColor& color);

private:
    struct Impl;                 // definido em MetalWindow.mm (ObjC++).
    std::unique_ptr<Impl> impl_;
};

}  // namespace starlag::render
