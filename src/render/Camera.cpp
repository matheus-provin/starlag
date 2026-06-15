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
