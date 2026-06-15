// ============================================================================
//  test_fly_camera — testes da navegação 6-DOF (T2.3).
//
//  Lógica de câmera/controlador é C++ puro (GLM), validável headless: vetores de
//  base por yaw/pitch, movimento WASD proporcional a dt, mouse-look com clamp de
//  pitch, dolly por scroll e ajuste multiplicativo de velocidade (na borda).
// ============================================================================

#include "render/Camera.h"
#include "render/FlyCameraController.h"
#include "render/InputState.h"

#include <glm/glm.hpp>

#include <cmath>
#include <cstdio>

using namespace starlag::render;

namespace {

int g_failures = 0;

void expectTrue(const char* name, bool ok) {
    std::printf("  [%s] %s\n", ok ? "PASS" : "FALHA", name);
    if (!ok) ++g_failures;
}

void expectNear(const char* name, double got, double want, double tol = 1e-4) {
    const double diff = std::fabs(got - want);
    const double scale = std::fmax(1.0, std::fabs(want));
    const bool ok = diff <= tol * scale;
    std::printf("  [%s] %s  (got=%.5f, want=%.5f)\n", ok ? "PASS" : "FALHA", name, got, want);
    if (!ok) ++g_failures;
}

bool vecNear(const glm::vec3& a, const glm::vec3& b, float tol = 1e-4f) {
    return glm::length(a - b) <= tol;
}

// --- Vetores de base da câmera por yaw/pitch --------------------------------
void testCameraBasis() {
    std::printf("[camera: vetores de base]\n");
    Camera cam;
    cam.setYawPitch(0.0f, 0.0f);
    expectTrue("yaw0/pitch0 forward = -Z", vecNear(cam.forward(), glm::vec3(0, 0, -1)));
    expectTrue("yaw0 right = +X", vecNear(cam.right(), glm::vec3(1, 0, 0)));
    expectTrue("yaw0 up = +Y", vecNear(cam.up(), glm::vec3(0, 1, 0)));

    cam.setYawPitch(glm::radians(90.0f), 0.0f);
    expectTrue("yaw90 forward = -X", vecNear(cam.forward(), glm::vec3(-1, 0, 0)));

    // Pitch é clampado a ±89° (não chega a ±90°, evitando gimbal flip).
    cam.setYawPitch(0.0f, glm::radians(120.0f));
    expectNear("pitch clamp +89", cam.pitch(), glm::radians(89.0f), 1e-4);
    cam.setYawPitch(0.0f, glm::radians(-120.0f));
    expectNear("pitch clamp -89", cam.pitch(), glm::radians(-89.0f), 1e-4);

    // forward é sempre unitário.
    cam.setYawPitch(glm::radians(33.0f), glm::radians(-20.0f));
    expectNear("forward unitario", glm::length(cam.forward()), 1.0, 1e-5);
}

// --- Movimento WASD proporcional a dt e velocidade --------------------------
void testMovement() {
    std::printf("[movimento WASD]\n");
    FlyControllerParams p;
    p.moveSpeed = 10.0;
    FlyCameraController ctrl(p);
    ctrl.setSpeed(10.0);

    Camera cam;
    cam.setPosition(glm::vec3(0, 0, 0));
    cam.setYawPitch(0.0f, 0.0f);  // olhando -Z.

    // Andar para frente 0.5s a 10 pc/s → +5 ao longo de -Z (z = -5).
    InputState in;
    in.forward = true;
    ctrl.update(cam, in, 0.5);
    expectNear("frente 0.5s @10 → z=-5", cam.position().z, -5.0, 1e-4);
    expectNear("frente: x inalterado", cam.position().x, 0.0, 1e-4);

    // Independência de frame rate: 2 passos de 0.25s = 1 passo de 0.5s.
    Camera cam2;
    cam2.setPosition(glm::vec3(0, 0, 0));
    cam2.setYawPitch(0.0f, 0.0f);
    ctrl.setSpeed(10.0);
    ctrl.update(cam2, in, 0.25);
    ctrl.update(cam2, in, 0.25);
    expectNear("2x0.25s == 1x0.5s", cam2.position().z, -5.0, 1e-4);

    // Strafe direito (D) move em +X.
    Camera cam3;
    cam3.setPosition(glm::vec3(0, 0, 0));
    cam3.setYawPitch(0.0f, 0.0f);
    ctrl.setSpeed(10.0);
    InputState d;
    d.right = true;
    ctrl.update(cam3, d, 1.0);
    expectNear("strafe D → x=+10", cam3.position().x, 10.0, 1e-4);

    // Diagonal (W+D) não anda mais rápido (vetor normalizado).
    Camera cam4;
    cam4.setPosition(glm::vec3(0, 0, 0));
    cam4.setYawPitch(0.0f, 0.0f);
    ctrl.setSpeed(10.0);
    InputState wd;
    wd.forward = true; wd.right = true;
    ctrl.update(cam4, wd, 1.0);
    expectNear("diagonal: deslocamento total = 10", glm::length(cam4.position()), 10.0, 1e-3);

    // Subir (E) move em +Y mundo.
    Camera cam5;
    cam5.setPosition(glm::vec3(0, 0, 0));
    cam5.setYawPitch(0.0f, 0.0f);
    ctrl.setSpeed(10.0);
    InputState up;
    up.up = true;
    ctrl.update(cam5, up, 1.0);
    expectNear("subir E → y=+10", cam5.position().y, 10.0, 1e-4);
}

// --- Mouse-look: yaw/pitch a partir do delta --------------------------------
void testMouseLook() {
    std::printf("[mouse-look]\n");
    FlyControllerParams p;
    p.mouseSensitivity = 0.01;  // 0.01 rad/pixel para contas redondas.
    FlyCameraController ctrl(p);

    Camera cam;
    cam.setYawPitch(0.0f, 0.0f);

    // Sem looking, o delta é ignorado.
    InputState noLook;
    noLook.looking = false; noLook.mouseDx = 100.0;
    ctrl.update(cam, noLook, 0.016);
    expectNear("sem looking: yaw inalterado", cam.yaw(), 0.0, 1e-6);

    // Com looking: mouseDx=+50 → yaw += 0.5 rad.
    InputState look;
    look.looking = true; look.mouseDx = 50.0; look.mouseDy = 0.0;
    ctrl.update(cam, look, 0.016);
    expectNear("looking: yaw += 0.5", cam.yaw(), 0.5, 1e-4);

    // mouseDy=+30 → pitch -= 0.3 (mouse p/ baixo olha p/ baixo).
    InputState lookDown;
    lookDown.looking = true; lookDown.mouseDy = 30.0;
    ctrl.update(cam, lookDown, 0.016);
    expectNear("looking: pitch -= 0.3", cam.pitch(), -0.3, 1e-4);
}

// --- Dolly por scroll: avança ao longo do forward ---------------------------
void testDolly() {
    std::printf("[dolly por scroll]\n");
    FlyControllerParams p;
    p.dollyFactor = 0.1;
    FlyCameraController ctrl(p);
    ctrl.setSpeed(10.0);

    Camera cam;
    cam.setPosition(glm::vec3(0, 0, 0));
    cam.setYawPitch(0.0f, 0.0f);  // forward = -Z.

    // scroll +2 → passo = 2 * 0.1 * 10 = 2.0 ao longo de -Z → z = -2.
    InputState scroll;
    scroll.scrollDelta = 2.0;
    ctrl.update(cam, scroll, 0.016);
    expectNear("scroll +2 → z=-2 (avanca)", cam.position().z, -2.0, 1e-4);

    // scroll negativo recua.
    InputState back;
    back.scrollDelta = -1.0;
    ctrl.update(cam, back, 0.016);
    expectNear("scroll -1 → z=-1 (recua)", cam.position().z, -1.0, 1e-4);
}

// --- Ajuste de velocidade: multiplicativo e só na borda da tecla ------------
void testSpeedAdjust() {
    std::printf("[ajuste de velocidade]\n");
    FlyControllerParams p;
    p.speedStepFactor = 2.0;  // dobra/divide por 2 a cada tique.
    FlyCameraController ctrl(p);
    ctrl.setSpeed(10.0);

    Camera cam;  // estático; só observamos a velocidade.
    cam.setYawPitch(0.0f, 0.0f);

    // 1 frame com speedUp (borda solto→pressionado) → dobra para 20.
    InputState su;
    su.speedUp = true;
    ctrl.update(cam, su, 0.016);
    expectNear("speedUp 1 tique: 10→20", ctrl.speed(), 20.0, 1e-6);

    // Segurar (sem soltar) NÃO aplica de novo — borda já consumida.
    ctrl.update(cam, su, 0.016);
    expectNear("segurar nao repete: ainda 20", ctrl.speed(), 20.0, 1e-6);

    // Soltar e pressionar de novo → 40.
    InputState none;
    ctrl.update(cam, none, 0.016);
    ctrl.update(cam, su, 0.016);
    expectNear("re-press: 20→40", ctrl.speed(), 40.0, 1e-6);

    // speedDown divide.
    InputState sd;
    sd.speedDown = true;
    ctrl.update(cam, none, 0.016);
    ctrl.update(cam, sd, 0.016);
    expectNear("speedDown: 40→20", ctrl.speed(), 20.0, 1e-6);

    // Limites: não passa de maxSpeed nem abaixo de minSpeed.
    FlyControllerParams pl;
    pl.maxSpeed = 25.0; pl.minSpeed = 1.0; pl.speedStepFactor = 10.0;
    FlyCameraController ctrl2(pl);
    ctrl2.setSpeed(20.0);
    InputState up2; up2.speedUp = true;
    ctrl2.update(cam, none, 0.016);
    ctrl2.update(cam, up2, 0.016);
    expectTrue("respeita maxSpeed", ctrl2.speed() <= 25.0 + 1e-9);
}

}  // namespace

int main() {
    std::printf("== starlag T2.3 — testes da navegacao 6-DOF ==\n");

    testCameraBasis();
    testMovement();
    testMouseLook();
    testDolly();
    testSpeedAdjust();

    if (g_failures == 0) {
        std::printf("\nTODOS os testes passaram.\n");
        return 0;
    }
    std::printf("\n%d teste(s) FALHARAM.\n", g_failures);
    return 1;
}
