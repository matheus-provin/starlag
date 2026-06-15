// ============================================================================
//  ImGuiLayer.mm — implementação ObjC++ da integração ImGui (T4.1).
// ============================================================================

#include "render/ImGuiLayer.h"

#import <Metal/Metal.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_metal.h"

namespace starlag::render {

ImGuiLayer::~ImGuiLayer() {
    if (initialized_) {
        ImGui_ImplMetal_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        initialized_ = false;
    }
}

void ImGuiLayer::init(void* glfwWindow, void* metalDevice) {
    if (initialized_) return;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;  // não persistir layout em disco (projeto de estudo).
    ImGui::StyleColorsDark();

    auto* window = static_cast<GLFWwindow*>(glfwWindow);
    id<MTLDevice> device = (__bridge id<MTLDevice>)metalDevice;

    // install_callbacks=true: o backend GLFW encadeia os callbacks de input do
    // ImGui (teclado/mouse/scroll) sem sobrescrever os nossos (ele preserva e
    // chama o callback anterior — ex.: o de scroll que registramos na T2.3).
    ImGui_ImplGlfw_InitForOther(window, /*install_callbacks=*/true);
    ImGui_ImplMetal_Init(device);

    initialized_ = true;
}

void ImGuiLayer::beginFrame(void* renderPassDescriptor) {
    if (!initialized_) return;
    auto* pass = (__bridge MTLRenderPassDescriptor*)renderPassDescriptor;
    ImGui_ImplMetal_NewFrame(pass);
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::endFrame(void* commandBuffer, void* encoder) {
    if (!initialized_) return;
    ImGui::Render();
    id<MTLCommandBuffer> cmd = (__bridge id<MTLCommandBuffer>)commandBuffer;
    id<MTLRenderCommandEncoder> enc = (__bridge id<MTLRenderCommandEncoder>)encoder;
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), cmd, enc);
}

bool ImGuiLayer::wantCaptureMouse() const {
    if (!initialized_) return false;
    return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiLayer::wantCaptureKeyboard() const {
    if (!initialized_) return false;
    return ImGui::GetIO().WantCaptureKeyboard;
}

}  // namespace starlag::render
