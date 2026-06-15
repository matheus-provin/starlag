// ============================================================================
//  test_simulation — testes da fachada SimulationResult (T3.4).
//
//  A fachada apenas orquestra módulos já testados (T3.1/T3.2/T3.3), então aqui
//  validamos: (a) os valores agregados batem com os módulos subjacentes;
//  (b) métricas derivadas (dívida temporal, distância contraída) estão corretas;
//  (c) consistência entre modos; (d) o resumo textual é gerado.
//  Referências reusadas das tarefas anteriores (já verificadas em Python).
// ============================================================================

#include "physics/Simulation.h"

#include <cmath>
#include <cstdio>
#include <string>

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

// --- Modo velocidade constante: Vega 25.04 ly @ 0.99c -----------------------
void testConstMode() {
    std::printf("[modo constante: Vega @0.99c]\n");
    TripRequest req;
    req.distanceLy = 25.04;
    req.mode = PhysicsMode::ConstantVelocity;
    req.cruiseBeta = 0.99;
    req.departureDate = Date{2026, 6, 15, 0.0};
    req.destinationName = "Vega";

    SimulationResult r = runSimulation(req);

    expectNear("gamma = 7.0888", r.peakGamma, 7.0888120501, 1e-6);
    expectNear("beta pico = 0.99", r.peakBeta, 0.99);
    expectNear("tempo coordenado", r.coordinateTimeYr, 25.04 / 0.99, 1e-6);
    expectNear("tempo proprio", r.properTimeYr, (25.04 / 0.99) / 7.0888120501, 1e-5);
    expectNear("dist contraida = D/gamma", r.contractedDistanceLy,
               25.04 / 7.0888120501, 1e-5);
    expectNear("divida temporal = t - tau", r.timeDebtYr,
               r.coordinateTimeYr - r.properTimeYr);
    // Chegada na origem: 2026-06-15 + 25.293 anos -> 2051-09-30 (T3.3).
    expectTrue("chegada origem 2051-09-30",
               r.arrivalDateOrigin.year == 2051 && r.arrivalDateOrigin.month == 9 &&
               r.arrivalDateOrigin.day == 30);
    expectTrue("resumo nao-vazio", !r.summary.empty());
}

// --- Modo acelerado: Vega 25.04 ly @ 1g -------------------------------------
void testAccelMode() {
    std::printf("[modo acelerado: Vega @1g]\n");
    TripRequest req;
    req.distanceLy = 25.04;
    req.mode = PhysicsMode::Accelerated;
    req.accelG = 1.0;
    req.cruiseBeta = 0.9999999;
    req.departureDate = Date{2026, 6, 15, 0.0};
    req.destinationName = "Vega";

    SimulationResult r = runSimulation(req);

    // Valores de referência da T3.2 (perfil triangular).
    expectNear("tempo proprio ~6.44291", r.properTimeYr, 6.44291127, 1e-5);
    expectNear("tempo coordenado ~26.90777", r.coordinateTimeYr, 26.9077703, 1e-5);
    expectNear("gamma pico ~13.92434", r.peakGamma, 13.9243368, 1e-5);
    expectNear("beta pico ~0.997418", r.peakBeta, 0.9974178466, 1e-6);
    // Distância contraída = D / gamma_pico (escolha documentada).
    expectNear("dist contraida = D/gamma_pico", r.contractedDistanceLy,
               25.04 / 13.9243368, 1e-5);
    expectTrue("kind triangular", r.flight.kind == FlightKind::Triangular);
    // Chegada relógio de bordo: 2026-06-15 + 6.443 -> 2032-11-23 (T3.3).
    expectTrue("chegada bordo 2032-11-23",
               r.arrivalDateShip.year == 2032 && r.arrivalDateShip.month == 11 &&
               r.arrivalDateShip.day == 23);
}

// --- Consistência geral / sanidade ------------------------------------------
void testConsistency() {
    std::printf("[consistencia]\n");
    TripRequest req;
    req.distanceLy = 100.0;
    req.mode = PhysicsMode::Accelerated;
    req.accelG = 1.0;
    req.cruiseBeta = 0.99;
    SimulationResult r = runSimulation(req);

    expectTrue("tau < t", r.properTimeYr < r.coordinateTimeYr);
    expectTrue("divida temporal >= 0", r.timeDebtYr >= 0.0);
    expectTrue("dist contraida <= dist real", r.contractedDistanceLy <= r.distanceLy);
    expectTrue("chegada origem >= chegada bordo (ano)",
               r.arrivalDateOrigin.year >= r.arrivalDateShip.year);
    expectTrue("tempos finitos",
               std::isfinite(r.properTimeYr) && std::isfinite(r.coordinateTimeYr));

    // Borda: distância 0 → viagem nula, sem NaN.
    SimulationResult z = runSimulation(TripRequest{});  // distanceLy=0 default.
    expectNear("dist=0: tau=0", z.properTimeYr, 0.0);
    expectNear("dist=0: divida=0", z.timeDebtYr, 0.0);
}

}  // namespace

int main() {
    std::printf("== starlag T3.4 — testes da fachada de simulacao ==\n");

    testConstMode();
    testAccelMode();
    testConsistency();

    if (g_failures == 0) {
        std::printf("\nTODOS os testes passaram.\n");
        return 0;
    }
    std::printf("\n%d teste(s) FALHARAM.\n", g_failures);
    return 1;
}
