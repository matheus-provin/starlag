// ============================================================================
//  test_star_field — testes da montagem do buffer de instâncias (T2.2).
//
//  Sintéticos (sizing por magnitude, layout, cor) + catálogo HYG real (contagem
//  ~120k, posições copiadas, tamanhos na faixa configurada).
// ============================================================================

#include "render/StarField.h"
#include "data/CatalogParser.h"
#include "data/Star.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

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
    std::printf("  [%s] %s  (got=%.4f, want=%.4f)\n", ok ? "PASS" : "FALHA", name, got, want);
    if (!ok) ++g_failures;
}

#ifndef STARLAG_TEST_CATALOG
#define STARLAG_TEST_CATALOG ""
#endif

// --- magnitudeToSize: monotonicidade e saturação ----------------------------
void testSizing() {
    std::printf("[magnitude -> tamanho]\n");
    StarFieldParams p;  // brightMag=0, faintMag=8, minSize=1.5, maxSize=9.

    // Brilhante (mag<=0) → maxSize; fraca (mag>=8) → minSize.
    expectNear("mag 0 -> maxSize", magnitudeToSize(0.0, p), p.maxSize, 1e-3);
    expectNear("mag 8 -> minSize", magnitudeToSize(8.0, p), p.minSize, 1e-3);
    // Saturação fora da faixa.
    expectNear("mag -5 (super brilhante) -> maxSize", magnitudeToSize(-5.0, p), p.maxSize, 1e-3);
    expectNear("mag 15 (muito fraca) -> minSize", magnitudeToSize(15.0, p), p.minSize, 1e-3);
    // Ponto médio (mag 4) → meio entre min e max.
    expectNear("mag 4 -> meio", magnitudeToSize(4.0, p),
               (p.maxSize + p.minSize) / 2.0f, 1e-2);
    // Monotonicidade: mais brilhante = maior.
    expectTrue("mag 1 maior que mag 5",
               magnitudeToSize(1.0, p) > magnitudeToSize(5.0, p));
}

// --- buildStarField: layout e cor -------------------------------------------
void testBuild() {
    std::printf("[buildStarField sintetico]\n");
    std::vector<starlag::data::Star> stars;

    starlag::data::Star vega;
    vega.x = 1.0; vega.y = 2.0; vega.z = 3.0;
    vega.ci = -0.001; vega.hasCi = true; vega.mag = 0.03;
    stars.push_back(vega);

    starlag::data::Star faint;
    faint.x = -5.0; faint.y = 0.0; faint.z = 4.0;
    faint.ci = 1.5; faint.hasCi = true; faint.mag = 7.0;
    stars.push_back(faint);

    std::vector<StarInstance> field = buildStarField(stars);
    expectTrue("2 instancias", field.size() == 2);

    // Posições copiadas fielmente (parsecs).
    expectNear("Vega px", field[0].px, 1.0);
    expectNear("Vega py", field[0].py, 2.0);
    expectNear("Vega pz", field[0].pz, 3.0);

    // Vega (brilhante, mag 0.03) maior que a fraca (mag 7).
    expectTrue("Vega maior que a fraca", field[0].size > field[1].size);

    // Vega azul-branca: componente azul >= vermelho.
    expectTrue("Vega azulada (cb>=cr)", field[0].cb >= field[0].cr);
    // A fraca (ci=1.5) é alaranjada/avermelhada: vermelho > azul.
    expectTrue("fraca avermelhada (cr>cb)", field[1].cr > field[1].cb);

    // Layout: StarInstance tem 7 floats contíguos (cabe num MTLBuffer direto).
    expectTrue("StarInstance = 7 floats", sizeof(StarInstance) == 7 * sizeof(float));
}

// --- Catálogo HYG real ------------------------------------------------------
void testRealCatalog() {
    std::printf("[catalogo HYG real]\n");
    const std::string path = STARLAG_TEST_CATALOG;
    if (path.empty()) {
        std::printf("  (STARLAG_TEST_CATALOG nao definido; pulando)\n");
        return;
    }
    starlag::data::ParseReport rep = starlag::data::parseCatalogFile(path);
    expectTrue("catalogo ok", rep.ok);
    if (!rep.ok) return;

    std::vector<StarInstance> field = buildStarField(rep.stars);
    expectTrue("instancias = nº de estrelas", field.size() == rep.stars.size());
    expectTrue("~120k instancias", field.size() > 119000 && field.size() < 120000);

    // Todas as cores e tamanhos válidos (sem NaN, na faixa).
    StarFieldParams p;
    bool allValid = true;
    for (const StarInstance& s : field) {
        if (!std::isfinite(s.px) || !std::isfinite(s.cr) || !std::isfinite(s.size)) {
            allValid = false; break;
        }
        if (s.cr < 0 || s.cr > 1 || s.size < p.minSize - 1e-3f || s.size > p.maxSize + 1e-3f) {
            allValid = false; break;
        }
    }
    expectTrue("todas instancias validas (cor[0,1], tamanho na faixa)", allValid);
}

}  // namespace

int main() {
    std::printf("== starlag T2.2 — testes do campo de estrelas ==\n");

    testSizing();
    testBuild();
    testRealCatalog();

    if (g_failures == 0) {
        std::printf("\nTODOS os testes passaram.\n");
        return 0;
    }
    std::printf("\n%d teste(s) FALHARAM.\n", g_failures);
    return 1;
}
