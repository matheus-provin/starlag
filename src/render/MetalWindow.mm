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

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// Diretório dos shaders, injetado pelo CMake (mesmo padrão de STARLAG_TEST_CATALOG).
// TODO: ao empacotar o app, migrar para um .metallib pré-compilado no bundle.
#ifndef STARLAG_SHADER_DIR
#define STARLAG_SHADER_DIR ""
#endif

namespace starlag::render {

namespace {

// Vértice da cena de teste (deve casar com VertexIn em basic.metal):
// posição (x,y,z) + cor (r,g,b), tudo em float (Metal não usa double na GPU).
struct VertexCPU {
    float px, py, pz;
    float cr, cg, cb;
};

// Constrói a geometria da cena de teste: grade no plano XZ + 3 eixos coloridos.
// Desenhada como linhas (MTLPrimitiveTypeLine), em pares de vértices.
std::vector<VertexCPU> buildTestSceneLines() {
    std::vector<VertexCPU> v;
    const int half = 10;          // grade de -10..10 (em parsecs).
    const float step = 1.0f;
    const float g = 0.25f;        // cinza discreto da grade.

    // Linhas paralelas ao eixo Z (variando X) e ao eixo X (variando Z).
    for (int i = -half; i <= half; ++i) {
        const float c = i * step;
        v.push_back({c, 0.0f, -float(half), g, g, g});
        v.push_back({c, 0.0f, +float(half), g, g, g});
        v.push_back({-float(half), 0.0f, c, g, g, g});
        v.push_back({+float(half), 0.0f, c, g, g, g});
    }

    // Eixos coloridos a partir da origem (referencial espacial para as estrelas):
    const float L = float(half);
    v.push_back({0, 0, 0, 1, 0.2f, 0.2f}); v.push_back({L, 0, 0, 1, 0.2f, 0.2f});  // X vermelho
    v.push_back({0, 0, 0, 0.2f, 1, 0.2f}); v.push_back({0, L, 0, 0.2f, 1, 0.2f});  // Y verde
    v.push_back({0, 0, 0, 0.2f, 0.4f, 1}); v.push_back({0, 0, L, 0.2f, 0.4f, 1});  // Z azul
    return v;
}

// Lê o arquivo de shader do disco (caminho de STARLAG_SHADER_DIR).
std::string loadShaderSource(const std::string& filename) {
    const std::string path = std::string(STARLAG_SHADER_DIR) + "/" + filename;
    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::runtime_error("Nao foi possivel abrir o shader: " + path);
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

}  // namespace

// ---------------------------------------------------------------------------
//  Estado interno (PIMPL). Mantém todos os objetos ObjC/Metal e a GLFWwindow.
// ---------------------------------------------------------------------------
struct MetalWindow::Impl {
    GLFWwindow* window = nullptr;
    id<MTLDevice> device = nil;
    id<MTLCommandQueue> queue = nil;
    CAMetalLayer* layer = nil;

    // Estado de input (T2.3), preenchido a cada pollEvents().
    InputState input;
    double lastMouseX = 0.0, lastMouseY = 0.0;
    bool hadMouse = false;       // já temos uma posição anterior do mouse?
    double scrollAccum = 0.0;    // acumulado pela callback de scroll do GLFW.
    bool prevLeftDown = false;   // estado anterior do botão esquerdo (T2.4: clique).

    // Recursos da cena de teste (T2.1), criados sob demanda na 1ª renderização.
    id<MTLRenderPipelineState> pipeline = nil;
    id<MTLDepthStencilState> depthState = nil;
    id<MTLBuffer> sceneVertices = nil;
    NSUInteger sceneVertexCount = 0;

    // Recursos do campo de estrelas (T2.2): pipeline de pontos + buffer de
    // instâncias (recriado quando a contagem aumenta).
    id<MTLRenderPipelineState> starPipeline = nil;
    id<MTLBuffer> starBuffer = nil;
    NSUInteger starCapacity = 0;  // capacidade do buffer, em nº de instâncias.

    // Marcadores de seleção (T2.4): buffer próprio + estado de profundidade
    // "sempre passa" (desenha por cima das estrelas, sem ser ocluído).
    id<MTLBuffer> markerBuffer = nil;
    NSUInteger markerCapacity = 0;
    id<MTLDepthStencilState> noDepthState = nil;

    // Callback de UI (T4.1): desenhado dentro do render pass do renderStars.
    MetalWindow::UiCallback uiCallback;

