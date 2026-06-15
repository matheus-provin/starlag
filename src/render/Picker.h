// ============================================================================
//  Picker — seleção de estrela por raio do mouse (T2.4).
//
//  Converte um pixel da tela num raio no espaço do mundo (desprojeção via a
//  inversa da matriz view-projection) e encontra a estrela mais próxima desse
//  raio. C++ puro sobre GLM: nenhuma dependência de Metal/ObjC → testável
//  headless com raios sintéticos.
//
//  Critério de seleção: menor distância PERPENDICULAR do ponto da estrela ao
//  raio, exigindo (a) que a estrela esteja À FRENTE da câmera (t ≥ 0) e (b) que
//  a distância angular fique dentro de um limiar (o usuário não precisa acertar
//  o pixel exato). Empate é desfeito pela estrela mais próxima ao longo do raio.
// ============================================================================

#pragma once

#include "data/Star.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <vector>

namespace starlag::render {

// Raio no espaço do mundo: origin + t*dir, t ≥ 0. `dir` é unitário.
struct Ray {
    glm::vec3 origin{0.0f};
    glm::vec3 dir{0.0f, 0.0f, -1.0f};
};

// Resultado de uma seleção. `hit == false` quando nada caiu dentro do limiar.
struct PickResult {
    bool hit = false;
    size_t index = 0;        // índice na lista de estrelas (se hit).
    double perpDistance = 0; // distância perpendicular ao raio (parsecs).
    double alongRay = 0;     // distância ao longo do raio até o ponto mais próximo.
};

// Constrói o raio do mundo a partir de um pixel da tela.
//   mouseX/mouseY: posição do cursor em pixels (origem no canto SUPERIOR esq.,
//                  como o GLFW reporta). viewW/viewH: tamanho da viewport.
//   invViewProj:   inversa de (projection*view). camPos: posição da câmera.
// O raio parte de camPos na direção do pixel desprojetado.
Ray screenPointToRay(double mouseX, double mouseY, int viewW, int viewH,
                     const glm::mat4& invViewProj, const glm::vec3& camPos);

// Distância perpendicular de um ponto a um raio (t ≥ 0). Retorna também, por
// ponteiro, o parâmetro t do ponto mais próximo (>= 0). Exposta para teste.
double pointRayDistance(const glm::vec3& point, const Ray& ray, double* tOut = nullptr);

// Encontra a estrela mais próxima do raio dentro de `angularThresholdRad`
// (limiar do ângulo entre o raio e a direção câmera→estrela; um limiar angular
// é correto porque estrelas distantes aparecem menores na tela). Empate pela
// mais próxima ao longo do raio. `maxRangePc` limita o alcance (0 = sem limite).
PickResult pickNearestStar(const Ray& ray, const std::vector<data::Star>& stars,
                           double angularThresholdRad, double maxRangePc = 0.0);

}  // namespace starlag::render
