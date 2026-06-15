// ============================================================================
//  test_flight_profile — testes do perfil de voo acelerado (T3.2).
//
//  Valores de referência derivados e verificados independentemente em Python
//  (modelo do "foguete relativístico"). Sem framework: asserções com tolerância
//  e exit code para o CTest.
//
//  Referências confirmadas (1 g = 1.032295 ly/yr², c=1 ly/yr):
//    - Vega 25.04 ly @ 1g, cap β=0.9999999 → TRIANGULAR:
//        τ ≈ 6.44291 yr, t ≈ 26.90777 yr, γ_pico ≈ 13.92434, β_pico ≈ 0.997418
//    - 1000 ly @ 1g, cap β=0.99 → TRAPEZOIDAL (coast):
//        τ ≈ 145.93906 yr, t ≈ 1011.78194 yr, γ_pico = 7.0888120
//    - 1 ly @ 1g (cap alto) → TRIANGULAR:
//        τ ≈ 1.89234 yr, t ≈ 2.20791 yr, γ_pico ≈ 1.516148
// ============================================================================

#include "physics/Constants.h"
#include "physics/FlightProfile.h"

#include <cmath>
#include <cstdio>

using namespace starlag::physics;

namespace {

int g_failures = 0;

void expectNear(const char* name, double got, double want, double tol = 1e-5) {
    const double diff = std::fabs(got - want);
    const double scale = std::fmax(1.0, std::fabs(want));
    const bool ok = diff <= tol * scale;
    std::printf("  [%s] %s  (got=%.10g, want=%.10g)\n",
                ok ? "PASS" : "FALHA", name, got, want);
    if (!ok) ++g_failures;
}

void expectTrue(const char* name, bool ok) {
    std::printf("  [%s] %s\n", ok ? "PASS" : "FALHA", name);
    if (!ok) ++g_failures;
}

// --- Conversão de unidades: 1 g em ly/yr² -----------------------------------
void testOneG() {
    std::printf("[1 g em ly/yr2]\n");
    expectNear("kOneG_ly_yr2 ~1.032295", kOneG_ly_yr2, 1.032295, 1e-5);
    expectNear("gToLyPerYr2(1)=kOneG", gToLyPerYr2(1.0), kOneG_ly_yr2);
    expectNear("gToLyPerYr2(2)=2g", gToLyPerYr2(2.0), 2.0 * kOneG_ly_yr2);
}

// --- Viagem a Vega a 1g (perfil triangular) ---------------------------------
void testVega1g() {
    std::printf("[Vega 25.04ly @1g -> triangular]\n");
    const double a = gToLyPerYr2(1.0);
    FlightProfileResult r = computeAcceleratedTrip(25.04, a, 0.9999999);

    expectTrue("kind == Triangular", r.kind == FlightKind::Triangular);
    expectNear("tempo proprio ~6.44291 yr", r.properTimeYr, 6.44291127, 1e-5);
    expectNear("tempo coordenado ~26.90777 yr", r.coordinateTimeYr, 26.9077703, 1e-5);
    expectNear("gamma de pico ~13.92434", r.peakGamma, 13.9243368, 1e-5);
    expectNear("beta de pico ~0.997418", r.peakBeta, 0.9974178466, 1e-6);
    expectNear("sem coast", r.coastDistanceLy, 0.0);
    expectTrue("tau < t", r.properTimeYr < r.coordinateTimeYr);
    // Sanidade: as duas metades somam a distância total.
    expectNear("2x meia-distancia = D", 2.0 * r.accelDistanceLy, 25.04);
}

// --- Viagem longa a 1g com cap 0.99 (perfil trapezoidal/coast) --------------
void testLong1gCoast() {
    std::printf("[1000ly @1g cap0.99 -> trapezoidal]\n");
    const double a = gToLyPerYr2(1.0);
    FlightProfileResult r = computeAcceleratedTrip(1000.0, a, 0.99);

    expectTrue("kind == Trapezoidal", r.kind == FlightKind::Trapezoidal);
    expectNear("tempo proprio ~145.93906 yr", r.properTimeYr, 145.939056, 1e-4);
    expectNear("tempo coordenado ~1011.78194 yr", r.coordinateTimeYr, 1011.78194, 1e-4);
    expectNear("gamma pico = gamma(0.99)", r.peakGamma, 7.0888120501, 1e-6);
    expectNear("beta de pico = 0.99 (cap)", r.peakBeta, 0.99);
    expectTrue("ha coast (>0)", r.coastDistanceLy > 0.0);
    // Conservação de distância: 2·accel + coast = D.
    expectNear("2*accel + coast = D",
               2.0 * r.accelDistanceLy + r.coastDistanceLy, 1000.0, 1e-6);
}

// --- Viagem curta 1 ly a 1g (triangular) ------------------------------------
void testShort1ly() {
    std::printf("[1ly @1g -> triangular]\n");
    const double a = gToLyPerYr2(1.0);
    FlightProfileResult r = computeAcceleratedTrip(1.0, a, 0.9999999);

    expectTrue("kind == Triangular", r.kind == FlightKind::Triangular);
    expectNear("tempo proprio ~1.89234 yr", r.properTimeYr, 1.8923438, 1e-5);
    expectNear("tempo coordenado ~2.20791 yr", r.coordinateTimeYr, 2.2079086, 1e-5);
    expectNear("gamma pico ~1.516148", r.peakGamma, 1.5161476, 1e-5);
}

// --- Casos de borda ----------------------------------------------------------
void testEdges() {
    std::printf("[bordas]\n");
    const double a = gToLyPerYr2(1.0);

    FlightProfileResult zeroDist = computeAcceleratedTrip(0.0, a, 0.99);
    expectNear("dist=0: tau=0", zeroDist.properTimeYr, 0.0);
    expectNear("dist=0: t=0", zeroDist.coordinateTimeYr, 0.0);

    FlightProfileResult zeroAcc = computeAcceleratedTrip(10.0, 0.0, 0.99);
    expectNear("a=0: tau=0", zeroAcc.properTimeYr, 0.0);
    expectTrue("a=0: finito", std::isfinite(zeroAcc.properTimeYr));

    // Consistência física geral: tripulação envelhece menos.
    FlightProfileResult any = computeAcceleratedTrip(50.0, a, 0.95);
    expectTrue("geral: tau <= t", any.properTimeYr <= any.coordinateTimeYr);
    expectTrue("geral: tempos finitos",
               std::isfinite(any.properTimeYr) && std::isfinite(any.coordinateTimeYr));
}

}  // namespace

int main() {
    std::printf("== starlag T3.2 — testes de perfil de voo acelerado ==\n");

    testOneG();
    testVega1g();
    testLong1gCoast();
    testShort1ly();
    testEdges();

    if (g_failures == 0) {
        std::printf("\nTODOS os testes passaram.\n");
        return 0;
    }
    std::printf("\n%d teste(s) FALHARAM.\n", g_failures);
    return 1;
}
