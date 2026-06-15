// ============================================================================
//  test_camera — testes da câmera 3D / matrizes view-projection (T2.1).
//
//  A câmera é matemática pura (GLM), então validamos headless contra valores
//  de referência calculados independentemente (perspectiva com depth [0,1] do
//  Metal e lookAt destro). Projetamos pontos do mundo para clip/NDC e conferimos
//  o mapeamento de profundidade (near→0, far→1), que é o ponto mais sutil.
// ============================================================================

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "render/Camera.h"

#include <glm/glm.hpp>

#include <cmath>
#include <cstdio>

using namespace starlag::render;

namespace {

int g_failures = 0;

void expectNear(const char* name, double got, double want, double tol = 1e-4) {
    const double diff = std::fabs(got - want);
    const double scale = std::fmax(1.0, std::fabs(want));
    const bool ok = diff <= tol * scale;
    std::printf("  [%s] %s  (got=%.6f, want=%.6f)\n", ok ? "PASS" : "FALHA", name, got, want);
    if (!ok) ++g_failures;
}

void expectTrue(const char* name, bool ok) {
    std::printf("  [%s] %s\n", ok ? "PASS" : "FALHA", name);
    if (!ok) ++g_failures;
}

// Projeta um ponto do mundo pela matriz e devolve as coords NDC (após dividir
// por w). Também expõe w (negativo do z de câmera) para checagens.
glm::vec4 project(const glm::mat4& m, const glm::vec3& p) {
    return m * glm::vec4(p, 1.0f);
}

// --- Perspectiva: elementos conhecidos --------------------------------------
void testPerspectiveValues() {
    std::printf("[perspectiva]\n");
    Camera cam;
    cam.setPerspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
    const glm::mat4 P = cam.projectionMatrix();
    // fovY=90, aspect=1 → m00=1, m11=1 (tan(45°)=1). GLM: m[col][row].
    expectNear("m00 (fovY=90,aspect=1)", P[0][0], 1.0);
    expectNear("m11 (fovY=90,aspect=1)", P[1][1], 1.0);
    expectNear("m22 = f/(n-f)", P[2][2], -1.001001, 1e-4);
    expectNear("m32 = -(f*n)/(f-n)", P[3][2], -0.100100, 1e-4);
    expectNear("m23 = -1 (perspectiva)", P[2][3], -1.0);

    Camera cam2;
    cam2.setPerspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 1000.0f);
    const glm::mat4 P2 = cam2.projectionMatrix();
    expectNear("m00 (fovY=60,16:9)", P2[0][0], 0.974279, 1e-4);
    expectNear("m11 (fovY=60)", P2[1][1], 1.732051, 1e-4);
}

// --- Mapeamento de profundidade near→0, far→1 (convenção Metal) -------------
void testDepthRange() {
    std::printf("[profundidade 0..1]\n");
    Camera cam;
    // Câmera em (0,0,5) olhando a origem (−Z). near=0.1, far=100.
    cam.lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0));
    cam.setPerspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
    const glm::mat4 VP = cam.viewProjection();

    // Ponto a 0.1 à frente (no near plane): z_mundo = 4.9 → depth ≈ 0.
    glm::vec4 nearP = project(VP, glm::vec3(0, 0, 4.9f));
    expectNear("near plane → depth 0", nearP.z / nearP.w, 0.0, 1e-3);

    // Ponto a 100 à frente (no far plane): z_mundo = −95 → depth ≈ 1.
    glm::vec4 farP = project(VP, glm::vec3(0, 0, -95.0f));
    expectNear("far plane → depth 1", farP.z / farP.w, 1.0, 1e-3);

    // Ponto intermediário (z_eye=−1, z_mundo=4): depth ≈ 0.900901.
    glm::vec4 midP = project(VP, glm::vec3(0, 0, 4.0f));
    expectNear("ponto medio → depth ~0.9009", midP.z / midP.w, 0.900901, 1e-3);

    // w = -z_eye deve ser positivo para pontos à frente (dentro do frustum).
    expectTrue("w > 0 para ponto a frente", farP.w > 0.0f);
}

// --- lookAt: a origem projeta no centro da tela -----------------------------
void testLookAtCentering() {
    std::printf("[lookAt centraliza alvo]\n");
    Camera cam;
    cam.lookAt(glm::vec3(8, 6, 12), glm::vec3(0, 0, 0));
    cam.setPerspective(glm::radians(60.0f), 1.0f, 0.1f, 1000.0f);
    const glm::mat4 VP = cam.viewProjection();

    // O alvo (origem) deve cair no centro do clip: NDC x≈0, y≈0.
    glm::vec4 c = project(VP, glm::vec3(0, 0, 0));
    expectNear("alvo NDC x ~ 0", c.x / c.w, 0.0, 1e-4);
    expectNear("alvo NDC y ~ 0", c.y / c.w, 0.0, 1e-4);
    expectTrue("alvo na frente (w>0)", c.w > 0.0f);

    // A posição da câmera é exposta corretamente.
    expectTrue("position() echo", cam.position() == glm::vec3(8, 6, 12));
}

// --- setAspect preserva fov/near/far ----------------------------------------
void testSetAspect() {
    std::printf("[setAspect preserva resto]\n");
    Camera cam;
    cam.setPerspective(glm::radians(60.0f), 1.0f, 0.1f, 1000.0f);
    const float m11Before = cam.projectionMatrix()[1][1];  // depende só de fovY.
    cam.setAspect(2.0f);
    const float m11After = cam.projectionMatrix()[1][1];
    expectNear("m11 inalterado ao mudar aspect", m11After, m11Before, 1e-6);
    // m00 escala com 1/aspect: ao dobrar o aspect, m00 cai à metade.
    expectNear("m00 escala com 1/aspect", cam.projectionMatrix()[0][0],
               m11Before / 2.0f, 1e-4);
}

}  // namespace

int main() {
    std::printf("== starlag T2.1 — testes da camera / view-projection ==\n");

    testPerspectiveValues();
    testDepthRange();
    testLookAtCentering();
    testSetAspect();

    if (g_failures == 0) {
        std::printf("\nTODOS os testes passaram.\n");
        return 0;
    }
    std::printf("\n%d teste(s) FALHARAM.\n", g_failures);
    return 1;
}
