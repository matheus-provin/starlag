// ============================================================================
//  Camera — câmera 3D com matrizes view/projection (T2.1).
//
//  C++ puro sobre GLM: nenhuma dependência de Metal/ObjC, então é 100% testável
//  headless (as matrizes são pura matemática). A camada de render (MetalWindow)
//  consome apenas o produto final viewProjection() como 16 floats column-major,
//  layout que GLM e Metal compartilham — sem vazar GLM para o `.mm`.
//
//  Convenções:
//    - Sistema destro (right-handed), como o GLM por padrão.
//    - Profundidade em clip-space no intervalo [0, 1] (convenção do Metal/D3D),
//      obtida compilando o projeto com GLM_FORCE_DEPTH_ZERO_TO_ONE (ver .cpp).
//    - Posições em parsecs (mesma unidade do catálogo HYG), mas a câmera é
//      agnóstica a unidades.
//
//  Marco 2 (T2.1): câmera "look-at" simples (posição + alvo). A navegação livre
//  6-DOF com input (WASD/mouse) entra na T2.3, reaproveitando esta base.
// ============================================================================

#pragma once

#include <glm/glm.hpp>

namespace starlag::render {

class Camera {
public:
    Camera() = default;

    // --- Configuração da câmera (view) --------------------------------------
    void setPosition(const glm::vec3& eye) { eye_ = eye; }
    void setTarget(const glm::vec3& target) { target_ = target; }
    void setUp(const glm::vec3& up) { up_ = up; }

    // Aponta a câmera de `eye` para `target` de uma vez.
    void lookAt(const glm::vec3& eye, const glm::vec3& target,
                const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));

    // --- Orientação por yaw/pitch (modo fly, T2.3) --------------------------
    // Define a direção de visão por ângulos (radianos). yaw gira em torno do
    // eixo Y do mundo; pitch inclina para cima/baixo. O alvo passa a ser
    // derivado de posição + direção (a câmera deixa de "mirar" um ponto fixo).
    void setYawPitch(float yawRadians, float pitchRadians);
    float yaw() const { return yaw_; }
    float pitch() const { return pitch_; }

    // Vetores de base do referencial da câmera (unitários), derivados de
    // yaw/pitch. `forward` aponta para onde a câmera olha.
    glm::vec3 forward() const;
    glm::vec3 right() const;
    glm::vec3 up() const;

    // --- Configuração da projeção (perspectiva) -----------------------------
    // fovY em radianos; aspect = largura/altura; near/far são distâncias > 0.
    void setPerspective(float fovYRadians, float aspect, float nearZ, float farZ);

    // Atualiza só o aspect (ex.: ao redimensionar a janela), preservando o resto.
    void setAspect(float aspect);

    // --- Acessores ----------------------------------------------------------
    const glm::vec3& position() const { return eye_; }
    const glm::vec3& target() const { return target_; }

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix() const;

    // Produto projection * view (a matriz que vai para o shader como "MVP" de um
    // modelo na identidade). Column-major, pronta para Metal.
    glm::mat4 viewProjection() const;

private:
    // View
    glm::vec3 eye_{0.0f, 0.0f, 5.0f};
    glm::vec3 target_{0.0f, 0.0f, 0.0f};
    glm::vec3 up_{0.0f, 1.0f, 0.0f};

    // Orientação fly (radianos). Mantida em paralelo a eye_/target_: setYawPitch
    // recomputa target_ = eye_ + forward(), então viewMatrix() não muda.
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;

    // Projeção (valores padrão sensatos; sobrescritos por setPerspective).
    float fovY_ = glm::radians(60.0f);
    float aspect_ = 16.0f / 9.0f;
    float nearZ_ = 0.1f;
    float farZ_ = 1000.0f;
};

}  // namespace starlag::render
