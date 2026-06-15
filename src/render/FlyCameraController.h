// ============================================================================
//  FlyCameraController — navegação livre 6-DOF da câmera (T2.3).
//
//  Consome um InputState + dt e atualiza uma Camera (posição e orientação
//  yaw/pitch). C++ puro sobre GLM → 100% testável headless.
//
//  Modelo de navegação (REQUIREMENTS §7):
//    - WASD move no referencial da câmera; up/down sobem/descem (eixo Y mundo).
//    - Mouse (botão direito) gira yaw/pitch; pitch é limitado a ±89° na Camera.
//    - Roda do mouse faz DOLLY: avança/recua ao longo do forward (passo
//      proporcional à velocidade atual → fino perto, largo longe).
//    - Teclas dedicadas ajustam a velocidade de voo em passos MULTIPLICATIVOS
//      (escala logarítmica): essencial para cobrir da vizinhança solar a
//      dezenas de parsecs sem trocar de controle.
//    - Movimento proporcional a dt → independente de frame rate.
// ============================================================================

#pragma once

#include "render/Camera.h"
#include "render/InputState.h"

namespace starlag::render {

struct FlyControllerParams {
    double moveSpeed = 5.0;        // velocidade base de voo (parsecs/segundo).
    double minSpeed = 0.05;        // limites do ajuste de velocidade.
    double maxSpeed = 500.0;
    double speedStepFactor = 1.5;  // multiplicador por "tique" de speedUp/Down.
    double mouseSensitivity = 0.0025;  // radianos por pixel de mouse.
    double dollyFactor = 0.15;     // fração da velocidade aplicada por unidade de scroll.
};

class FlyCameraController {
public:
    FlyCameraController() = default;
    explicit FlyCameraController(const FlyControllerParams& params) : params_(params) {}

    // Velocidade de voo atual (parsecs/s); ajustável por input.
    double speed() const { return speed_; }
    void setSpeed(double s);

    // Aplica um frame de input à câmera. `dt` em segundos.
    void update(Camera& camera, const InputState& input, double dt);

private:
    FlyControllerParams params_{};
    double speed_ = 5.0;
    // Evita repetição contínua enquanto a tecla de velocidade fica pressionada:
    // só aplica um passo quando a tecla transiciona de solta → pressionada.
    bool prevSpeedUp_ = false;
    bool prevSpeedDown_ = false;
};

}  // namespace starlag::render
