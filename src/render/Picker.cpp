// ============================================================================
//  Picker.cpp — implementação do ray-casting de seleção (T2.4).
// ============================================================================

#include "render/Picker.h"

#include <cmath>

namespace starlag::render {

Ray screenPointToRay(double mouseX, double mouseY, int viewW, int viewH,
                     const glm::mat4& invViewProj, const glm::vec3& camPos) {
    Ray ray;
    ray.origin = camPos;

    if (viewW <= 0 || viewH <= 0) {
        return ray;  // viewport degenerada; raio padrão (frente).
    }

    // Pixel → NDC. x: [-1,1] da esquerda p/ direita. y: o mouse tem origem no
    // topo (GLFW), enquanto o NDC tem +y para cima, então invertemos y.
    const float ndcX = static_cast<float>(2.0 * mouseX / viewW - 1.0);
    const float ndcY = static_cast<float>(1.0 - 2.0 * mouseY / viewH);

    // Um ponto no far plane (z=1 na convenção Metal [0,1]) desprojetado ao mundo.
    glm::vec4 farClip(ndcX, ndcY, 1.0f, 1.0f);
    glm::vec4 farWorld = invViewProj * farClip;
    if (std::fabs(farWorld.w) > 1e-9f) {
        farWorld /= farWorld.w;  // divisão de perspectiva.
    }

    const glm::vec3 dir = glm::vec3(farWorld) - camPos;
    const float len = glm::length(dir);
    if (len > 1e-12f) {
        ray.dir = dir / len;  // normaliza.
    }
    return ray;
}

double pointRayDistance(const glm::vec3& point, const Ray& ray, double* tOut) {
    // Projeta (point - origin) sobre dir para achar o t do ponto mais próximo.
    const glm::vec3 toPoint = point - ray.origin;
    double t = static_cast<double>(glm::dot(toPoint, ray.dir));  // dir é unitário.
    if (t < 0.0) t = 0.0;  // o ponto mais próximo não pode ficar atrás da origem.
    if (tOut) *tOut = t;

    const glm::vec3 closest = ray.origin + ray.dir * static_cast<float>(t);
    return static_cast<double>(glm::length(point - closest));
}

PickResult pickNearestStar(const Ray& ray, const std::vector<data::Star>& stars,
                           double angularThresholdRad, double maxRangePc) {
    PickResult best;
    double bestAngle = angularThresholdRad;  // só aceita ângulos ≤ limiar.

    for (size_t i = 0; i < stars.size(); ++i) {
        const data::Star& s = stars[i];
        const glm::vec3 p(static_cast<float>(s.x), static_cast<float>(s.y),
                          static_cast<float>(s.z));

        double t = 0.0;
        const double perp = pointRayDistance(p, ray, &t);

        // Precisa estar à frente da câmera (t > 0) e dentro do alcance.
        if (t <= 1e-6) continue;
        if (maxRangePc > 0.0 && t > maxRangePc) continue;

        // Ângulo aproximado entre o raio e a direção câmera→estrela: atan(perp/t).
        const double angle = std::atan2(perp, t);
        if (angle > bestAngle) continue;

        // Aceita: prioriza menor ângulo; empate (mesmo ângulo) pela mais próxima.
        if (!best.hit || angle < bestAngle ||
            (angle == bestAngle && t < best.alongRay)) {
            best.hit = true;
            best.index = i;
            best.perpDistance = perp;
            best.alongRay = t;
            bestAngle = angle;
        }
    }
    return best;
}

}  // namespace starlag::render
