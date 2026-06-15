// ============================================================================
//  test_relativity — testes do núcleo de dilatação temporal (T3.1).
//
//  Valida contra casos de referência conhecidos da Relatividade Restrita.
//  Sem framework externo: helpers de asserção com tolerância para `double`,
//  contagem de falhas e exit code (0 = tudo passou) para integrar ao CTest.
//
//  Casos canônicos do fator de Lorentz (γ):
//    β=0      → γ=1
//    β=0.6    → γ=1.25         (clássico de livro: 3-4-5)
//    β=0.8    → γ=5/3 ≈ 1.6667
//    β=0.866… → γ≈2            (√3/2)
//    β=0.99   → γ≈7.0888
//    β=0.999  → γ≈22.3663  (22.36627204, valor exato verificado)
// ============================================================================

#include "physics/Constants.h"
#include "physics/Relativity.h"

#include <cmath>
#include <cstdio>

using namespace starlag::physics;

namespace {

int g_failures = 0;

// Asserção com tolerância relativa+absoluta (robusta para magnitudes variadas).
void expectNear(const char* name, double got, double want, double tol = 1e-9) {
    const double diff = std::fabs(got - want);
    const double scale = std::fmax(1.0, std::fabs(want));
    const bool ok = diff <= tol * scale;
    std::printf("  [%s] %s  (got=%.12g, want=%.12g)\n",
                ok ? "PASS" : "FALHA", name, got, want);
    if (!ok) ++g_failures;
}

void expectTrue(const char* name, bool ok) {
    std::printf("  [%s] %s\n", ok ? "PASS" : "FALHA", name);
    if (!ok) ++g_failures;
}

// --- Fator de Lorentz: casos conhecidos -------------------------------------
void testLorentzFactor() {
    std::printf("[lorentzFactor]\n");
    expectNear("beta=0   -> gamma=1",      lorentzFactor(0.0),   1.0);
    expectNear("beta=0.6 -> gamma=1.25",   lorentzFactor(0.6),   1.25);
    expectNear("beta=0.8 -> gamma=5/3",    lorentzFactor(0.8),   5.0 / 3.0);
    expectNear("beta=0.99  -> gamma~7.09", lorentzFactor(0.99),  7.0888120500, 1e-7);
    expectNear("beta=0.999 -> gamma~22.4", lorentzFactor(0.999), 22.3662720421, 1e-7);

    // γ=2 exatamente quando β=√3/2 (≈0.8660254).
    expectNear("beta=sqrt(3)/2 -> gamma=2",
               lorentzFactor(std::sqrt(3.0) / 2.0), 2.0, 1e-9);
}

// --- Estabilidade numérica perto de c ---------------------------------------
void testNumericalStabilityNearC() {
    std::printf("[estabilidade perto de c]\n");
    // No limite kMaxBeta = 0.9999999, γ deveria ser ~2236.07.
    // γ = 1/√((1-β)(1+β)); para β=0.9999999, 1-β=1e-7, 1+β≈2 → √(2e-7).
    const double g = lorentzFactor(kMaxBeta);
    const double expected = 1.0 / std::sqrt((1.0 - kMaxBeta) * (1.0 + kMaxBeta));
    expectNear("gamma(kMaxBeta) coerente", g, expected, 1e-6);
    expectTrue("gamma(kMaxBeta) finito e grande", std::isfinite(g) && g > 2000.0);
}

// --- clampBeta ---------------------------------------------------------------
void testClampBeta() {
    std::printf("[clampBeta]\n");
    expectNear("negativo -> 0",      clampBeta(-0.5), 0.0);
    expectNear("normal mantem",      clampBeta(0.5),  0.5);
    expectNear("acima do limite satura", clampBeta(0.999999999), kMaxBeta);
    expectNear("NaN -> 0",           clampBeta(std::nan("")), 0.0);
    expectTrue("clamp sempre < 1",   clampBeta(2.0) < 1.0);
}

// --- Relações de ida-e-volta entre tempos -----------------------------------
void testTimeRelations() {
    std::printf("[relacoes de tempo]\n");
    const double gamma = lorentzFactor(0.8);  // 5/3
    const double t = 10.0;                     // 10 anos coordenados
    const double tau = properTimeFromCoordinate(t, gamma);
    expectNear("tau = t/gamma", tau, 10.0 / (5.0 / 3.0));      // 6.0
    expectNear("round-trip t", coordinateTimeFromProper(tau, gamma), t);
}

// --- Viagem completa a velocidade constante ---------------------------------
void testConstVelocityTrip() {
    std::printf("[viagem velocidade constante]\n");

    // Caso 1: 10 ly a β=0.8 (γ=5/3).
    //   t = 10/0.8 = 12.5 anos (Terra)
    //   τ = 12.5 / (5/3) = 7.5 anos (tripulação)
    //   distância contraída = 10 / (5/3) = 6.0 ly
    {
        ConstVelocityTrip trip = computeConstVelocityTrip(10.0, 0.8);
        expectNear("gamma", trip.gamma, 5.0 / 3.0);
        expectNear("tempo coordenado = 12.5 yr", trip.coordinateTimeYr, 12.5);
        expectNear("tempo proprio = 7.5 yr",     trip.properTimeYr, 7.5);
        expectNear("dist contraida = 6 ly",      trip.contractedDistanceLy, 6.0);
        // Sanidade física: tripulação sempre envelhece MENOS que a origem.
        expectTrue("tau < t", trip.properTimeYr < trip.coordinateTimeYr);
    }

    // Caso 2: Sol→Vega ≈ 25.04 ly a β=0.999 (γ≈22.366).
    //   t = 25.04/0.999 ≈ 25.065 anos; τ = t/γ ≈ 1.1207 anos.
    {
        ConstVelocityTrip trip = computeConstVelocityTrip(25.04, 0.999);
        expectNear("Vega: t ~25.065 yr", trip.coordinateTimeYr, 25.04 / 0.999, 1e-9);
        expectNear("Vega: tau ~1.12 yr",
                   trip.properTimeYr, (25.04 / 0.999) / 22.3662720421, 1e-6);
    }

    // Caso 3 (borda): β=0 → nenhum tempo decorre, sem divisão por zero.
    {
        ConstVelocityTrip trip = computeConstVelocityTrip(10.0, 0.0);
        expectNear("beta=0: gamma=1", trip.gamma, 1.0);
        expectNear("beta=0: t=0", trip.coordinateTimeYr, 0.0);
        expectNear("beta=0: tau=0", trip.properTimeYr, 0.0);
        expectTrue("beta=0: finito", std::isfinite(trip.coordinateTimeYr));
    }

    // Caso 4 (borda): distância 0 → tudo zero.
    {
        ConstVelocityTrip trip = computeConstVelocityTrip(0.0, 0.9);
        expectNear("dist=0: t=0", trip.coordinateTimeYr, 0.0);
        expectNear("dist=0: tau=0", trip.properTimeYr, 0.0);
    }
}

}  // namespace

int main() {
    std::printf("== starlag T3.1 — testes de dilatacao temporal ==\n");

    testLorentzFactor();
    testNumericalStabilityNearC();
    testClampBeta();
    testTimeRelations();
    testConstVelocityTrip();

    if (g_failures == 0) {
        std::printf("\nTODOS os testes passaram.\n");
        return 0;
    }
    std::printf("\n%d teste(s) FALHARAM.\n", g_failures);
    return 1;
}