    // Depth buffer; recriado quando o tamanho do drawable muda.
    id<MTLTexture> depthTexture = nil;
    NSUInteger depthW = 0, depthH = 0;

    // Garante que pipeline/depth/geometria existam (compila o shader 1x).
    void ensureSceneResources();
    // Garante o pipeline de pontos das estrelas (compila stars.metal 1x).
    void ensureStarResources();
    // Garante uma depth texture com (w,h); recria se o tamanho mudou.
    void ensureDepthTexture(NSUInteger w, NSUInteger h);
};

void MetalWindow::Impl::ensureSceneResources() {
    if (pipeline != nil) return;  // já inicializado.

    // 1) Compila a library a partir do source em runtime.
    NSError* err = nil;
    const std::string src = loadShaderSource("basic.metal");
    NSString* srcStr = [NSString stringWithUTF8String:src.c_str()];
    id<MTLLibrary> lib = [device newLibraryWithSource:srcStr options:nil error:&err];
    if (!lib) {
        const char* msg = err ? err.localizedDescription.UTF8String : "desconhecido";
        throw std::runtime_error(std::string("Falha ao compilar basic.metal: ") + msg);
    }
    id<MTLFunction> vfn = [lib newFunctionWithName:@"vertex_main"];
    id<MTLFunction> ffn = [lib newFunctionWithName:@"fragment_main"];

    // 2) Vertex descriptor: casa o layout de VertexCPU com VertexIn do shader.
    MTLVertexDescriptor* vdesc = [[MTLVertexDescriptor alloc] init];
    vdesc.attributes[0].format = MTLVertexFormatFloat3;          // position
    vdesc.attributes[0].offset = 0;
    vdesc.attributes[0].bufferIndex = 0;
    vdesc.attributes[1].format = MTLVertexFormatFloat3;          // color
    vdesc.attributes[1].offset = sizeof(float) * 3;
    vdesc.attributes[1].bufferIndex = 0;
    vdesc.layouts[0].stride = sizeof(VertexCPU);

    // 3) Pipeline state: liga shaders + formatos de cor e profundidade.
    MTLRenderPipelineDescriptor* pdesc = [[MTLRenderPipelineDescriptor alloc] init];
    pdesc.vertexFunction = vfn;
    pdesc.fragmentFunction = ffn;
    pdesc.vertexDescriptor = vdesc;
    pdesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;  // = layer.
    pdesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
    pipeline = [device newRenderPipelineStateWithDescriptor:pdesc error:&err];
    if (!pipeline) {
        const char* msg = err ? err.localizedDescription.UTF8String : "desconhecido";
        throw std::runtime_error(std::string("Falha ao criar pipeline: ") + msg);
    }

    // 4) Depth-stencil: testa profundidade (menor z passa) e escreve no buffer.
    MTLDepthStencilDescriptor* ddesc = [[MTLDepthStencilDescriptor alloc] init];
    ddesc.depthCompareFunction = MTLCompareFunctionLess;
    ddesc.depthWriteEnabled = YES;
    depthState = [device newDepthStencilStateWithDescriptor:ddesc];

    // 5) Geometria da cena, copiada para um buffer da GPU (compartilhado).
    const std::vector<VertexCPU> verts = buildTestSceneLines();
    sceneVertexCount = verts.size();
    sceneVertices = [device newBufferWithBytes:verts.data()
                                        length:verts.size() * sizeof(VertexCPU)
                                       options:MTLResourceStorageModeShared];
}

void MetalWindow::Impl::ensureStarResources() {
    if (starPipeline != nil) return;  // já inicializado.

    NSError* err = nil;
    const std::string src = loadShaderSource("stars.metal");
    NSString* srcStr = [NSString stringWithUTF8String:src.c_str()];
    id<MTLLibrary> lib = [device newLibraryWithSource:srcStr options:nil error:&err];
    if (!lib) {
        const char* msg = err ? err.localizedDescription.UTF8String : "desconhecido";
        throw std::runtime_error(std::string("Falha ao compilar stars.metal: ") + msg);
    }
    id<MTLFunction> vfn = [lib newFunctionWithName:@"star_vertex"];
    id<MTLFunction> ffn = [lib newFunctionWithName:@"star_fragment"];

    // Vertex descriptor: posição(3) + cor(3) + size(1) = StarInstance (7 floats).
    MTLVertexDescriptor* vdesc = [[MTLVertexDescriptor alloc] init];
    vdesc.attributes[0].format = MTLVertexFormatFloat3;            // position
    vdesc.attributes[0].offset = 0;
    vdesc.attributes[0].bufferIndex = 0;
    vdesc.attributes[1].format = MTLVertexFormatFloat3;            // color
    vdesc.attributes[1].offset = sizeof(float) * 3;
    vdesc.attributes[1].bufferIndex = 0;
    vdesc.attributes[2].format = MTLVertexFormatFloat;             // size
    vdesc.attributes[2].offset = sizeof(float) * 6;
    vdesc.attributes[2].bufferIndex = 0;
    vdesc.layouts[0].stride = sizeof(float) * 7;

    MTLRenderPipelineDescriptor* pdesc = [[MTLRenderPipelineDescriptor alloc] init];
    pdesc.vertexFunction = vfn;
    pdesc.fragmentFunction = ffn;
    pdesc.vertexDescriptor = vdesc;
    pdesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    pdesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
    // Blending alpha: os discos macios do fragment se misturam com o fundo.
    pdesc.colorAttachments[0].blendingEnabled = YES;
    pdesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    pdesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    pdesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    pdesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
    pdesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pdesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

    starPipeline = [device newRenderPipelineStateWithDescriptor:pdesc error:&err];
    if (!starPipeline) {
        const char* msg = err ? err.localizedDescription.UTF8String : "desconhecido";
        throw std::runtime_error(std::string("Falha ao criar pipeline de estrelas: ") + msg);
    }

    // Estado de profundidade "sempre passa" (sem escrita) para os marcadores de
    // seleção (T2.4) aparecerem por cima do campo de estrelas.
    MTLDepthStencilDescriptor* nd = [[MTLDepthStencilDescriptor alloc] init];
    nd.depthCompareFunction = MTLCompareFunctionAlways;
    nd.depthWriteEnabled = NO;
    noDepthState = [device newDepthStencilStateWithDescriptor:nd];
}

void MetalWindow::Impl::ensureDepthTexture(NSUInteger w, NSUInteger h) {
    if (depthTexture != nil && depthW == w && depthH == h) return;  // ainda válida.

    MTLTextureDescriptor* td = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                     width:w
                                    height:h
                                 mipmapped:NO];
    td.usage = MTLTextureUsageRenderTarget;
    td.storageMode = MTLStorageModePrivate;  // só a GPU acessa o depth buffer.
    depthTexture = [device newTextureWithDescriptor:td];
    depthW = w;
    depthH = h;
}

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

