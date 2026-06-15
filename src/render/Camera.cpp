// ============================================================================
//  Camera.cpp — implementação da câmera 3D (T2.1).
//
//  Importante: definimos GLM_FORCE_DEPTH_ZERO_TO_ONE ANTES de incluir os headers
//  de transformação. O Metal usa profundidade de clip-space em [0, 1] (como o
//  Direct3D), enquanto o default do GLM é o de OpenGL, [-1, 1]. Sem este define,
//  metade do alcance de profundidade ficaria além do far plane do Metal e a
//  cena pareceria "cortada". (O mesmo define precisa valer onde quer que estas
//  matrizes sejam geradas; aqui é o único lugar.)
// ============================================================================

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "render/Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace starlag::render {

void Camera::lookAt(const glm::vec3& eye, const glm::vec3& target,
                    const glm::vec3& up) {
    eye_ = eye;
    target_ = target;
    up_ = up;
}

void Camera::setPerspective(float fovYRadians, float aspect, float nearZ,
                            float farZ) {
    fovY_ = fovYRadians;
    aspect_ = aspect;
    nearZ_ = nearZ;
    farZ_ = farZ;
}

void Camera::setAspect(float aspect) { aspect_ = aspect; }

void Camera::setYawPitch(float yawRadians, float pitchRadians) {
    yaw_ = yawRadians;
    // Limita o pitch a ±89° para evitar gimbal flip (olhar exatamente para os
    // polos degenera o vetor "up" do lookAt).
    const float limit = glm::radians(89.0f);
    pitch_ = glm::clamp(pitchRadians, -limit, limit);
    // Mantém target_ coerente com a nova direção, para viewMatrix() continuar
    // usando o lookAt(eye, target) sem precisar de um caminho separado.
    target_ = eye_ + forward();
}

glm::vec3 Camera::forward() const {
    // Convenção: yaw=0, pitch=0 → olhar para −Z (frente padrão da câmera).
    // yaw gira em torno de +Y; pitch inclina em torno do eixo "right".
    const float cp = std::cos(pitch_);
    return glm::normalize(glm::vec3(-std::sin(yaw_) * cp,
                                    std::sin(pitch_),
                                    -std::cos(yaw_) * cp));
}

glm::vec3 Camera::right() const {
    // right = forward × worldUp, normalizado (perpendicular ao plano vertical).
    return glm::normalize(glm::cross(forward(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::vec3 Camera::up() const {
    return glm::normalize(glm::cross(right(), forward()));
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(eye_, target_, up_);
}

glm::mat4 Camera::projectionMatrix() const {
    // glm::perspective respeita GLM_FORCE_DEPTH_ZERO_TO_ONE (clip Z em [0,1]).
    return glm::perspective(fovY_, aspect_, nearZ_, farZ_);
}

glm::mat4 Camera::viewProjection() const {
    return projectionMatrix() * viewMatrix();
}

}  // namespace starlag::render
