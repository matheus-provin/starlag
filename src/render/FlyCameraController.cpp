// ============================================================================
//  FlyCameraController.cpp — implementação da navegação 6-DOF (T2.3).
// ============================================================================

#include "render/FlyCameraController.h"

#include <algorithm>

namespace starlag::render {

void FlyCameraController::setSpeed(double s) {
    speed_ = std::clamp(s, params_.minSpeed, params_.maxSpeed);
}

void FlyCameraController::update(Camera& camera, const InputState& input, double dt) {
    if (dt < 0.0) dt = 0.0;  // proteção contra dt negativo (clock anômalo).

    // --- 1) Ajuste de velocidade (passo multiplicativo, só na borda da tecla) ---
    if (input.speedUp && !prevSpeedUp_) setSpeed(speed_ * params_.speedStepFactor);
    if (input.speedDown && !prevSpeedDown_) setSpeed(speed_ / params_.speedStepFactor);
    prevSpeedUp_ = input.speedUp;
    prevSpeedDown_ = input.speedDown;

    // --- 2) Mouse-look: atualiza yaw/pitch quando o botão direito está ativo ---
    if (input.looking && (input.mouseDx != 0.0 || input.mouseDy != 0.0)) {
        // mouseDx > 0 (mouse p/ direita) → yaw aumenta (gira p/ direita).
        // mouseDy > 0 (mouse p/ baixo)   → pitch diminui (olha p/ baixo).
        const float newYaw =
            camera.yaw() + static_cast<float>(input.mouseDx * params_.mouseSensitivity);
        const float newPitch =
            camera.pitch() - static_cast<float>(input.mouseDy * params_.mouseSensitivity);
        camera.setYawPitch(newYaw, newPitch);  // a Camera clampa o pitch a ±89°.
    }

    // --- 3) Movimento WASD + subir/descer (proporcional a dt e à velocidade) ---
    const glm::vec3 fwd = camera.forward();
    const glm::vec3 rgt = camera.right();
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

    glm::vec3 move(0.0f);
    if (input.forward) move += fwd;
    if (input.back)    move -= fwd;
    if (input.right)   move += rgt;
    if (input.left)    move -= rgt;
    if (input.up)      move += worldUp;
    if (input.down)    move -= worldUp;

    glm::vec3 newPos = camera.position();
    if (glm::dot(move, move) > 0.0f) {  // há alguma direção pressionada.
        move = glm::normalize(move);     // diagonal não anda mais rápido.
        newPos += move * static_cast<float>(speed_ * dt);
    }

    // --- 4) Dolly pelo scroll: avança/recua ao longo do forward ---------------
    // Passo proporcional à velocidade atual → fino perto, largo longe (log-scale).
    if (input.scrollDelta != 0.0) {
        const double step = input.scrollDelta * params_.dollyFactor * speed_;
        newPos += fwd * static_cast<float>(step);
    }

    // Aplica a nova posição mantendo a orientação (recomputa o target interno).
    camera.setPosition(newPos);
    camera.setYawPitch(camera.yaw(), camera.pitch());
}

}  // namespace starlag::render