    // --- 4) Input (T2.3): user pointer + callback de scroll. ---
    // A roda do mouse só chega por callback no GLFW; acumulamos e drenamos no
    // pollEvents. As teclas e a posição do mouse são lidas por polling direto.
    glfwSetWindowUserPointer(impl_->window, impl_.get());
    glfwSetScrollCallback(impl_->window, [](GLFWwindow* w, double /*dx*/, double dy) {
        auto* impl = static_cast<Impl*>(glfwGetWindowUserPointer(w));
        if (impl) impl->scrollAccum += dy;
    });
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

    GLFWwindow* w = impl_->window;
    InputState& in = impl_->input;

    // --- Teclas de movimento (estado atual: segurando) ---
    auto held = [&](int key) { return glfwGetKey(w, key) == GLFW_PRESS; };
    in.forward = held(GLFW_KEY_W);
    in.back    = held(GLFW_KEY_S);
    in.left    = held(GLFW_KEY_A);
    in.right   = held(GLFW_KEY_D);
    in.up      = held(GLFW_KEY_E) || held(GLFW_KEY_SPACE);
    in.down    = held(GLFW_KEY_Q) || held(GLFW_KEY_LEFT_CONTROL);

    // Ajuste de velocidade por teclas dedicadas (a borda é detectada no controller).
    in.speedUp   = held(GLFW_KEY_EQUAL) || held(GLFW_KEY_RIGHT_BRACKET);
    in.speedDown = held(GLFW_KEY_MINUS) || held(GLFW_KEY_LEFT_BRACKET);

