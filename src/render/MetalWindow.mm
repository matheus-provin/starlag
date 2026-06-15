// ============================================================================
//  MetalWindow.mm — implementação Objective-C++ da ponte GLFW + Metal.
//
//  Estratégia (T0.2): usamos GLFW apenas para criar a janela e tratar input
//  (GLFW_NO_API, pois o Metal não é OpenGL). A partir da janela GLFW obtemos a
//  NSWindow nativa (via cabeçalho de exposição nativa do GLFW) e anexamos uma
//  CAMetalLayer ao seu contentView. O render loop adquire um drawable da layer
//  a cada frame e o limpa com a clear color.
//
//  Por que ObjC++ (.mm): CAMetalLayer, MTLDevice, MTLCommandQueue e NSWindow são
//  APIs Objective-C; este arquivo é o único que as toca. O resto do app vê só a
//  interface C++ de MetalWindow.h.
// ============================================================================

#include "render/MetalWindow.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <AppKit/AppKit.h>

// Exposição nativa do GLFW: precisamos da NSWindow por baixo da GLFWwindow.
#define GLFW_INCLUDE_NONE          // não puxar headers de GL.
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#include <stdexcept>

namespace starlag::render {

// ---------------------------------------------------------------------------
//  Estado interno (PIMPL). Mantém todos os objetos ObjC/Metal e a GLFWwindow.
// ---------------------------------------------------------------------------
struct MetalWindow::Impl {
    GLFWwindow* window = nullptr;
    id<MTLDevice> device = nil;
    id<MTLCommandQueue> queue = nil;
    CAMetalLayer* layer = nil;
};

// Conta janelas vivas para inicializar/terminar o GLFW uma única vez.
static int g_glfwWindowCount = 0;

MetalWindow::MetalWindow(int width, int height, const std::string& title)
    : impl_(std::make_unique<Impl>()) {

    // --- 1) GLFW: init (idempotente) e criação de janela sem contexto GL. ---
    if (g_glfwWindowCount == 0) {
        if (!glfwInit()) {
            throw std::runtime_error("Falha ao inicializar GLFW.");
        }
    }
    // Metal não é OpenGL: pedimos uma janela "sem API cliente".
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    impl_->window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!impl_->window) {
        if (g_glfwWindowCount == 0) glfwTerminate();
        throw std::runtime_error("Falha ao criar a janela GLFW.");
    }
    ++g_glfwWindowCount;

    // --- 2) Metal: device + fila de comandos. ---
    impl_->device = MTLCreateSystemDefaultDevice();
    if (!impl_->device) {
        throw std::runtime_error("Metal indisponivel: MTLCreateSystemDefaultDevice() falhou.");
    }
    impl_->queue = [impl_->device newCommandQueue];

    // --- 3) CAMetalLayer anexada ao contentView da NSWindow do GLFW. ---
    NSWindow* nsWindow = glfwGetCocoaWindow(impl_->window);
    CAMetalLayer* metalLayer = [CAMetalLayer layer];
    metalLayer.device = impl_->device;
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.framebufferOnly = YES;  // otimização: só usado como alvo de render.

    NSView* contentView = nsWindow.contentView;
    contentView.wantsLayer = YES;
    contentView.layer = metalLayer;

    // Acompanha a densidade de pixels (Retina) para o drawable ter resolução nativa.
    const CGFloat scale = nsWindow.backingScaleFactor;
    metalLayer.contentsScale = scale;
    metalLayer.drawableSize = CGSizeMake(width * scale, height * scale);

    impl_->layer = metalLayer;
}

MetalWindow::~MetalWindow() {
    if (impl_->window) {
        glfwDestroyWindow(impl_->window);
        --g_glfwWindowCount;
        if (g_glfwWindowCount == 0) {
            glfwTerminate();
        }
    }
    // Objetos ObjC (device/queue/layer) são liberados pelo ARC ao destruir o Impl.
}

bool MetalWindow::isOpen() const {
    return impl_->window && !glfwWindowShouldClose(impl_->window);
}

void MetalWindow::pollEvents() {
    glfwPollEvents();
}

void MetalWindow::renderClearFrame(const ClearColor& color) {
    @autoreleasepool {
        // Mantém o drawable em sincronia com o tamanho atual do framebuffer
        // (cobre redimensionamento da janela).
        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(impl_->window, &fbW, &fbH);
        if (fbW > 0 && fbH > 0) {
            impl_->layer.drawableSize = CGSizeMake(fbW, fbH);
        }

        id<CAMetalDrawable> drawable = [impl_->layer nextDrawable];
        if (!drawable) {
            return;  // sem drawable disponível neste instante; pula o frame.
        }

        // Render pass que apenas limpa a tela com a clear color.
        MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = drawable.texture;
        pass.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass.colorAttachments[0].storeAction = MTLStoreActionStore;
        pass.colorAttachments[0].clearColor =
            MTLClearColorMake(color.r, color.g, color.b, color.a);

        id<MTLCommandBuffer> cmd = [impl_->queue commandBuffer];
        id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:pass];
        // Nada a desenhar ainda (T0.2): apenas fecha o encoder após o clear.
        [enc endEncoding];

        [cmd presentDrawable:drawable];  // apresentado no vsync (~60 FPS).
        [cmd commit];
    }
}

}  // namespace starlag::render
