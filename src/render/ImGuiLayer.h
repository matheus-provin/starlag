// ============================================================================
//  ImGuiLayer — integração do Dear ImGui ao loop Metal + GLFW (T4.1).
//
//  Encapsula a inicialização e o ciclo de frame do ImGui com os backends
//  imgui_impl_glfw + imgui_impl_metal. Interface C++ limpa (PIMPL): o resto do
//  app (main.cpp) chama beginFrame()/endFrame() e desenha os widgets com a API
//  pública do ImGui; nenhum tipo ObjC/Metal aparece aqui.
//
//  Os handles do device/layer/janela vêm do MetalWindow como ponteiros opacos
//  (void*), convertidos para os tipos ObjC dentro do .mm. Assim este header
//  permanece C++ puro e o acoplamento ObjC fica contido na implementação.
//
//  Fluxo por frame:
//    layer.beginFrame(renderPassDescriptor);   // dentro do render pass
//    ... ImGui::Begin(...); widgets; ImGui::End();
//    layer.endFrame(commandBuffer, encoder);    // antes de endEncoding
// ============================================================================

#pragma once

#include <memory>

namespace starlag::render {

class ImGuiLayer {
public:
    ImGuiLayer() = default;
    ~ImGuiLayer();

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    // Inicializa o ImGui. `glfwWindow` é o GLFWwindow*; `metalDevice` é o
    // id<MTLDevice> — ambos passados como void* (cast no .mm). Idempotente.
    void init(void* glfwWindow, void* metalDevice);

    // Começa um novo frame de UI. `renderPassDescriptor` é o
    // MTLRenderPassDescriptor* do frame atual (para o backend Metal).
    void beginFrame(void* renderPassDescriptor);

    // Finaliza a UI: renderiza os draw data do ImGui no `encoder`
    // (id<MTLRenderCommandEncoder>) usando o `commandBuffer` (id<MTLCommandBuffer>).
    void endFrame(void* commandBuffer, void* encoder);

    // ImGui quer capturar o mouse/teclado neste frame? (cursor sobre um painel).
    // O app usa isto para não disparar picking/navegação ao interagir com a UI.
    bool wantCaptureMouse() const;
    bool wantCaptureKeyboard() const;

    bool initialized() const { return initialized_; }

private:
    bool initialized_ = false;
};

}  // namespace starlag::render
