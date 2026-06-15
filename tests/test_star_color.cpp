// ============================================================================
//  test_star_color — testes da cor de estrelas (T2.2).
//
//  Valida headless o pipeline B−V → temperatura → RGB contra estrelas conhecidas
//  (referências calculadas independentemente em Python). Confere a tendência
//  física: quentes (B−V baixo) azuladas, frias (B−V alto) avermelhadas.
// ============================================================================

#include "render/StarColor.h"
#include "data/Star.h"

#include <cmath>
#include <cstdio>

using namespace starlag::render;

namespace {

int g_failures = 0;

void expectNear(const char* name, double got, double want, double tol = 1e-3) {
    const double diff = std::fabs(got - want);
    const double scale = std::fmax(1.0, std::fabs(want));
    const bool ok = diff <= tol * scale;
    std::printf("  [%s] %s  (got=%.4f, want=%.4f)\n", ok ? "PASS" : "FALHA", name, got, want);
    if (!ok) ++g_failures;
}

void expectTrue(const char* name, bool ok) {
    std::printf("  [%s] %s\n", ok ? "PASS" : "FALHA", name);
    if (!ok) ++g_failures;
}

// --- B−V → temperatura (Ballesteros) ----------------------------------------
void testTemperature() {
    std::printf("[B-V -> temperatura]\n");
    expectNear("Vega bv=-0.001 -> ~10138K", colorIndexToTemperature(-0.001), 10138.0, 1e-2);
    expectNear("Sol bv=0.656 -> ~5757K", colorIndexToTemperature(0.656), 5757.0, 1e-2);
    expectNear("Betelgeuse bv=1.85 -> ~3333K", colorIndexToTemperature(1.85), 3333.0, 1e-2);
    // Monotonicidade: B−V maior → temperatura menor.
    expectTrue("mais azul = mais quente",
               colorIndexToTemperature(-0.3) > colorIndexToTemperature(1.5));
}

// --- temperatura → RGB ------------------------------------------------------
void testTemperatureToRgb() {
    std::printf("[temperatura -> RGB]\n");
    Rgb hot = temperatureToRgb(30000.0);   // O/B azul.
    expectTrue("30000K azulada (b>r)", hot.b > hot.r);
    expectTrue("30000K b saturado", hot.b > 0.99f);

    Rgb sun = temperatureToRgb(5778.0);     // G branca-quente.
    expectNear("Sol r=1.0", sun.r, 1.0, 1e-2);
    expectNear("Sol g~0.951", sun.g, 0.951, 2e-2);
    expectNear("Sol b~0.904", sun.b, 0.904, 2e-2);

    Rgb cold = temperatureToRgb(3000.0);    // M vermelha.
    expectTrue("3000K avermelhada (r>b)", cold.r > cold.b);
    expectTrue("3000K r saturado", cold.r > 0.99f);

    // Faixa: valores extremos são limitados (sem NaN/negativos).
    Rgb clamped = temperatureToRgb(100.0);
    expectTrue("temp baixa limitada (sem NaN)",
               std::isfinite(clamped.r) && clamped.r >= 0.0f && clamped.r <= 1.0f);
}

// --- starColor: escolha de fonte (ci > spect > fallback) --------------------
void testStarColor() {
    std::printf("[starColor: ci vs spect vs fallback]\n");
    using starlag::data::Star;

    // Vega-like: usa ci → azul-branca.
    Star vega;
    vega.ci = -0.001; vega.hasCi = true; vega.spect = "A0Vvar";
    Rgb cv = starColor(vega);
    expectTrue("Vega (ci) azulada (b>=r)", cv.b >= cv.r);

    // Sem ci, mas com spect 'M' → vermelha.
    Star m;
    m.hasCi = false; m.spect = "M2Iab";
    Rgb cm = starColor(m);
    expectTrue("M (spect) avermelhada (r>b)", cm.r > cm.b);

    // Sem ci, mas com spect 'O' → azul.
    Star o;
    o.hasCi = false; o.spect = "O5V";
    Rgb co = starColor(o);
    expectTrue("O (spect) azulada (b>r)", co.b > co.r);

    // Sem ci e sem spect → fallback solar (válido, não-NaN).
    Star unknown;
    unknown.hasCi = false; unknown.spect = "";
    Rgb cu = starColor(unknown);
    expectTrue("fallback solar valido",
               std::isfinite(cu.r) && cu.r > 0.5f);

    // spectralClassToTemperature: ordem OBAFGKM decrescente.
    expectTrue("O mais quente que M",
               spectralClassToTemperature('O') > spectralClassToTemperature('M'));
    expectTrue("classe desconhecida = solar",
               std::fabs(spectralClassToTemperature('?') - 5778.0) < 1.0);
}

}  // namespace

int main() {
    std::printf("== starlag T2.2 — testes da cor de estrelas ==\n");

    testTemperature();
    testTemperatureToRgb();
    testStarColor();

    if (g_failures == 0) {
        std::printf("\nTODOS os testes passaram.\n");
        return 0;
    }
    std::printf("\n%d teste(s) FALHARAM.\n", g_failures);
    return 1;
}