    // --- Mouse-look: ativo enquanto o botão direito está pressionado ---
    in.looking = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    double mx = 0.0, my = 0.0;
    glfwGetCursorPos(w, &mx, &my);
    if (in.looking && impl_->hadMouse) {
        in.mouseDx = mx - impl_->lastMouseX;
        in.mouseDy = my - impl_->lastMouseY;
    } else {
        in.mouseDx = 0.0;
        in.mouseDy = 0.0;
    }
    impl_->lastMouseX = mx;
    impl_->lastMouseY = my;
    impl_->hadMouse = true;
    // Esconde/trava o cursor durante o look para um fly mais confortável.
    glfwSetInputMode(w, GLFW_CURSOR,
                     in.looking ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    // --- Seleção (T2.4): posição do cursor + clique esquerdo (borda de soltura) ---
    // glfwGetCursorPos dá PONTOS (screen coords); o ray-cast usa a viewport em
    // PIXELS de framebuffer. Em telas Retina esses diferem pelo backing scale,
    // então convertemos o cursor para pixels (senão o raio sai na escala errada
    // e o marcador aparece deslocado do clique).
    int winW = 0, winH = 0, fbW = 0, fbH = 0;
    glfwGetWindowSize(w, &winW, &winH);
    glfwGetFramebufferSize(w, &fbW, &fbH);
    const double scaleX = (winW > 0) ? static_cast<double>(fbW) / winW : 1.0;
    const double scaleY = (winH > 0) ? static_cast<double>(fbH) / winH : 1.0;
    in.cursorX = mx * scaleX;
    in.cursorY = my * scaleY;
    const bool leftDown = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    // Um "clique" é o frame em que o botão foi SOLTO (press→release): evita
    // disparar repetidamente enquanto segura e não conflita com o arraste.
    in.clicked = impl_->prevLeftDown && !leftDown && !in.looking;
    impl_->prevLeftDown = leftDown;

    // --- Scroll: drena o acumulado da callback ---
    in.scrollDelta = impl_->scrollAccum;
    impl_->scrollAccum = 0.0;
}

const InputState& MetalWindow::input() const {
    return impl_->input;
}

void MetalWindow::framebufferSize(int* width, int* height) const {
    int w = 0, h = 0;
    glfwGetFramebufferSize(impl_->window, &w, &h);
    if (width) *width = w;
    if (height) *height = h;
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

void MetalWindow::renderTestScene(const float* viewProj, const ClearColor& color) {
    @autoreleasepool {
        // Mantém o drawable em sincronia com o tamanho atual do framebuffer.
        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(impl_->window, &fbW, &fbH);
        if (fbW > 0 && fbH > 0) {
            impl_->layer.drawableSize = CGSizeMake(fbW, fbH);
        }

        id<CAMetalDrawable> drawable = [impl_->layer nextDrawable];
        if (!drawable) return;  // sem drawable agora; pula o frame.

        const NSUInteger w = drawable.texture.width;
        const NSUInteger h = drawable.texture.height;

        // Recursos lazy: compila o pipeline na 1ª chamada; depth segue o tamanho.
        impl_->ensureSceneResources();
        impl_->ensureDepthTexture(w, h);

        // Render pass: limpa cor + profundidade (1.0 = far).
        MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = drawable.texture;
        pass.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass.colorAttachments[0].storeAction = MTLStoreActionStore;
        pass.colorAttachments[0].clearColor =
            MTLClearColorMake(color.r, color.g, color.b, color.a);
        pass.depthAttachment.texture = impl_->depthTexture;
        pass.depthAttachment.loadAction = MTLLoadActionClear;
        pass.depthAttachment.storeAction = MTLStoreActionDontCare;
        pass.depthAttachment.clearDepth = 1.0;

        id<MTLCommandBuffer> cmd = [impl_->queue commandBuffer];
        id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:pass];

        [enc setRenderPipelineState:impl_->pipeline];
        [enc setDepthStencilState:impl_->depthState];

        // Buffer 0 = vértices; buffer 1 = uniforme MVP (16 floats column-major,
        // mesmo layout do GLM — cópia direta do ponteiro recebido).
        [enc setVertexBuffer:impl_->sceneVertices offset:0 atIndex:0];
        [enc setVertexBytes:viewProj length:sizeof(float) * 16 atIndex:1];

        [enc drawPrimitives:MTLPrimitiveTypeLine
                vertexStart:0
                vertexCount:impl_->sceneVertexCount];

        [enc endEncoding];
        [cmd presentDrawable:drawable];
        [cmd commit];
    }
}

void MetalWindow::renderStars(const float* viewProj, const float* instanceData,
                              size_t count, const ClearColor& color, bool drawGrid,
                              const float* markerData, size_t markerCount) {
    @autoreleasepool {
        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(impl_->window, &fbW, &fbH);
        if (fbW > 0 && fbH > 0) {
            impl_->layer.drawableSize = CGSizeMake(fbW, fbH);
        }

        id<CAMetalDrawable> drawable = [impl_->layer nextDrawable];
        if (!drawable) return;

        const NSUInteger w = drawable.texture.width;
        const NSUInteger h = drawable.texture.height;

        impl_->ensureSceneResources();  // grade (depthState + geometria).
        impl_->ensureStarResources();   // pipeline de pontos.
        impl_->ensureDepthTexture(w, h);

        // (Re)aloca o buffer de instâncias se necessário e copia os dados.
        const NSUInteger floatsPerInstance = 7;
        if (count > 0) {
            if (impl_->starBuffer == nil || impl_->starCapacity < count) {
                impl_->starBuffer =
                    [impl_->device newBufferWithLength:count * floatsPerInstance * sizeof(float)
                                               options:MTLResourceStorageModeShared];
                impl_->starCapacity = count;
            }
            std::memcpy(impl_->starBuffer.contents, instanceData,
                        count * floatsPerInstance * sizeof(float));
        }

        MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = drawable.texture;
        pass.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass.colorAttachments[0].storeAction = MTLStoreActionStore;
        pass.colorAttachments[0].clearColor =
            MTLClearColorMake(color.r, color.g, color.b, color.a);
        pass.depthAttachment.texture = impl_->depthTexture;
        pass.depthAttachment.loadAction = MTLLoadActionClear;
        pass.depthAttachment.storeAction = MTLStoreActionDontCare;
        pass.depthAttachment.clearDepth = 1.0;

        id<MTLCommandBuffer> cmd = [impl_->queue commandBuffer];
        id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:pass];
        [enc setDepthStencilState:impl_->depthState];

        // Grade de referência por baixo (opcional).
        if (drawGrid) {
            [enc setRenderPipelineState:impl_->pipeline];
            [enc setVertexBuffer:impl_->sceneVertices offset:0 atIndex:0];
            [enc setVertexBytes:viewProj length:sizeof(float) * 16 atIndex:1];
            [enc drawPrimitives:MTLPrimitiveTypeLine
                    vertexStart:0
                    vertexCount:impl_->sceneVertexCount];
        }

        // Campo de estrelas (pontos instanciados).
        if (count > 0) {
            [enc setRenderPipelineState:impl_->starPipeline];
            [enc setVertexBuffer:impl_->starBuffer offset:0 atIndex:0];
            [enc setVertexBytes:viewProj length:sizeof(float) * 16 atIndex:1];
            [enc drawPrimitives:MTLPrimitiveTypePoint
                    vertexStart:0
                    vertexCount:count];
        }

        // Marcadores de seleção (T2.4): por cima, sem teste de profundidade.
        if (markerCount > 0 && markerData != nullptr) {
            if (impl_->markerBuffer == nil || impl_->markerCapacity < markerCount) {
                impl_->markerBuffer =
                    [impl_->device newBufferWithLength:markerCount * floatsPerInstance * sizeof(float)
                                               options:MTLResourceStorageModeShared];
                impl_->markerCapacity = markerCount;
            }
            std::memcpy(impl_->markerBuffer.contents, markerData,
                        markerCount * floatsPerInstance * sizeof(float));

            [enc setRenderPipelineState:impl_->starPipeline];
            [enc setDepthStencilState:impl_->noDepthState];  // sempre visível.
            [enc setVertexBuffer:impl_->markerBuffer offset:0 atIndex:0];
            [enc setVertexBytes:viewProj length:sizeof(float) * 16 atIndex:1];
            [enc drawPrimitives:MTLPrimitiveTypePoint
                    vertexStart:0
                    vertexCount:markerCount];
        }

        // UI (T4.1): desenhada por cima da cena, no mesmo render pass.
        if (impl_->uiCallback) {
            impl_->uiCallback((__bridge void*)pass, (__bridge void*)cmd, (__bridge void*)enc);
        }

        [enc endEncoding];
        [cmd presentDrawable:drawable];
        [cmd commit];
    }
}

void MetalWindow::setUiCallback(UiCallback cb) {
    impl_->uiCallback = std::move(cb);
}

void* MetalWindow::glfwWindowHandle() const {
    return impl_->window;
}

void* MetalWindow::metalDeviceHandle() const {
    return (__bridge void*)impl_->device;
}

}  // namespace starlag::render
