// ============================================================================
//  test_picker — testes do ray-casting de seleção + StarInfo (T2.4).
//
//  Picker é C++ puro (GLM), validável headless: desprojeção pixel→raio (via a
//  inversa da view-projection da Camera), distância ponto-raio, e seleção da
//  estrela mais próxima por ângulo. StarInfo: nome de exibição e luminosidade.
// ============================================================================

#include "render/Camera.h"
#include "render/Picker.h"
#include "render/StarInfo.h"
#include "data/Star.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace starlag::render;
namespace data = starlag::data;

namespace {

int g_failures = 0;

void expectTrue(const char* name, bool ok) {
    std::printf("  [%s] %s\n", ok ? "PASS" : "FALHA", name);
    if (!ok) ++g_failures;
}

void expectNear(const char* name, double got, double want, double tol = 1e-3) {
    const double diff = std::fabs(got - want);
    const double scale = std::fmax(1.0, std::fabs(want));
    const bool ok = diff <= tol * scale;
    std::printf("  [%s] %s  (got=%.5f, want=%.5f)\n", ok ? "PASS" : "FALHA", name, got, want);
    if (!ok) ++g_failures;
}

data::Star starAt(int64_t id, double x, double y, double z) {
    data::Star s;
    s.id = id; s.x = x; s.y = y; s.z = z;
    return s;
}

// --- Distância ponto-raio ---------------------------------------------------
void testPointRayDistance() {
    std::printf("[distancia ponto-raio]\n");
    Ray ray;
    ray.origin = glm::vec3(0, 0, 10);
    ray.dir = glm::vec3(0, 0, -1);

    double t = 0;
    // Estrela na origem: diretamente à frente, t=10, perp=0.
    expectNear("origem: perp 0", pointRayDistance(glm::vec3(0, 0, 0), ray, &t), 0.0);
    expectNear("origem: t=10", t, 10.0);

    // Estrela em (1,0,0): perp=1, t=10.
    expectNear("(1,0,0): perp 1", pointRayDistance(glm::vec3(1, 0, 0), ray, &t), 1.0);
    expectNear("(1,0,0): t=10", t, 10.0);

    // Estrela atrás da câmera (0,0,11): t clampa em 0.
    pointRayDistance(glm::vec3(0, 0, 11), ray, &t);
    expectNear("atras: t=0", t, 0.0);
}

// --- pickNearestStar: seleção por ângulo ------------------------------------
void testPickNearest() {
    std::printf("[pickNearestStar]\n");
    Ray ray;
    ray.origin = glm::vec3(0, 0, 10);
    ray.dir = glm::vec3(0, 0, -1);

    std::vector<data::Star> stars = {
        starAt(1, 0.0, 0.0, 0.0),   // bem no raio (ângulo 0).
        starAt(2, 1.0, 0.0, 0.0),   // ângulo ~5.7°.
        starAt(3, 0.0, 0.5, 0.0),   // ângulo ~2.9°.
        starAt(4, 0.0, 0.0, 11.0),  // atrás da câmera.
    };

    // Limiar amplo (10°): a mais alinhada (id 1, ângulo 0) vence.
    PickResult r = pickNearestStar(ray, stars, glm::radians(10.0));
    expectTrue("acertou alguma", r.hit);
    expectTrue("selecionou id 1 (mais alinhada)", r.hit && stars[r.index].id == 1);

    // Limiar apertado (1°): só a id 1 (ângulo 0) passa.
    PickResult tight = pickNearestStar(ray, stars, glm::radians(1.0));
    expectTrue("limiar apertado seleciona id 1", tight.hit && stars[tight.index].id == 1);

    // Remove a id 1: agora a id 3 (~2.9°) é a melhor dentro de 10°.
    std::vector<data::Star> noCenter = {stars[1], stars[2], stars[3]};
    PickResult r3 = pickNearestStar(ray, noCenter, glm::radians(10.0));
    expectTrue("sem a central, vence id 3", r3.hit && noCenter[r3.index].id == 3);

    // Estrela atrás nunca é selecionada (raio só vê à frente).
    std::vector<data::Star> behindOnly = {starAt(9, 0, 0, 11)};
    PickResult none = pickNearestStar(ray, behindOnly, glm::radians(45.0));
    expectTrue("estrela atras nao e selecionada", !none.hit);

    // Limiar zero / nenhuma alinhada → sem hit.
    std::vector<data::Star> off = {starAt(5, 5.0, 0.0, 0.0)};
    PickResult miss = pickNearestStar(ray, off, glm::radians(1.0));
    expectTrue("nada dentro do limiar → miss", !miss.hit);
}

// --- screenPointToRay: centro da tela aponta para o alvo --------------------
void testScreenPointToRay() {
    std::printf("[screenPointToRay]\n");
    Camera cam;
    const glm::vec3 camPos(0, 0, 10);
    cam.setPosition(camPos);
    cam.setYawPitch(0.0f, 0.0f);  // olhando -Z.
    cam.setPerspective(glm::radians(60.0f), 1.0f, 0.1f, 100.0f);

    const glm::mat4 invVP = glm::inverse(cam.viewProjection());

    // Pixel no CENTRO da tela (640,360 de 1280x720) → raio para -Z.
    Ray center = screenPointToRay(640.0, 360.0, 1280, 720, invVP, camPos);
    expectNear("centro: dir.x ~ 0", center.dir.x, 0.0, 1e-4);
    expectNear("centro: dir.y ~ 0", center.dir.y, 0.0, 1e-4);
    expectNear("centro: dir.z ~ -1", center.dir.z, -1.0, 1e-4);
    expectTrue("origem do raio = camera", center.origin == camPos);

    // O raio do centro deve selecionar uma estrela na origem (à frente).
    std::vector<data::Star> stars = {starAt(1, 0, 0, 0)};
    PickResult r = pickNearestStar(center, stars, glm::radians(5.0));
    expectTrue("centro seleciona estrela a frente", r.hit && stars[r.index].id == 1);

    // Pixel no canto superior direito deve apontar para +x, +y.
    Ray corner = screenPointToRay(1280.0, 0.0, 1280, 720, invVP, camPos);
    expectTrue("canto sup-dir: dir.x > 0", corner.dir.x > 0.0f);
    expectTrue("canto sup-dir: dir.y > 0", corner.dir.y > 0.0f);
}

// --- StarInfo: nome e luminosidade ------------------------------------------
void testStarInfo() {
    std::printf("[StarInfo]\n");
    // Vega-like: nome próprio + designação + lum do catálogo.
    data::Star vega;
    vega.id = 91262; vega.proper = "Vega"; vega.bayer = "Alp"; vega.con = "Lyr";
    vega.absmag = 0.58; vega.lum = 49.93; vega.hasLum = true;
    vega.distPc = 7.6787;
    expectTrue("displayName usa nome proprio", displayName(vega) == "Vega");
    expectNear("luminosidade do catalogo", luminositySolar(vega), 49.93, 1e-6);

    // Sem nome próprio: cai para Bayer + constelação.
    data::Star desig;
    desig.id = 1; desig.bayer = "Bet"; desig.con = "Ori";
    expectTrue("displayName Bayer+con", displayName(desig) == "Bet Ori");

    // Sem designação: identificador de catálogo.
    data::Star anon;
    anon.id = 5; anon.hip = 12345;
    expectTrue("displayName HIP", displayName(anon) == "HIP 12345");

    // Luminosidade estimada da magnitude absoluta (sem lum do catálogo).
    data::Star sol;
    sol.absmag = 4.83; sol.hasLum = false;
    expectNear("Sol L ~ 1 (de absmag)", luminositySolar(sol), 1.0, 1e-3);

    // formatStarInfo com origem inclui a distância entre as duas.
    data::Star origin = starAt(0, 0, 0, 0);
    data::Star dest = starAt(1, 3, 4, 0);  // 5 pc da origem.
    const std::string info = formatStarInfo(dest, &origin);
    expectTrue("info menciona distancia da origem",
               info.find("ate aqui") != std::string::npos);
}

}  // namespace

int main() {
    std::printf("== starlag T2.4 — testes de picking + StarInfo ==\n");

    testPointRayDistance();
    testPickNearest();
    testScreenPointToRay();
    testStarInfo();

    if (g_failures == 0) {
        std::printf("\nTODOS os testes passaram.\n");
        return 0;
    }
    std::printf("\n%d teste(s) FALHARAM.\n", g_failures);
    return 1;
}
