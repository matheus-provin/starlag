// ============================================================================
//  starlag — Simulador 3D de Viagens Interestelares Relativísticas
//  Ponto de entrada do aplicativo.
//
//  Estado atual: abre uma janela Metal (via GLFW) com a tela limpa (M0/T0.2).
//  Antes de abrir a janela, imprime no terminal um "relatório de viagem" de
//  exemplo (Sol→Vega) usando o núcleo de física já validado (M3: T3.1/T3.2),
//  para tornar a física tangível dentro do app real — não só nos testes.
//  Render de estrelas (M2) e UI ImGui (M4) entram nos próximos marcos.
// ============================================================================

#include "physics/Calendar.h"
#include "physics/Simulation.h"
#include "render/MetalWindow.h"

#include <chrono>
#include <cstdio>
#include <exception>

namespace {

// Imprime uma simulação usando a fachada runSimulation (T3.4): empacota todas as
// métricas + datas + resumo em linguagem natural numa única chamada.
void printResult(const char* titulo, const starlag::physics::SimulationResult& r) {
    using namespace starlag::physics;
    std::printf("\n[%s]\n", titulo);
    std::printf("  Velocidade de pico       : %.5f c (gamma=%.3f)\n", r.peakBeta, r.peakGamma);
    std::printf("  Tempo na origem          : %.3f anos  -> chegada %s\n",
                r.coordinateTimeYr, formatDate(r.arrivalDateOrigin).c_str());
    std::printf("  Tempo a bordo (tripulac.): %.3f anos  -> relogio  %s\n",
                r.properTimeYr, formatDate(r.arrivalDateShip).c_str());
    std::printf("  Distancia contraida      : %.3f anos-luz\n", r.contractedDistanceLy);
    std::printf("  'Divida temporal'        : %.3f anos para o futuro\n", r.timeDebtYr);
    std::printf("  Resumo: %s\n", r.summary.c_str());
}

// "Prova de vida" da física dentro do executável do app (Sol→Vega, dois modos).
void printDemoTrip() {
    using namespace starlag::physics;

    TripRequest base;
    base.distanceLy = 25.04;
    base.departureDate = Date{2026, 6, 15, 0.0};
    base.originName = "Terra";
    base.destinationName = "Vega";

    std::printf("\n=================================================\n");
    std::printf("  starlag — relatorio de viagem (demo de fisica)\n");
    std::printf("  Rota: Sol -> %s  (%.2f anos-luz)\n",
                base.destinationName.c_str(), base.distanceLy);
    std::printf("  Partida: %s\n", formatDate(base.departureDate).c_str());
    std::printf("=================================================\n");

    TripRequest constReq = base;
    constReq.mode = PhysicsMode::ConstantVelocity;
    constReq.cruiseBeta = 0.99;
    printResult("Modo velocidade constante @ 99% c", runSimulation(constReq));

    TripRequest accelReq = base;
    accelReq.mode = PhysicsMode::Accelerated;
    accelReq.accelG = 1.0;
    accelReq.cruiseBeta = 0.9999999;
    printResult("Modo acelerado @ 1 g", runSimulation(accelReq));

    std::printf("\n");
}

}  // namespace

int main() {
    using clock = std::chrono::steady_clock;

    try {
        // Prova de vida da física dentro do app (antes de abrir a janela).
        printDemoTrip();

        starlag::render::MetalWindow window(1280, 720, "starlag");
        // Azul-noite escuro: o céu profundo onde as estrelas serão desenhadas
        // a partir do Marco 2. (Validado em T0.2 que o loop limpa a cada frame.)
        const starlag::render::ClearColor skyColor{0.02, 0.02, 0.06, 1.0};

        std::printf("starlag v0.1.0 — janela Metal aberta (feche para sair).\n");

        // Contadores para medir FPS médio a cada ~1 segundo.
        auto fpsWindowStart = clock::now();
        int framesInWindow = 0;

        while (window.isOpen()) {
            window.pollEvents();
            window.renderClearFrame(skyColor);

            // --- Medição de FPS ---
            ++framesInWindow;
            const auto now = clock::now();
            const std::chrono::duration<double> elapsed = now - fpsWindowStart;
            if (elapsed.count() >= 1.0) {
                const double fps = framesInWindow / elapsed.count();
                std::printf("FPS: %.1f\n", fps);
                framesInWindow = 0;
                fpsWindowStart = now;
            }
        }

        std::printf("Janela fechada. Ate logo.\n");
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Erro fatal: %s\n", e.what());
        return 1;
    }
}
